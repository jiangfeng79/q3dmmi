#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSet>
#include <QSslError>
#include <QSslSocket>
#include <QUrl>

// ---------------------------------------------------------------------------
// Bus stop metadata.
// ---------------------------------------------------------------------------

struct BusStopInfo
{
    QString busStopCode;
    QString roadName;
    QString description;

    double latitude = 0.0;
    double longitude = 0.0;
};

// ---------------------------------------------------------------------------
// Route stop.
// ---------------------------------------------------------------------------

struct RouteStop
{
    int sequence = 0;
    double distance = 0.0;
    BusStopInfo stop;
};

// ---------------------------------------------------------------------------
// Complete route.
// Key example:
//      36_1
//      36_2
//      858_1
// ---------------------------------------------------------------------------

struct BusRoute
{
    QString serviceNo;
    int direction = 1;

    QList<RouteStop> stops;
};

Q_DECLARE_METATYPE(BusStopInfo)
Q_DECLARE_METATYPE(RouteStop)
Q_DECLARE_METATYPE(BusRoute)
Q_DECLARE_METATYPE(QList<BusRoute>)

// ---------------------------------------------------------------------------
// Worker.
// Downloads:
//      BusStops
//      BusRoutes
//
// Produces:
//      routeTable["36_1"]
//      routeTable["36_2"]
//
// Suitable for drawing bus routes on map layers.
// ---------------------------------------------------------------------------

class BusRouteWorker : public QObject
{
    Q_OBJECT

public:
    explicit BusRouteWorker(QObject* parent = nullptr) : QObject(parent)
    {
        qRegisterMetaType<BusRoute>("BusRoute");
        qRegisterMetaType<QList<BusRoute>>("QList<BusRoute>");
    }

    ~BusRouteWorker() override
    {
        stop();
    }

    void setAccountKey(const QString& accountKey)
    {
        m_accountKey = accountKey.trimmed();
    }

    void setTargetBusNo(const QString& busNo)
    {
        m_targetBusNo = busNo.trimmed();
    }

    void startFetchProcess(const QString& accountKey = QString())
    {
        if (m_isFetching)
        {
            return;
        }

        stop();

        m_isFetching = true;
        m_busStopsSkip = 0;
        m_busRoutesSkip = 0;
        m_busStops.clear();
        m_routeTable.clear();
        m_emittedBusNos.clear();

        if (!accountKey.trimmed().isEmpty())
        {
            m_accountKey = accountKey.trimmed();
        }
        m_nam = new QNetworkAccessManager(this);
        connect(m_nam, &QNetworkAccessManager::finished, this, &BusRouteWorker::onNetworkReply);
        fetchBusStops();
    }

    void start(const QString& accountKey = QString())
    {
        startFetchProcess(accountKey);
    }

    void stop()
    {
        m_isFetching = false;
        if (m_nam)
        {
            delete m_nam;
            m_nam = nullptr;
        }
    }

public slots:
    void fetchRouteForBus(const QString& busNo, const QString& accountKey = QString())
    {
        const QString targetBus = busNo.trimmed();
        if (targetBus.isEmpty())
        {
            return;
        }

        m_targetBusNo = targetBus;
        m_requestedBusNos.insert(targetBus);

        if (!accountKey.trimmed().isEmpty())
        {
            m_accountKey = accountKey.trimmed();
        }

        QList<BusRoute> matchingRoutes = getRoutesForBus(targetBus);
        if (!matchingRoutes.isEmpty())
        {
            m_emittedBusNos.insert(targetBus);
            emit busRouteReady(targetBus, matchingRoutes);
            return;
        }

        if (!m_isFetching)
        {
            startFetchProcess(m_accountKey);
        }
    }

signals:
    void routeTableUpdated(const QHash<QString, BusRoute>& routes);
    void busRouteReady(const QString& busNo, const QList<BusRoute>& routes);
    void fetchFailed(const QString& errorString);

private:
    enum class FetchState
    {
        BusStops,
        BusRoutes
    };

    QList<BusRoute> getRoutesForBus(const QString& busNo) const
    {
        QList<BusRoute> res;
        for (auto it = m_routeTable.begin(); it != m_routeTable.end(); ++it)
        {
            if (it.value().serviceNo.compare(busNo, Qt::CaseInsensitive) == 0)
            {
                res.append(it.value());
            }
        }
        std::sort(res.begin(), res.end(), [](const BusRoute& a, const BusRoute& b) {
            return a.direction < b.direction;
        });
        return res;
    }

    void issueRequest(const QString& url)
    {
        if (!m_nam)
        {
            m_nam = new QNetworkAccessManager(this);
            connect(m_nam, &QNetworkAccessManager::finished, this, &BusRouteWorker::onNetworkReply);
        }

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

    void fetchBusStops(int skip = 0)
    {
        m_state = FetchState::BusStops;
        issueRequest(QString("https://datamall2.mytransport.sg/"
                             "ltaodataservice/BusStops?$skip=%1")
                         .arg(skip));
    }

    void fetchBusRoutes(int skip = 0)
    {
        m_state = FetchState::BusRoutes;
        issueRequest(QString("https://datamall2.mytransport.sg/"
                             "ltaodataservice/BusRoutes?$skip=%1")
                         .arg(skip));
    }

private slots:
    void onNetworkReply(QNetworkReply* reply)
    {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
        {
            qWarning().noquote() << "BusRouteWorker network error:" << reply->errorString();
            m_isFetching = false;
            emit fetchFailed(reply->errorString());
            return;
        }

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            qWarning().noquote() << "BusRouteWorker parse error:" << parseError.errorString();
            m_isFetching = false;
            emit fetchFailed(parseError.errorString());
            return;
        }

        QJsonObject root = doc.object();
        QJsonArray values = root["value"].toArray();
        if (m_state == FetchState::BusStops)
        {
            parseBusStops(values);
            if (values.size() == PageSize)
            {
                m_busStopsSkip += PageSize;
                fetchBusStops(m_busStopsSkip);
                return;
            }

            qWarning().noquote() << "BusStops loaded:" << m_busStops.size() << "stops. Fetching BusRoutes...";
            fetchBusRoutes();
            return;
        }

        if (m_state == FetchState::BusRoutes)
        {
            const QSet<QString> updatedServices = parseBusRoutes(values);

            for (const QString& reqBus : m_requestedBusNos)
            {
                if (!updatedServices.contains(reqBus))
                {
                    continue;
                }

                QList<BusRoute> routes = getRoutesForBus(reqBus);
                if (!routes.isEmpty())
                {
                    for (BusRoute& r : routes)
                    {
                        for (RouteStop& rs : r.stops)
                        {
                            if ((rs.stop.latitude == 0.0 || rs.stop.longitude == 0.0) &&
                                m_busStops.contains(rs.stop.busStopCode))
                            {
                                rs.stop = m_busStops.value(rs.stop.busStopCode);
                            }
                        }
                        std::sort(r.stops.begin(), r.stops.end(), [](const RouteStop& a, const RouteStop& b) {
                            return a.sequence < b.sequence;
                        });
                        QString key = QString("%1_%2").arg(r.serviceNo).arg(r.direction);
                        m_routeTable[key] = r;
                    }

                    emit busRouteReady(reqBus, routes);
                }
            }

            if (values.size() == PageSize)
            {
                m_busRoutesSkip += PageSize;
                fetchBusRoutes(m_busRoutesSkip);
                return;
            }

            m_isFetching = false;
            emit routeTableUpdated(m_routeTable);
        }
    }

private:
    void parseBusStops(const QJsonArray& values)
    {
        for (const QJsonValue& value : values)
        {
            const QJsonObject obj = value.toObject();
            BusStopInfo stop;
            stop.busStopCode = obj["BusStopCode"].toString();
            stop.roadName = obj["RoadName"].toString();
            stop.description = obj["Description"].toString();
            stop.latitude = obj["Latitude"].toDouble();
            stop.longitude = obj["Longitude"].toDouble();
            m_busStops.insert(stop.busStopCode, stop);
        }
    }

    QSet<QString> parseBusRoutes(const QJsonArray& values)
    {
        QSet<QString> updatedServices;
        for (const QJsonValue& value : values)
        {
            const QJsonObject obj = value.toObject();
            QString serviceNo = obj["ServiceNo"].toString();
            int direction = obj["Direction"].toInt();
            QString busStopCode = obj["BusStopCode"].toString();
            QString key = QString("%1_%2").arg(serviceNo).arg(direction);

            RouteStop routeStop;
            routeStop.sequence = obj["StopSequence"].toInt();
            routeStop.distance = obj["Distance"].toDouble();

            if (m_busStops.contains(busStopCode))
            {
                routeStop.stop = m_busStops.value(busStopCode);
            }

            BusRoute& route = m_routeTable[key];
            route.serviceNo = serviceNo;
            route.direction = direction;
            route.stops.append(routeStop);
            updatedServices.insert(serviceNo);
        }
        return updatedServices;
    }

private:
    static constexpr int PageSize = 500;
    QNetworkAccessManager* m_nam = nullptr;
    QString m_accountKey;
    QString m_targetBusNo;
    FetchState m_state = FetchState::BusStops;

    int m_busStopsSkip = 0;
    int m_busRoutesSkip = 0;

    bool m_isFetching = false;
    QSet<QString> m_requestedBusNos;
    QSet<QString> m_emittedBusNos;

    QHash<QString, BusStopInfo> m_busStops;
    QHash<QString, BusRoute> m_routeTable;
};