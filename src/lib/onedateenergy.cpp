/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "onedateenergy.h"

#include "apirequest_p.h"
#include "qalphacloud_log.h"
#include "utils_p.h"

#include <QHash>
#include <QPointer>

#include <cmath>

namespace QAlphaCloud
{

class OneDateEnergyPrivate
{
public:
    explicit OneDateEnergyPrivate(OneDateEnergy *q);

    void setStatus(RequestStatus status);
    void setError(ErrorCode error);
    void setErrorString(const QString &errorString);

    void processApiResult(const QJsonObject &json);

    OneDateEnergy *const q;

    // TODO QPointer?
    Connector *m_connector = nullptr;
    QString m_serialNumber;
    QDate m_date;
    bool m_cached = true;

    int m_photovoltaic = 0; // epv
    int m_input = 0; // eInput
    int m_output = 0; // eOutput
    int m_charge = 0; // eCharge
    int m_discharge = 0; // eDischarge
    int m_gridCharge = 0; // eGridCharge

    QJsonObject m_json;
    RequestStatus m_status = RequestStatus::NoRequest;
    ErrorCode m_error = ErrorCode::NoError;
    QString m_errorString;

    bool m_valid = false;

    QPointer<ApiRequest> m_request;

    QHash<QDate, QJsonObject> m_cache;
};

OneDateEnergyPrivate::OneDateEnergyPrivate(OneDateEnergy *q)
    : q(q)
{
}

void OneDateEnergyPrivate::setStatus(RequestStatus status)
{
    if (m_status != status) {
        m_status = status;
        Q_EMIT q->statusChanged(status);
    }
}

void OneDateEnergyPrivate::setError(ErrorCode error)
{
    if (m_error != error) {
        m_error = error;
        Q_EMIT q->errorChanged(error);
    }
}

void OneDateEnergyPrivate::setErrorString(const QString &errorString)
{
    if (m_errorString != errorString) {
        m_errorString = errorString;
        Q_EMIT q->errorStringChanged(errorString);
    }
}

void OneDateEnergyPrivate::processApiResult(const QJsonObject &json)
{
    bool valid = false;

    const auto oldTotalLoad = q->totalLoad();

    const auto photovoltaicValue = json.value(QStringLiteral("epv"));
    const auto photovoltaicKwh = photovoltaicValue.toDouble();
    const int photovoltaic = static_cast<int>(std::round(photovoltaicKwh * 1000));
    Utils::updateField(m_photovoltaic, photovoltaic, q, &OneDateEnergy::photovoltaicChanged);
    valid = valid || (!photovoltaicValue.isUndefined() && !photovoltaicValue.isNull());

    const auto inputValue = json.value(QStringLiteral("eInput"));
    const auto inputKwh = inputValue.toDouble();
    const int input = static_cast<int>(std::round(inputKwh * 1000));
    Utils::updateField(m_input, input, q, &OneDateEnergy::inputChanged);
    valid = valid || (!inputValue.isUndefined() && !inputValue.isNull());

    const auto outputValue = json.value(QStringLiteral("eOutput"));
    const auto outputKwh = outputValue.toDouble();
    const int output = static_cast<int>(std::round(outputKwh * 1000));
    Utils::updateField(m_output, output, q, &OneDateEnergy::outputChanged);
    valid = valid || (!outputValue.isUndefined() && !outputValue.isNull());

    const auto chargeValue = json.value(QStringLiteral("eCharge"));
    const auto chargeKwh = chargeValue.toDouble();
    const int charge = static_cast<int>(std::round(chargeKwh * 1000));
    Utils::updateField(m_charge, charge, q, &OneDateEnergy::chargeChanged);
    valid = valid || (!chargeValue.isUndefined() && !chargeValue.isNull());

    const auto dischargeValue = json.value(QStringLiteral("eDischarge"));
    const auto dischargeKwh = dischargeValue.toDouble();
    const int discharge = static_cast<int>(std::round(dischargeKwh * 1000));
    Utils::updateField(m_discharge, discharge, q, &OneDateEnergy::dischargeChanged);
    valid = valid || (!dischargeValue.isUndefined() && !dischargeValue.isNull());

    // TODO eCharge / eDischarge

    const auto gridChargeValue = json.value(QStringLiteral("eGridCharge"));
    const auto gridChargeKwh = gridChargeValue.toDouble();
    const int gridCharge = static_cast<int>(std::round(gridChargeKwh * 1000));
    Utils::updateField(m_gridCharge, gridCharge, q, &OneDateEnergy::gridChargeChanged);
    valid = valid || (!gridChargeValue.isUndefined() && !gridChargeValue.isNull());

    // TODO eChargingPile

    if (oldTotalLoad != q->totalLoad()) {
        Q_EMIT q->totalLoadChanged(q->totalLoad());
    }

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

OneDateEnergy::OneDateEnergy(QObject *parent)
    : OneDateEnergy(nullptr, QString(), QDate::currentDate(), parent)
{
}

OneDateEnergy::OneDateEnergy(Connector *connector, const QString &serialNumber, const QDate &date, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<OneDateEnergyPrivate>(this))
{
    setConnector(connector);

    d->m_serialNumber = serialNumber;
    d->m_cache.clear();
    d->m_date = date;
}

OneDateEnergy::~OneDateEnergy() = default;

Connector *OneDateEnergy::connector() const
{
    return d->m_connector;
}

void OneDateEnergy::setConnector(Connector *connector)
{
    if (d->m_connector == connector) {
        return;
    }

    d->m_connector = connector;
    d->m_cache.clear();
    reset();
    Q_EMIT connectorChanged(connector);
}

QString OneDateEnergy::serialNumber() const
{
    return d->m_serialNumber;
}

void OneDateEnergy::setSerialNumber(const QString &serialNumber)
{
    if (d->m_serialNumber == serialNumber) {
        return;
    }

    d->m_serialNumber = serialNumber;
    d->m_cache.clear();
    reset();
    Q_EMIT serialNumberChanged(serialNumber);
}

QDate OneDateEnergy::date() const
{
    return d->m_date;
}

void OneDateEnergy::setDate(const QDate &date)
{
    if (d->m_date == date) {
        return;
    }

    d->m_date = date;
    reset();
    Q_EMIT dateChanged(date);
}

void OneDateEnergy::resetDate()
{
    setDate(QDate::currentDate());
}

bool OneDateEnergy::cached() const
{
    return d->m_cached;
}

void OneDateEnergy::setCached(bool cached)
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

int OneDateEnergy::totalLoad() const
{
    return d->m_photovoltaic + d->m_discharge + d->m_input - d->m_output - d->m_charge;
}

int OneDateEnergy::photovoltaic() const
{
    return d->m_photovoltaic;
}

int OneDateEnergy::input() const
{
    return d->m_input;
}

int OneDateEnergy::output() const
{
    return d->m_output;
}

int OneDateEnergy::charge() const
{
    return d->m_charge;
}

int OneDateEnergy::discharge() const
{
    return d->m_discharge;
}

int OneDateEnergy::gridCharge() const
{
    return d->m_gridCharge;
}

QJsonObject OneDateEnergy::rawJson() const
{
    return d->m_json;
}

bool OneDateEnergy::valid() const
{
    return d->m_valid;
}

RequestStatus OneDateEnergy::status() const
{
    return d->m_status;
}

ErrorCode OneDateEnergy::error() const
{
    return d->m_error;
}

QString OneDateEnergy::errorString() const
{
    return d->m_errorString;
}

bool OneDateEnergy::reload()
{
    if (!d->m_connector) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load OneDateEnergy without a connector";
        return false;
    }

    if (d->m_serialNumber.isEmpty()) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load OneDateEnergy without a serial number";
        return false;
    }

    const QDate date = d->m_date;
    if (!date.isValid()) {
        qCWarning(QALPHACLOUD_LOG) << "Cannot load OneDateEnergy without a valid date";
        return false;
    }

    if (d->m_request) {
        qCDebug(QALPHACLOUD_LOG) << "Cancelling OneDateEnergy request in-flight";
        d->m_request->abort();
        d->m_request = nullptr;
    }

    const auto cachedData = d->m_cache.value(date);
    if (!cachedData.isEmpty()) {
        d->processApiResult(cachedData);
        return true;
    }

    auto *request = new ApiRequest(d->m_connector, ApiRequest::EndPoint::OneDateEnergyBySn, this);
    request->setSysSn(d->m_serialNumber);
    request->setQueryDate(date);

    connect(request, &ApiRequest::errorOccurred, this, [this, request] {
        d->setError(request->error());
        d->setErrorString(request->errorString());
        d->setStatus(RequestStatus::Error);
    });

    connect(request, &ApiRequest::result, this, [this, request, date] {
        const QJsonObject json = request->data().toObject();

        d->processApiResult(json);

        // Don't cache today's data as it will gain new data as the day progresses.
        // Also don't cache if there is no valid data.
        if (d->m_cached && d->m_valid && date != QDate::currentDate()) {
            d->m_cache.insert(date, json);
        }
    });

    const bool ok = request->send();

    if (ok) {
        d->m_request = request;

        d->setStatus(QAlphaCloud::RequestStatus::Loading);
    }

    return ok;
}

bool OneDateEnergy::forceReload()
{
    d->m_cache.clear();
    return reload();
}

void OneDateEnergy::reset()
{
    d->processApiResult(QJsonObject());
    if (d->m_request) {
        d->m_request->abort();
        d->m_request = nullptr;
    }
    d->setStatus(RequestStatus::NoRequest);
}

} // namespace QAlphaCloud
