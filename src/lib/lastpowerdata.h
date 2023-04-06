/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

#include <memory>

#include "connector.h"
#include "qalphacloud.h"

namespace QAlphaCloud
{

class LastPowerDataPrivate;

/**
 * @brief Live Power Data
 *
 * Provides live information.
 *
 * Wraps the @c /getLastPowerData API endpoint.
 */
class Q_DECL_EXPORT LastPowerData : public QObject
{
    Q_OBJECT

    /**
     * @brief The connector to use
     */
    Q_PROPERTY(QAlphaCloud::Connector *connector READ connector WRITE setConnector NOTIFY connectorChanged REQUIRED)

    /**
     * @brief The serial number
     *
     * The serial number of the storage system whose data should be queried.
     */
    Q_PROPERTY(QString serialNumber READ serialNumber WRITE setSerialNumber NOTIFY serialNumberChanged REQUIRED)

    /**
     * @brief Photovoltaic power in W.
     */
    Q_PROPERTY(int photovoltaicPower READ photovoltaicPower NOTIFY photovoltaicPowerChanged)
    /**
     * @brief Current load in W.
     */
    Q_PROPERTY(int currentLoad READ currentLoad NOTIFY currentLoadChanged)
    // TODO add enum for FeedingToGrid / ConsumingGridPower
    // and/or add gridFeed/gridUse properties.
    /**
     * @brief Grid power in W.
     *
     * * When this is negative, power is being fed into the grid.
     * * When this is positive, power is drawn from the grid.
     */
    Q_PROPERTY(int gridPower READ gridPower NOTIFY gridPowerChanged)
    // TODO add enum for NoCharge / Charging / Discharging / FullyCharged
    /**
     * @brief Battery power in W.
     */
    Q_PROPERTY(int batteryPower READ batteryPower NOTIFY batteryPowerChanged)
    // TODO pev

    /**
     * @brief Battery state of charge in %.
     */
    Q_PROPERTY(qreal batterySoc READ batterySoc NOTIFY batterySocChanged)

    /**
     * @brief Raw JSON
     *
     * The raw JSON returned by the API, useful for extracting data
     * that isn't provided through the API yet.
     */
    Q_PROPERTY(QJsonObject rawJson READ rawJson NOTIFY rawJsonChanged)

    /**
     * @brief Whether this object contains data
     *
     * This is independent of the status. The status can be QAlphaCloud::Error
     * when a subsequent request fails but any data isn't cleared unless
     * new data is loaded successfully.
     */
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged STORED false)

    /**
     * @brief The current request status
     */
    Q_PROPERTY(QAlphaCloud::RequestStatus status READ status NOTIFY statusChanged)

    /**
     * @brief The error, if any
     *
     * There can still be valid data in this object from a previous
     * successful request.
     */
    Q_PROPERTY(QAlphaCloud::ErrorCode error READ error NOTIFY errorChanged)
    /**
     * @brief The error string, if any
     *
     * @note Not every error code has an errorString associated with it.
     */
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    /**
     * @brief Creates a LastPowerData instance
     * @param parent The owner
     *
     * @note A connector and serialNumber must be set before requests can be made.
     */
    explicit LastPowerData(QObject *parent = nullptr);
    /**
     * @brief Creates a LastPowerData intsance
     * @param connector The connector
     * @param serialNumber The serial number of the storage system whose data should be queried
     * @param parent The owner
     */
    LastPowerData(QAlphaCloud::Connector *connector, const QString &serialNumber, QObject *parent = nullptr);
    ~LastPowerData() override;

    Q_REQUIRED_RESULT QAlphaCloud::Connector *connector() const;
    void setConnector(QAlphaCloud::Connector *connector);
    Q_SIGNAL void connectorChanged(QAlphaCloud::Connector *connector);

    Q_REQUIRED_RESULT QString serialNumber() const;
    void setSerialNumber(const QString &serialNumber);
    Q_SIGNAL void serialNumberChanged(const QString &serialNumber);

    Q_REQUIRED_RESULT int photovoltaicPower() const;
    Q_SIGNAL void photovoltaicPowerChanged(int photovoltaicPower);

    Q_REQUIRED_RESULT int currentLoad() const;
    Q_SIGNAL void currentLoadChanged(int currentLoad);

    Q_REQUIRED_RESULT int gridPower() const;
    Q_SIGNAL void gridPowerChanged(int gridPower);

    Q_REQUIRED_RESULT int batteryPower() const;
    Q_SIGNAL void batteryPowerChanged(int batteryPower);

    Q_REQUIRED_RESULT qreal batterySoc() const;
    Q_SIGNAL void batterySocChanged(qreal batterySoc);

    Q_REQUIRED_RESULT QJsonObject rawJson() const;
    Q_SIGNAL void rawJsonChanged();

    Q_REQUIRED_RESULT bool valid() const;
    Q_SIGNAL void validChanged(bool valid);

    Q_REQUIRED_RESULT QAlphaCloud::RequestStatus status() const;
    Q_SIGNAL void statusChanged(QAlphaCloud::RequestStatus status);

    QAlphaCloud::ErrorCode error() const;
    Q_SIGNAL void errorChanged(QAlphaCloud::ErrorCode error);

    QString errorString() const;
    Q_SIGNAL void errorStringChanged(const QString &errorString);

public Q_SLOTS:

    /**
     * @brief (Re)load data
     *
     * In QML, this is done automatically on component completion if the
     * @a active property (not documented here) is true, which is the default.
     * @return Whether the request was sent.
     *
     * @note You must set a connector and serialNumber before requests can be sent.
     *
     * @note When the request fails, the current data is not cleared.
     */
    bool reload();
    /**
     * @brief Reset object
     *
     * This clears all data and resets the object back to its initial state.
     */
    void reset();

private:
    friend class LastPowerDataPrivate;
    std::unique_ptr<LastPowerDataPrivate> const d;
};

} // namespace QAlphaCloud
