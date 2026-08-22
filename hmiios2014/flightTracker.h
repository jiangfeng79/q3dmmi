#pragma once

// ---------------------------------------------------------------------------
// Live airflight tracking near Changi.
//
// Adapted from the standalone ../changiFlights project. A TrackerWorker runs
// on a dedicated worker thread, polls the adsb.lol API for aircraft within a
// radius of Changi Airport, maintains a position table keyed by aircraft hex,
// and emits the current table to the map renderer (TSDWindow), which draws the
// aircraft as points + callsign labels on the map.
// ---------------------------------------------------------------------------

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QHash>
#include <QSet>
#include <QDateTime>

// A single aircraft snapshot from the adsb.lol API.
struct Aircraft {
    QString hex;
    QString callsign;
    QString type;
    double lat = 0.0;
    double lon = 0.0;
    int altBaro = 0;
    double groundSpeed = 0.0;
};

// A single position sample recorded for an aircraft.
struct PositionSample {
    double lat = 0.0;
    double lon = 0.0;
    QDateTime timestamp;
};

// Per-aircraft tracking state kept in the worker's position table.
struct TrackedAircraft {
    Aircraft latest;
    QList<PositionSample> history;
    int missedCount = 0; // consecutive polls where the aircraft was absent
};

// Runs on a dedicated worker thread. Polls the adsb.lol API on a fixed
// interval, maintains a position tracking table keyed by aircraft hex, and
// emits the current table to the caller. An aircraft is removed from the
// table only after it has been absent from the query result for
// `m_missThreshold` consecutive polls.
class TrackerWorker : public QObject {
    Q_OBJECT

public:
    explicit TrackerWorker(QObject *parent = nullptr) : QObject(parent) {
        // Setup primary and fallback mirrors using the readsb v2 API format
        m_endpoints << "https://opendata.adsb.fi/api/v3/lat/%1/lon/%2/dist/%3"
                    << "https://api.adsb.lol/v2/lat/%1/lon/%2/dist/%3";
    }

    // Start polling. Must be called from the worker thread (e.g. via
    // moveToThread). The network manager and timer are created here so they
    // inherit the worker thread's affinity.
    void start(double lat = 1.3644, double lon = 103.9915, int radiusNm = 30) {
        m_lat = lat;
        m_lon = lon;
        m_radiusNm = radiusNm;

        // Create without a parent so they inherit the worker thread's affinity
        // (the worker object itself was constructed on the main thread).
        m_nam = new QNetworkAccessManager;
        connect(m_nam, &QNetworkAccessManager::finished,
                this, &TrackerWorker::onNetworkReply);

        m_timer = new QTimer;
        m_timer->setInterval(5000);
        connect(m_timer, &QTimer::timeout, this, &TrackerWorker::fetchTelemetry);

        fetchTelemetry();
        m_timer->start();
    }

    void stop() {
        if (m_timer) {
            m_timer->stop();
        }
    }

signals:
    // Emitted (from the worker thread) with the current position tracking table.
    void trackingTableUpdated(const QHash<QString, TrackedAircraft> &table);
    void fetchFailed(const QString &errorString);

public slots:
    void fetchTelemetry() {
        if (!m_nam) return;

        const QString templateUrl = m_endpoints.at(m_currentEndpointIdx);

        const QString urlStr = templateUrl.arg(m_lat).arg(m_lon).arg(m_radiusNm);
        qWarning().noquote() << urlStr;

        QNetworkRequest request{QUrl(urlStr)};
        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36");

        m_nam->get(request);
    }

private slots:
    void onNetworkReply(QNetworkReply *reply) {
        reply->deleteLater();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || statusCode == 429) {
            qWarning().noquote() << QString("Endpoint %1 failed (HTTP Status %2): %3")
                                    .arg(m_endpoints.at(m_currentEndpointIdx))
                                    .arg(statusCode)
                                    .arg(reply->errorString());

            // Try next fallback mirror
            m_currentEndpointIdx = (m_currentEndpointIdx + 1) % m_endpoints.size();
            
            emit fetchFailed(reply->errorString());
            return;
        }

        const QByteArray responseData = reply->readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit fetchFailed(QString("JSON Parse Error: %1").arg(parseError.errorString()));
            return;
        }

        const QJsonObject rootObj = doc.object();
        QSet<QString> seenThisPoll;

        if (rootObj.contains("ac") && rootObj["ac"].isArray()) {
            const QJsonArray acArray = rootObj["ac"].toArray();

            for (const QJsonValue &val : acArray) {
                if (!val.isObject()) continue;

                const QJsonObject obj = val.toObject();
                Aircraft ac;

                ac.hex = obj.value("hex").toString("N/A");

                // Trim trailing space padded callsigns (e.g., "SIA318  ")
                const QString rawCallsign = obj.value("flight").toString().trimmed();
                ac.callsign = rawCallsign.isEmpty() ? "NO_CALLSIGN" : rawCallsign;

                ac.type = obj.value("t").toString("UNK");
                ac.lat = obj.value("lat").toDouble(0.0);
                ac.lon = obj.value("lon").toDouble(0.0);
                ac.groundSpeed = obj.value("gs").toDouble(0.0);

                // Handle 'alt_baro' which can be int or string ("ground")
                const QJsonValue altVal = obj.value("alt_baro");
                if (altVal.isDouble()) {
                    ac.altBaro = altVal.toInt();
                } else if (altVal.isString() && altVal.toString() == "ground") {
                    ac.altBaro = 0;
                }

                // Update (or insert) the tracking entry for this aircraft.
                TrackedAircraft &entry = m_table[ac.hex];
                entry.latest = ac;
                entry.missedCount = 0;
                entry.history.append({ac.lat, ac.lon, QDateTime::currentDateTime()});

                seenThisPoll.insert(ac.hex);
            }
        }

        // Mark aircraft that were not seen this poll; remove them once they
        // have been missed for `m_missThreshold` consecutive polls.
        //
        // Use a while loop (not a for loop with ++it): QHash::erase() returns
        // the iterator to the next element, so we must not advance again after
        // an erase. Advancing past end() (which happens when the last element
        // is erased) is undefined behavior and crashes.
        auto it = m_table.begin();
        while (it != m_table.end()) {
            if (!seenThisPoll.contains(it.key())) {
                it.value().missedCount++;
                if (it.value().missedCount >= m_missThreshold) {
                    it = m_table.erase(it);
                    continue;
                }
            }
            ++it;
        }

        qWarning() << "Tracking table updated with " << m_table.size() << " aircraft.";
        emit trackingTableUpdated(m_table);
    }

private:
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_timer = nullptr;

    double m_lat = 1.3644;
    double m_lon = 103.9915;
    int m_radiusNm = 30;
    QStringList m_endpoints;
    int m_currentEndpointIdx = 0;

    // Number of consecutive missed polls before an aircraft is removed.
    static constexpr int m_missThreshold = 3;

    QHash<QString, TrackedAircraft> m_table;
};
