/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "lastpowerdata.h"

#include "apirequest.h"
#include "qalphacloud_log.h"
#include "utils_p.h"

#include <QPointer>

namespace QAlphaCloud
{

class LastPowerDataPrivate
{
public:
    LastPowerDataPrivate(LastPowerData *q);

    void setStatus(RequestStatus status);
    void setError(ErrorCode error);
    void setErrorString(const QString &errorString);

    void processApiResult(const QJsonObject &json);

    LastPowerData *const q;

    // TODO QPointer?
    Connector *m_connector = nullptr;
    QString m_serialNumber;

    int m_photovoltaicPower = 0;
    int m_currentLoad = 0;
    int m_gridPower = 0;
    int m_batteryPower = 0;

    qreal m_batterySoc = 0.0;

    QJsonObject m_json;
    RequestStatus m_status = RequestStatus::NoRequest;
    ErrorCode m_error = ErrorCode::NoError;
    QString m_errorString;

    bool m_valid = false;

    QPointer<ApiRequest> m_request;
};

LastPowerDataPrivate::LastPowerDataPrivate(LastPowerData *q)
    : q(q)
{
}

void LastPowerDataPrivate::setStatus(RequestStatus status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT q->statusChanged(status);
    }
}

void LastPowerDataPrivate::setError(ErrorCode error)
{
    if (m_error != error) {
        m_error = error;
        Q_EMIT q->errorChanged(error);
    }
}

void LastPowerDataPrivate::setErrorString(const QString &errorString)
{
    if (m_errorString != errorString) {
        m_errorString = errorString;
        Q_EMIT q->errorStringChanged(errorString);
    }
}

void LastPowerDataPrivate::processApiResult(const QJsonObject &json)
{
    bool valid = false;

    const auto photovoltaicPower = json.value(QStringLiteral("ppv")).toInt();
    Utils::updateField(m_photovoltaicPower, photovoltaicPower, q, &LastPowerData::photovoltaicPowerChanged);

    const auto currentLoadValue = json.value(QStringLiteral("pload"));
    const auto currentLoad = currentLoadValue.toInt();
    Utils::updateField(m_currentLoad, currentLoad, q, &LastPowerData::currentLoadChanged);
    valid = valid || (!currentLoadValue.isUndefined() && !currentLoadValue.isNull());

    const auto batterySocValue = json.value(QStringLiteral("soc"));
    const auto batterySoc = batterySocValue.toDouble();
    Utils::updateField(m_batterySoc, batterySoc, q, &LastPowerData::batterySocChanged);
    valid = valid || (!batterySocValue.isUndefined() && !batterySocValue.isNull());

    const auto gridPowerValue = json.value(QStringLiteral("pgrid"));
    const auto gridPower = gridPowerValue.toInt();
    Utils::updateField(m_gridPower, gridPower, q, &LastPowerData::gridPowerChanged);
    valid = valid || (!gridPowerValue.isUndefined() && !gridPowerValue.isNull());

    const auto batteryPowerValue = json.value(QStringLiteral("pbat"));
    const auto batteryPower = batteryPowerValue.toInt();
    Utils::updateField(m_batteryPower, batteryPower, q, &LastPowerData::batteryPowerChanged);
    valid = valid || (!batteryPowerValue.isUndefined() && !batteryPowerValue.isNull());

    // TODO pev

    if (m_json != json) {
        m_json = json;
        Q_EMIT q->rawJsonChanged();
    }

    if (m_valid != valid) {
        m_valid = valid;
        Q_EMIT q->validChanged(valid);
    }

    setStatus(RequestStatus::Finished);
}

LastPowerData::LastPowerData(QObject *parent)
    : LastPowerData(nullptr, QString(), parent)
{
}

LastPowerData::LastPowerData(Connector *connector, const QString &serialNumber, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<LastPowerDataPrivate>(this))
{
    setConnector(connector);

    d->m_serialNumber = serialNumber;
}

LastPowerData::~LastPowerData() = default;

Connector *LastPowerData::connector() const
{
    return d->m_connector;
}

void LastPowerData::setConnector(Connector *connector)
{
    if (d->m_connector == connector) {
        return;
    }

    d->m_connector = connector;
    reset();
    Q_EMIT connectorChanged(connector);
}

QString LastPowerData::serialNumber() const
{
    return d->m_serialNumber;
}

void LastPowerData::setSerialNumber(const QString &serialNumber)
{
    if (d->m_serialNumber == serialNumber) {
        return;
    }

    d->m_serialNumber = serialNumber;
    reset();
    Q_EMIT serialNumberChanged(serialNumber);
}

int LastPowerData::photovoltaicPower() const
{
    return d->m_photovoltaicPower;
}

int LastPowerData::currentLoad() const
{
    return d->m_currentLoad;
}

int LastPowerData::gridPower() const
{
    return d->m_gridPower;
}

int LastPowerData::batteryPower() const
{
    return d->m_batteryPower;
}

qreal LastPowerData::batterySoc() const
{
    return d->m_batterySoc;
}

QJsonObject LastPowerData::rawJson() const
{
    return d->m_json;
}

bool LastPowerData::valid() const
{
    return d->m_valid;
}

RequestStatus LastPowerData::status() const
{
    return d->m_status;
}

QAlphaCloud::ErrorCode LastPowerData::error() const
{
    return d->m_error;
}

QString LastPowerData::errorString() const
{
    return d->m_errorString;
}

bool LastPowerData::reload()
{
    if (!d->m_connector) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load LastPowerData without a connector";
        return false;
    }

    if (d->m_serialNumber.isEmpty()) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load LastPowerData without a serial number";
        return false;
    }

    if (d->m_request) {
        qCDebug(QALPHACLOUD_LOG) << "Cancelling LastPowerData request in-flight";
        d->m_request->abort();
        d->m_request = nullptr;
    }

    d->setStatus(RequestStatus::Loading);

    auto *request = new ApiRequest(d->m_connector, ApiRequest::EndPoint::LastPowerData, this);
    request->setSysSn(d->m_serialNumber);

    connect(request, &ApiRequest::errorOccurred, this, [this, request] {
        d->setError(request->error());
        d->setErrorString(request->errorString());
        d->setStatus(RequestStatus::Error);
    });

    connect(request, &ApiRequest::result, this, [this, request] {
        const QJsonObject json = request->data().toObject();

        d->processApiResult(json);
    });

    request->send();

    d->m_request = request;

    return true;
}

void LastPowerData::reset()
{
    if (d->m_request) {
        d->m_request->abort();
        d->m_request = nullptr;
    }
    d->processApiResult(QJsonObject());
    d->setStatus(RequestStatus::NoRequest);
}

} // namespace QAlphaCloud
