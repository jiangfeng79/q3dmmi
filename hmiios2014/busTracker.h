#pragma once

#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSslError>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>

// Represents a single arriving bus.
struct ArrivalBus
{
    QString originCode;
    QString destinationCode;
    QString estimatedArrival;
    double latitude = 0.0;
    double longitude = 0.0;
    QString visitNumber;
    QString load;
    QString feature;
    QString type;
    double heading = 0.0;
};

// Represents a bus service at the stop.
struct BusService
{
    QString serviceNo;

    ArrivalBus nextBus;
    ArrivalBus nextBus2;
    ArrivalBus nextBus3;
};

// Tracking state for a bus stop.
struct BusStopSnapshot
{
    QString busStopCode;
    QList<BusService> services;
    QDateTime lastUpdated;
};

Q_DECLARE_METATYPE(ArrivalBus)
Q_DECLARE_METATYPE(BusService)
Q_DECLARE_METATYPE(BusStopSnapshot)

class BusTrackerWorker : public QObject
{
    Q_OBJECT

public:
    explicit BusTrackerWorker(QObject* parent = nullptr) : QObject(parent)
    {
        qRegisterMetaType<ArrivalBus>("ArrivalBus");
        qRegisterMetaType<BusService>("BusService");
        qRegisterMetaType<BusStopSnapshot>("BusStopSnapshot");
    }

    ~BusTrackerWorker() override
    {
        stop();
    }

    void setAccountKey(const QString& accountKey)
    {
        m_accountKey = accountKey.trimmed();
    }

    void setBusStopCode(const QString& busStopCode)
    {
        m_busStopCode = busStopCode.trimmed();
    }

    void setPollInterval(int pollIntervalMs)
    {
        m_pollIntervalMs = pollIntervalMs;
        if (m_timer)
        {
            m_timer->setInterval(m_pollIntervalMs);
        }
    }

    void trackBusStop(const QString& busStopCode, const QString& accountKey = QString(), int pollIntervalMs = 15000)
    {
        stop();

        if (!accountKey.trimmed().isEmpty())
        {
            m_accountKey = accountKey.trimmed();
        }
        if (!busStopCode.trimmed().isEmpty())
        {
            m_busStopCode = busStopCode.trimmed();
        }
        m_pollIntervalMs = pollIntervalMs;

        m_nam = new QNetworkAccessManager(this);
        connect(m_nam, &QNetworkAccessManager::finished, this, &BusTrackerWorker::onNetworkReply);

        m_timer = new QTimer(this);
        m_timer->setInterval(m_pollIntervalMs);
        connect(m_timer, &QTimer::timeout, this, &BusTrackerWorker::fetchArrivals);

        fetchArrivals();
        m_timer->start();
    }

    void start(const QString& accountKey = QString(), const QString& busStopCode = QString(), int pollIntervalMs = 15000)
    {
        trackBusStop(busStopCode, accountKey, pollIntervalMs);
    }

    void stop()
    {
        if (m_timer)
        {
            m_timer->stop();
            delete m_timer;
            m_timer = nullptr;
        }

        if (m_nam)
        {
            delete m_nam;
            m_nam = nullptr;
        }
    }

signals:
    void busArrivalUpdated(const BusStopSnapshot& snapshot);
    void fetchFailed(const QString& errorString);

public slots:
    void fetchArrivals()
    {
        fetchArrivalsFor(m_busStopCode, m_accountKey);
    }

    void fetchArrivalsFor(const QString& busStopCode, const QString& accountKey = QString())
    {
        if (!accountKey.trimmed().isEmpty())
        {
            m_accountKey = accountKey.trimmed();
        }
        if (!busStopCode.trimmed().isEmpty())
        {
            m_busStopCode = busStopCode.trimmed();
        }

        if (m_busStopCode.isEmpty())
        {
            return;
        }

        if (!m_nam)
        {
            m_nam = new QNetworkAccessManager(this);
            connect(m_nam, &QNetworkAccessManager::finished, this, &BusTrackerWorker::onNetworkReply);
        }

        QString url = QString(
                          "https://datamall2.mytransport.sg/"
                          "ltaodataservice/v3/BusArrival?"
                          "BusStopCode=%1")
                          .arg(m_busStopCode);

        QNetworkRequest request{QUrl(url)};
        request.setRawHeader("AccountKey", m_accountKey.toUtf8());
        request.setRawHeader("Accept", "application/json");

        QSslConfiguration sslConfig = request.sslConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        request.setSslConfiguration(sslConfig);

        QNetworkReply* reply = m_nam->get(request);
        connect(reply, &QNetworkReply::sslErrors, reply,
                static_cast<void (QNetworkReply::*)(const QList<QSslError>&)>(&QNetworkReply::ignoreSslErrors));
    }

private slots:
    void onNetworkReply(QNetworkReply* reply)
    {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            qWarning().noquote() << "BusTrackerWorker network error:" << reply->errorString();
            emit fetchFailed(reply->errorString());
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            qWarning().noquote() << "BusTrackerWorker JSON parse error:" << parseError.errorString();
            emit fetchFailed(parseError.errorString());
            return;
        }

        QJsonObject root = doc.object();
        BusStopSnapshot snapshot;
        snapshot.busStopCode = root.value("BusStopCode").toString();
        if (snapshot.busStopCode.isEmpty())
        {
            snapshot.busStopCode = m_busStopCode;
        }
        snapshot.lastUpdated = QDateTime::currentDateTime();
        QJsonArray services = root.value("Services").toArray();

        auto parseBus = [](const QJsonObject& obj) -> ArrivalBus
        {
            ArrivalBus bus;
            bus.originCode = obj.value("OriginCode").toString();
            bus.destinationCode = obj.value("DestinationCode").toString();
            bus.estimatedArrival = obj.value("EstimatedArrival").toString();

            QJsonValue latVal = obj.value("Latitude");
            if (latVal.isString())
            {
                bus.latitude = latVal.toString().toDouble();
            }
            else
            {
                bus.latitude = latVal.toDouble(0.0);
            }

            QJsonValue lonVal = obj.value("Longitude");
            if (lonVal.isString())
            {
                bus.longitude = lonVal.toString().toDouble();
            }
            else
            {
                bus.longitude = lonVal.toDouble(0.0);
            }

            bus.visitNumber = obj.value("VisitNumber").toString();
            bus.load = obj.value("Load").toString();
            bus.feature = obj.value("Feature").toString();
            bus.type = obj.value("Type").toString();
            return bus;
        };

        for (const QJsonValue& serviceValue : services)
        {
            QJsonObject serviceObj = serviceValue.toObject();
            BusService service;
            service.serviceNo = serviceObj.value("ServiceNo").toString();

            service.nextBus = parseBus(serviceObj.value("NextBus").toObject());
            service.nextBus2 = parseBus(serviceObj.value("NextBus2").toObject());
            service.nextBus3 = parseBus(serviceObj.value("NextBus3").toObject());
            snapshot.services.append(service);
        }

        if (snapshot.services.isEmpty())
        {
            return;
        }

        getHeadings(snapshot, m_snapshot);
        m_snapshot = snapshot;

        emit busArrivalUpdated(m_snapshot);
    }

private:
    QNetworkAccessManager* m_nam = nullptr;
    QTimer* m_timer = nullptr;
    int m_pollIntervalMs = 15000;

    QString m_accountKey;
    QString m_busStopCode;

    BusStopSnapshot m_snapshot;
    // Derive each bus's heading (radians, clockwise from north) by comparing
    // its position in the new snapshot against the previous one. Buses are
    // matched across polls by visit number so a bus that shifts between the
    // Next/2nd/3rd slots is still tracked. When a bus has not moved, its last
    // known heading is carried forward.
    void getHeadings(BusStopSnapshot& newSnapshot, const BusStopSnapshot& previousSnapshot)
    {
        if (previousSnapshot.services.size() != newSnapshot.services.size())
        {
            return;
        }

        auto headingBetween = [](const ArrivalBus& prev, const ArrivalBus& next) -> double {
            const double dLat = next.latitude - prev.latitude;
            const double dLon = (next.longitude - prev.longitude) * std::cos(prev.latitude * M_PI / 180.0);
            if (std::fabs(dLat) > 1e-9 || std::fabs(dLon) > 1e-9)
            {
                return std::atan2(dLon, dLat);
            }
            return prev.heading;  // no movement: keep last known heading
        };

        auto updateBus = [&headingBetween](const ArrivalBus& prevBus, ArrivalBus& nextBus) {
            if (nextBus.visitNumber.isEmpty())
            {
                return;
            }

            if (prevBus.visitNumber == nextBus.visitNumber)
            {
                nextBus.heading = headingBetween(prevBus, nextBus);
                return;
            }
            // No matching previous bus: keep the default heading.
        };

        for (int i = 0; i < newSnapshot.services.size(); ++i)
        {
            BusService& service = newSnapshot.services[i];
            const BusService& prevService = previousSnapshot.services.value(i);

            updateBus(prevService.nextBus, service.nextBus);
            updateBus(prevService.nextBus2, service.nextBus2);
            updateBus(prevService.nextBus3, service.nextBus3);
        }
    }
};