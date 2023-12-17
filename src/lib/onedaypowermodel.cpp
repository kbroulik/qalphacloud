/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "onedaypowermodel.h"

#include "apirequest.h"
#include "connector.h"
#include "qalphacloud.h"
#include "qalphacloud_log.h"
#include "utils_p.h"

#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>
#include <QPointer>
#include <QVector>

#include <algorithm>

struct PowerEntry {
    static PowerEntry fromJson(const QJsonObject &json)
    {
        const auto photovoltaicPower = int{json.value(QStringLiteral("ppv")).toInt()};
        const auto currentLoad = int{json.value(QStringLiteral("load")).toInt()};
        // NOTE in documentation this is just called "feed".
        const auto gridFeed = int{json.value(QStringLiteral("feedIn")).toInt()};
        const auto gridCharge = int{json.value(QStringLiteral("gridCharge")).toInt()};

        const auto batterySoc = json.value(QStringLiteral("cbat")).toDouble();

        const QDateTime uploadTime = QDateTime::fromString(json.value(QStringLiteral("uploadTime")).toString(), Qt::ISODate);

        PowerEntry entry{// TODO Should we just read this from the JSON every time?
                         json,
                         uploadTime,
                         photovoltaicPower,
                         currentLoad,
                         gridFeed,
                         gridCharge,
                         batterySoc};

        return entry;
    }

    bool operator==(const PowerEntry &other) const
    {
        return json == other.json;
    }

    bool operator!=(const PowerEntry &other) const
    {
        return !(json == other.json);
    }

    QJsonObject json;
    QDateTime uploadTime; // uploadTime

    // QString serialNumber; // sysSn
    int photovoltaicPower = 1337; // ppv
    int currentLoad = 0; // load
    int gridFeed = 0; // feed(In)
    int gridCharge = 0; // feed(In)

    qreal batterySoc = 0.0; // cbat

    // TODO chargingPile
};
Q_DECLARE_TYPEINFO(PowerEntry, Q_MOVABLE_TYPE);

namespace QAlphaCloud
{

class OneDayPowerModelPrivate
{
public:
    OneDayPowerModelPrivate(OneDayPowerModel *qq)
        : q(qq)
    {
    }

    void setFromDateTime(const QDateTime &fromDateTime);
    void setToDateTime(const QDateTime &toDateTime);

    void setPeakPhotovoltaic(int peakPhotovoltaic);
    void setPeakLoad(int peakLoad);
    void setPeakGridFeed(int peakGridFeed);
    void setPeakGridCharge(int peakGridCharge);

    void setStatus(RequestStatus status);
    void setError(ErrorCode error);
    void setErrorString(const QString &errorString);

    void processApiResult(const QJsonArray &jsonArray);

    OneDayPowerModel *const q;

    // TODO QPointer?
    Connector *m_connector = nullptr;
    QString m_serialNumber;
    QDate m_date;
    bool m_cached = true;

    QDateTime m_fromDateTime;
    QDateTime m_toDateTime;

    int m_peakPhotovoltaic = 0;
    int m_peakLoad = 0;
    int m_peakGridFeed = 0;
    int m_peakGridCharge = 0;

    QVector<PowerEntry> m_data;
    RequestStatus m_status = RequestStatus::NoRequest;
    ErrorCode m_error = ErrorCode::NoError;
    QString m_errorString;

    QPointer<ApiRequest> m_request;

    QHash<QDate, QJsonArray> m_cache;
};

void OneDayPowerModelPrivate::setFromDateTime(const QDateTime &fromDateTime)
{
    if (m_fromDateTime != fromDateTime) {
        m_fromDateTime = fromDateTime;
        Q_EMIT q->fromDateTimeChanged(fromDateTime);
    }
}

void OneDayPowerModelPrivate::setToDateTime(const QDateTime &toDateTime)
{
    if (m_toDateTime != toDateTime) {
        m_toDateTime = toDateTime;
        Q_EMIT q->toDateTimeChanged(toDateTime);
    }
}

void OneDayPowerModelPrivate::setPeakPhotovoltaic(int peakPhotovoltaic)
{
    if (m_peakPhotovoltaic != peakPhotovoltaic) {
        m_peakPhotovoltaic = peakPhotovoltaic;
        Q_EMIT q->peakPhotovoltaicChanged(peakPhotovoltaic);
    }
}

void OneDayPowerModelPrivate::setPeakLoad(int peakLoad)
{
    if (m_peakLoad != peakLoad) {
        m_peakLoad = peakLoad;
        Q_EMIT q->peakLoadChanged(peakLoad);
    }
}

void OneDayPowerModelPrivate::setPeakGridFeed(int peakGridFeed)
{
    if (m_peakGridFeed != peakGridFeed) {
        m_peakGridFeed = peakGridFeed;
        Q_EMIT q->peakGridFeedChanged(peakGridFeed);
    }
}

void OneDayPowerModelPrivate::setPeakGridCharge(int peakGridCharge)
{
    if (m_peakGridCharge != peakGridCharge) {
        m_peakGridCharge = peakGridCharge;
        Q_EMIT q->peakGridChargeChanged(peakGridCharge);
    }
}

void OneDayPowerModelPrivate::setStatus(RequestStatus status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT q->statusChanged(status);
    }
}

void OneDayPowerModelPrivate::setError(ErrorCode error)
{
    if (m_error != error) {
        m_error = error;
        Q_EMIT q->errorChanged(error);
    }
}

void OneDayPowerModelPrivate::setErrorString(const QString &errorString)
{
    if (m_errorString != errorString) {
        m_errorString = errorString;
        Q_EMIT q->errorStringChanged(errorString);
    }
}

void OneDayPowerModelPrivate::processApiResult(const QJsonArray &jsonArray)
{
    int peakPhotovoltaic = 0;
    int peakLoad = 0;
    int peakGridFeed = 0;
    int peakGridCharge = 0;

    QVector<PowerEntry> entries;
    entries.reserve(jsonArray.count());

    for (const QJsonValue &jsonValue : jsonArray) {
        const auto entry = PowerEntry::fromJson(jsonValue.toObject());

        if (entry.photovoltaicPower > peakPhotovoltaic) {
            peakPhotovoltaic = entry.photovoltaicPower;
        }
        if (entry.currentLoad > peakLoad) {
            peakLoad = entry.currentLoad;
        }
        if (entry.gridFeed > peakGridFeed) {
            peakGridFeed = entry.gridFeed;
        }
        if (entry.gridCharge > peakGridCharge) {
            peakGridCharge = entry.gridCharge;
        }

        entries << entry;
    }

    std::sort(entries.begin(), entries.end(), [](const PowerEntry &a, const PowerEntry &b) {
        return a.uploadTime < b.uploadTime;
    });

    // TODO delta update

    q->beginResetModel();
    m_data = entries;
    q->endResetModel();

    QDateTime fromDateTime;
    QDateTime toDateTime;
    if (!entries.isEmpty()) {
        fromDateTime = entries.constFirst().uploadTime;
        toDateTime = entries.constLast().uploadTime;
    }

    setFromDateTime(fromDateTime);
    setToDateTime(toDateTime);

    setPeakPhotovoltaic(peakPhotovoltaic);
    setPeakLoad(peakLoad);
    setPeakGridFeed(peakGridFeed);
    setPeakGridCharge(peakGridCharge);

    setStatus(RequestStatus::Finished);
}

OneDayPowerModel::OneDayPowerModel(QObject *parent)
    : OneDayPowerModel(nullptr, QString(), QDate::currentDate(), parent)
{
}

OneDayPowerModel::OneDayPowerModel(Connector *connector, const QString &serialNumber, const QDate &date, QObject *parent)
    : QAbstractListModel(parent)
    , d(std::make_unique<OneDayPowerModelPrivate>(this))
{
    setConnector(connector);

    d->m_serialNumber = serialNumber;
    d->m_date = date;

    connect(this, &OneDayPowerModel::rowsInserted, this, &OneDayPowerModel::countChanged);
    connect(this, &OneDayPowerModel::rowsRemoved, this, &OneDayPowerModel::countChanged);
    connect(this, &OneDayPowerModel::modelReset, this, &OneDayPowerModel::countChanged);
}

OneDayPowerModel::~OneDayPowerModel() = default;

Connector *OneDayPowerModel::connector() const
{
    return d->m_connector;
}

void OneDayPowerModel::setConnector(Connector *connector)
{
    if (d->m_connector == connector) {
        return;
    }

    d->m_connector = connector;
    d->m_cache.clear();
    reset();
    Q_EMIT connectorChanged(connector);
}

QString OneDayPowerModel::serialNumber() const
{
    return d->m_serialNumber;
}

void OneDayPowerModel::setSerialNumber(const QString &serialNumber)
{
    if (d->m_serialNumber == serialNumber) {
        return;
    }

    d->m_serialNumber = serialNumber;
    d->m_cache.clear();
    reset();
    Q_EMIT serialNumberChanged(serialNumber);
}

QDate OneDayPowerModel::date() const
{
    return d->m_date;
}

void OneDayPowerModel::setDate(const QDate &date)
{
    if (d->m_date == date) {
        return;
    }

    d->m_date = date;
    reset();
    Q_EMIT dateChanged(date);
}

void OneDayPowerModel::resetDate()
{
    setDate(QDate::currentDate());
}

bool OneDayPowerModel::cached() const
{
    return d->m_cached;
}

void OneDayPowerModel::setCached(bool cached)
{
    if (d->m_cached == cached) {
        return;
    }

    d->m_cached = cached;
    if (!cached) {
        d->m_cache.clear();
    }
    Q_EMIT cachedChanged(cached);
}

QDateTime OneDayPowerModel::fromDateTime() const
{
    return d->m_fromDateTime;
}

QDateTime OneDayPowerModel::toDateTime() const
{
    return d->m_toDateTime;
}

int OneDayPowerModel::peakPhotovoltaic() const
{
    return d->m_peakPhotovoltaic;
}

int OneDayPowerModel::peakLoad() const
{
    return d->m_peakLoad;
}

int OneDayPowerModel::peakGridFeed() const
{
    return d->m_peakGridFeed;
}

int OneDayPowerModel::peakGridCharge() const
{
    return d->m_peakGridCharge;
}

RequestStatus OneDayPowerModel::status() const
{
    return d->m_status;
}

ErrorCode OneDayPowerModel::error() const
{
    return d->m_error;
}

QString OneDayPowerModel::errorString() const
{
    return d->m_errorString;
}

int OneDayPowerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->m_data.count();
}

QVariant OneDayPowerModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const auto &item = d->m_data.at(index.row());

    switch (static_cast<Roles>(role)) {
    case Roles::PhotovoltaicEnergy:
        return item.photovoltaicPower;
    case Roles::CurrentLoad:
        return item.currentLoad;
    case Roles::GridFeed:
        return item.gridFeed;
    case Roles::GridCharge:
        return item.gridCharge;
    case Roles::BatterySoc:
        return item.batterySoc;
    case Roles::UploadTime:
        return item.uploadTime;
    case Roles::RawJson:
        return item.json;
    }

    return {};
}

QHash<int, QByteArray> OneDayPowerModel::roleNames() const
{
    return Utils::roleNamesFromEnum(QMetaEnum::fromType<Roles>());
}

bool OneDayPowerModel::reload()
{
    if (!d->m_connector) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load OneDayPowerModel without a connector";
        return false;
    }

    if (d->m_serialNumber.isEmpty()) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load OneDayPowerModel without a serial number";
        return false;
    }

    const QDate date = d->m_date;
    if (!date.isValid()) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load OneDayPowerModel without a valid date";
        return false;
    }

    if (d->m_request) {
        qCDebug(QALPHACLOUD_LOG) << "Cancelling OneDayPowerModel request in-flight";
        d->m_request->abort();
        d->m_request = nullptr;
    }

    const auto cachedData = d->m_cache.value(date);
    if (!cachedData.isEmpty()) {
        d->processApiResult(cachedData);
        return true;
    }

    auto *request = new ApiRequest(d->m_connector, ApiRequest::EndPoint::OneDayPowerBySn, this);
    request->setSysSn(d->m_serialNumber);
    request->setQueryDate(date);

    connect(request, &ApiRequest::errorOccurred, this, [this, request] {
        d->setError(request->error());
        d->setErrorString(request->errorString());
        d->setStatus(QAlphaCloud::RequestStatus::Error);
    });

    connect(request, &ApiRequest::result, this, [this, request, date] {
        const QJsonArray jsonArray = request->data().toArray();

        d->processApiResult(jsonArray);

        // Don't cache today's data as it will gain new data as the day progresses.
        // Also don't cache if there is no data.
        if (d->m_cached && !jsonArray.isEmpty() && date != QDate::currentDate()) {
            d->m_cache.insert(date, jsonArray);
        }
    });

    const bool ok = request->send();

    if (ok) {
        d->m_request = request;

        d->setStatus(QAlphaCloud::RequestStatus::Loading);
    }

    return ok;
}

bool OneDayPowerModel::forceReload()
{
    d->m_cache.clear();
    return reload();
}

void OneDayPowerModel::reset()
{
    beginResetModel();
    if (d->m_request) {
        d->m_request->abort();
        d->m_request = nullptr;
    }
    d->m_data.clear();
    d->setStatus(RequestStatus::NoRequest);
    endResetModel();
}

} // namespace QAlphaCloud
