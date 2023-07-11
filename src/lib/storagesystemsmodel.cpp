/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "storagesystemsmodel.h"

#include "apirequest_p.h"
#include "connector.h"
#include "qalphacloud.h"
#include "qalphacloud_log.h"
#include "utils_p.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QVector>

#include <algorithm>
#include <cmath>

struct StorageSystem {
    static StorageSystem fromJson(const QJsonObject &json)
    {
        const QString serialNumber = json.value(QStringLiteral("sysSn")).toString();

        QAlphaCloud::SystemStatus status = QAlphaCloud::SystemStatus::UnknownStatus;
        const QString statusString = json.value(QStringLiteral("emsStatus")).toString();
        if (statusString == QLatin1String("Normal")) {
            status = QAlphaCloud::SystemStatus::Normal;
        } else if (statusString == QLatin1String("Fault")) {
            status = QAlphaCloud::SystemStatus::Fault;
        }

        QString inverterModel = json.value(QStringLiteral("minv")).toString();
        const auto inverterPowerKwh = json.value(QStringLiteral("poinv")).toDouble();

        const QString batteryModel = json.value(QStringLiteral("mbat")).toString();
        const auto grossBatteryCapacityKwh = json.value(QStringLiteral("cobat")).toDouble();
        const auto remainingBatteryCapacityKwh = json.value(QStringLiteral("surplusCobat")).toDouble();
        const auto availableBatteryCapacity = json.value(QStringLiteral("usCapacity")).toDouble();

        const auto photovoltaicPowerKwh = json.value(QStringLiteral("popv")).toDouble();

        StorageSystem system{
            // TODO Should we just read this from the JSON every time?
            json,
            serialNumber,
            status,
            inverterModel,
            static_cast<int>(std::round(inverterPowerKwh * 1000)),
            batteryModel,
            static_cast<int>(std::round(grossBatteryCapacityKwh * 1000)),
            static_cast<int>(std::round(remainingBatteryCapacityKwh * 1000)),
            availableBatteryCapacity,
            static_cast<int>(std::round(photovoltaicPowerKwh * 1000)),
        };

        return system;
    }

    bool operator==(const StorageSystem &other) const
    {
        return json == other.json;
    }

    bool operator!=(const StorageSystem &other) const
    {
        return !(json == other.json);
    }

    QJsonObject json;

    QString serialNumber; // sysSn
    QAlphaCloud::SystemStatus status = QAlphaCloud::SystemStatus::UnknownStatus;

    QString inverterModel; // minv
    int inverterPower = 0; // poinv

    QString batteryModel; // mbat
    int grossBatteryCapacity = 0; // cobat
    int remainingBatteryCapacity = 0; // surplusCobat
    // TODO check if this can be deicmal.
    qreal usableBatteryCapacity = 0.0; // usCapacity

    int photovoltaicPower; // popv
};
Q_DECLARE_TYPEINFO(StorageSystem, Q_MOVABLE_TYPE);

namespace QAlphaCloud
{

class StorageSystemsModelPrivate
{
public:
    explicit StorageSystemsModelPrivate(StorageSystemsModel *qq)
        : q(qq)
    {
    }

    // TODO If we end up caching more, move all of this into a shared location.
    static QString cachePath();

    void setStatus(RequestStatus status);
    void setError(ErrorCode error);
    void setErrorString(const QString &errorString);

    void processApiResult(const QJsonArray &jsonArray);

    bool loadFromCache();
    bool writeToCache(const QJsonArray &jsonArray);

    StorageSystemsModel *const q;

    Connector *m_connector = nullptr;
    bool m_cached = true;

    RequestStatus m_status = RequestStatus::NoRequest;
    ErrorCode m_error = ErrorCode::NoError;
    QString m_errorString;

    QPointer<ApiRequest> m_request;

    QVector<StorageSystem> m_data;
};

QString StorageSystemsModelPrivate::cachePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/qalphacloud_storagesystems.json");
}

void StorageSystemsModelPrivate::setStatus(RequestStatus status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT q->statusChanged(status);
    }
}

void StorageSystemsModelPrivate::setError(ErrorCode error)
{
    if (m_error != error) {
        m_error = error;
        Q_EMIT q->errorChanged(error);
    }
}

void StorageSystemsModelPrivate::setErrorString(const QString &errorString)
{
    if (m_errorString != errorString) {
        m_errorString = errorString;
        Q_EMIT q->errorStringChanged(errorString);
    }
}

void StorageSystemsModelPrivate::processApiResult(const QJsonArray &jsonArray)
{
    bool dirty = false;

    // Check whether the data actually changed.
    // TODO proper delta update of the model.
    if (m_data.count() != jsonArray.count()) {
        dirty = true;
    } else {
        for (int i = 0; i < jsonArray.count(); ++i) {
            const QJsonObject newJson = jsonArray.at(i).toObject();
            const QJsonObject oldJson = m_data.at(i).json;

            if (newJson != oldJson) {
                dirty = true;
                break;
            }
        }
    }

    if (dirty) {
        const QString oldPrimarySerialNumber = q->primarySerialNumber();

        q->beginResetModel();

        m_data.clear();

        m_data.reserve(jsonArray.count());

        for (const QJsonValue &systemValue : jsonArray) {
            m_data << StorageSystem::fromJson(systemValue.toObject());
        }

        q->endResetModel();

        if (oldPrimarySerialNumber != q->primarySerialNumber()) {
            Q_EMIT q->primarySerialNumberChanged(q->primarySerialNumber());
        }
    }

    setStatus(QAlphaCloud::RequestStatus::Finished);
}

bool StorageSystemsModelPrivate::loadFromCache()
{
    const QString path = cachePath();

    QFile cacheFile(path);
    if (!cacheFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Not a warning, cache may just not exist.
        qCDebug(QALPHACLOUD_LOG) << "Failed to open StorageSystemsModel cache" << path << "for reading" << cacheFile.errorString();
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(cacheFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(QALPHACLOUD_LOG) << "Failed to parse StorageSystemsModel cache" << error.errorString();
        return false;
    }

    processApiResult(doc.array());
    qCDebug(QALPHACLOUD_LOG) << "Loaded StorageSystemsModel cache from" << path;
    return true;
}

bool StorageSystemsModelPrivate::writeToCache(const QJsonArray &jsonArray)
{
    const QString path = cachePath();

    // Probably don't bother with QSaveFile.
    QFile cacheFile(path);
    if (!cacheFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(QALPHACLOUD_LOG) << "Failed to open StorageSystemsModel cache" << path << "for writing" << cacheFile.errorString();
        return false;
    }

    const QByteArray data = QJsonDocument(jsonArray).toJson(QJsonDocument::Compact);
    if (cacheFile.write(data) != data.size()) {
        qCWarning(QALPHACLOUD_LOG) << "Failed to write StorageSystemsModel cache data";
        return false;
    }

    qCDebug(QALPHACLOUD_LOG) << "Cached StorageSystemsModel to " << path;
    return true;
}

StorageSystemsModel::StorageSystemsModel(QObject *parent)
    : StorageSystemsModel(nullptr, parent)
{
}

StorageSystemsModel::StorageSystemsModel(Connector *connector, QObject *parent)
    : QAbstractListModel(parent)
    , d(std::make_unique<StorageSystemsModelPrivate>(this))
{
    setConnector(connector);

    connect(this, &StorageSystemsModel::rowsInserted, this, &StorageSystemsModel::countChanged);
    connect(this, &StorageSystemsModel::rowsRemoved, this, &StorageSystemsModel::countChanged);
    connect(this, &StorageSystemsModel::modelReset, this, &StorageSystemsModel::countChanged);

    // Give the creator a chance to disable caching.
    QMetaObject::invokeMethod(
        this,
        [this] {
            if (d->m_cached) {
                d->loadFromCache();
            }
        },
        Qt::QueuedConnection);
}

StorageSystemsModel::~StorageSystemsModel() = default;

Connector *StorageSystemsModel::connector() const
{
    return d->m_connector;
}

void StorageSystemsModel::setConnector(Connector *connector)
{
    if (d->m_connector == connector) {
        return;
    }

    d->m_connector = connector;
    // Additional setup goes here, if any.
    Q_EMIT connectorChanged(connector);
}

bool StorageSystemsModel::cached() const
{
    return d->m_cached;
}

void StorageSystemsModel::setCached(bool cached)
{
    if (d->m_cached == cached) {
        return;
    }

    d->m_cached = cached;
    Q_EMIT cachedChanged(cached);
}

RequestStatus StorageSystemsModel::status() const
{
    return d->m_status;
}

QString StorageSystemsModel::primarySerialNumber() const
{
    if (d->m_data.isEmpty()) {
        return {};
    }

    return d->m_data.first().serialNumber;
}

QAlphaCloud::ErrorCode StorageSystemsModel::error() const
{
    return d->m_error;
}

QString StorageSystemsModel::errorString() const
{
    return d->m_errorString;
}

int StorageSystemsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->m_data.count();
}

QVariant StorageSystemsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const auto &item = d->m_data.at(index.row());

    switch (static_cast<Roles>(role)) {
    case Roles::SerialNumber:
        return item.serialNumber;
    case Roles::Status:
        // NOTE: enum class returned to QML like this turn into an "object"
        // and don't strictly compare to an enum referenced by name ("int"),
        // i.e. you can't use them in a switch and neither can you do ===. :-(
        return QVariant::fromValue(item.status);
        // return static_cast<int>(item.status);
    case Roles::InverterModel:
        return item.inverterModel;
    case Roles::InverterPower:
        return QVariant::fromValue(item.inverterPower);
    case Roles::BatteryModel:
        return item.batteryModel;
    case Roles::BatteryGrossCapacity:
        return QVariant::fromValue(item.grossBatteryCapacity);
    case Roles::BatteryRemainingCapacity:
        return QVariant::fromValue(item.remainingBatteryCapacity);
    case Roles::BatteryUsableCapacity:
        return QVariant::fromValue(item.usableBatteryCapacity);
    case Roles::PhotovoltaicPower:
        return QVariant::fromValue(item.photovoltaicPower);
    case Roles::RawJson:
        return item.json;
    }

    return {};
}

QHash<int, QByteArray> StorageSystemsModel::roleNames() const
{
    return Utils::roleNamesFromEnum(QMetaEnum::fromType<Roles>());
}

bool StorageSystemsModel::reload()
{
    if (!d->m_connector) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load StorageSystems without a connector";
        return false;
    }

    if (d->m_request) {
        qCDebug(QALPHACLOUD_LOG) << "Cancelling StorageSystems request in-flight";
        d->m_request->abort();
        d->m_request = nullptr;
    }

    auto *request = new ApiRequest(d->m_connector, ApiRequest::EndPoint::EssList, this);

    connect(request, &ApiRequest::errorOccurred, this, [this, request] {
        d->setError(request->error());
        d->setErrorString(request->errorString());
        d->setStatus(QAlphaCloud::RequestStatus::Error);
    });

    connect(request, &ApiRequest::result, this, [this, request] {
        const QJsonArray jsonArray = request->data().toArray();

        d->processApiResult(jsonArray);

        if (d->m_cached) {
            d->writeToCache(jsonArray);
        }
    });

    const bool ok = request->send();

    if (ok) {
        d->m_request = request;

        d->setStatus(QAlphaCloud::RequestStatus::Loading);
    }

    return ok;
}

} // namespace QAlphaCloud
