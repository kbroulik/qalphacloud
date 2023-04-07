/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QDate>
#include <QJsonObject>
#include <QObject>
#include <QString>

#include <memory>

#include "connector.h"
#include "qalphacloud.h"
#include "qalphacloud_export.h"

namespace QAlphaCloud
{

class OneDateEnergyPrivate;

/**
 * @brief Cumulative daily energy information.
 *
 * Provides cumulative information about a given date.
 *
 * Wraps the @c /getOneDateEnergy API endpoint.
 */
class QALPHACLOUD_EXPORT OneDateEnergy : public QObject
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
     * @brief The date
     *
     * The date for which to query the data.
     */
    Q_PROPERTY(QDate date READ date WRITE setDate RESET resetDate NOTIFY dateChanged)

    /**
     * @brief Cache data
     *
     * Whether to cache the returned data, default is true.
     *
     * This allows for quicker navigation between dates when they have been loaded
     * once and reduces network traffic.
     *
     * Data from the current day is never cached as data is collected throughout
     * the day.
     */
    Q_PROPERTY(bool cached READ cached WRITE setCached NOTIFY cachedChanged)

    /**
     * @brief Photovoltaic production in Wh.
     */
    Q_PROPERTY(int photovoltaic READ photovoltaic NOTIFY photovoltaicChanged)

    /**
     * @brief Total load in Wh.
     */
    Q_PROPERTY(int totalLoad READ totalLoad NOTIFY totalLoadChanged)
    /**
     * @brief Power input form grid in Wh.
     */
    Q_PROPERTY(int input READ input NOTIFY inputChanged)
    /**
     * @brief Power output to the grid in Wh.
     */
    Q_PROPERTY(int output READ output NOTIFY outputChanged)

    // Q_PROPERTY(int charge READ charge NOTIFY chargeChanged)
    // Q_PROPERTY(int discharge READ discharge NOTIFY dischargeChanged)
    /**
     * @brief Battery charge from grid in Wh.
     *
     * How many Wh have been fed from the grid into the battery.
     */
    Q_PROPERTY(int gridCharge READ gridCharge NOTIFY gridChargeChanged)

    // TODO eChargingPile (EV charger)

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
     * @brief Creates a OneDateEnergy instance
     * @param parent The owner
     *
     * @note A connector, serialNumber, and a date must be set before requests can be made.
     */
    explicit OneDateEnergy(QObject *parent = nullptr);
    /**
     * @brief Creates a OneDateEnergy intsance
     * @param connector The connector
     * @param serialNumber The serial number of the storage system whose data should be queried
     * @param date The date for which to query the data
     * @param parent The owner
     */
    OneDateEnergy(Connector *connector, const QString &serialNumber, const QDate &date, QObject *parent = nullptr);
    ~OneDateEnergy() override;

    Q_REQUIRED_RESULT Connector *connector() const;
    void setConnector(Connector *connector);
    Q_SIGNAL void connectorChanged(Connector *connector);

    Q_REQUIRED_RESULT QString serialNumber() const;
    void setSerialNumber(const QString &serialNumber);
    Q_SIGNAL void serialNumberChanged(const QString &serialNumber);

    Q_REQUIRED_RESULT QDate date() const;
    void setDate(const QDate &date);
    void resetDate();
    Q_SIGNAL void dateChanged(const QDate &date);

    Q_REQUIRED_RESULT bool cached() const;
    void setCached(bool cached);
    Q_SIGNAL void cachedChanged(bool cached);

    Q_REQUIRED_RESULT int totalLoad() const;
    Q_SIGNAL void totalLoadChanged(int totalLoad);

    Q_REQUIRED_RESULT int photovoltaic() const;
    Q_SIGNAL void photovoltaicChanged(int photovoltaic);

    Q_REQUIRED_RESULT int input() const;
    Q_SIGNAL void inputChanged(int input);

    Q_REQUIRED_RESULT int output() const;
    Q_SIGNAL void outputChanged(int output);

    // Q_REQUIRED_RESULT int charge() const;
    // Q_SIGNAL void chargeChanged(int charge);

    // Q_REQUIRED_RESULT int discharge() const;
    // Q_SIGNAL void dischargeChanged(int discharge);

    Q_REQUIRED_RESULT int gridCharge() const;
    Q_SIGNAL void gridChargeChanged(int gridCharge);

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
     * @note You must set a connector, a serialNumber, and a date before requests can be sent.
     *
     * @note When the request fails, the current data is not cleared.
     */
    bool reload();
    /**
     * @brief Force a reload
     *
     * Reloads the data, ignoring the cache.
     *
     * @return Whether the request was sent.
     */
    bool forceReload();
    /**
     * @brief Reset object
     *
     * This clears all data and resets the object back to its initial state.
     */
    void reset();

private:
    std::unique_ptr<OneDateEnergyPrivate> const d;
};

} // namespace QAlphaCloud
