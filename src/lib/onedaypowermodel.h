/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QDate>

#include <memory>

#include "connector.h"
#include "qalphacloud.h"
#include "qalphacloud_export.h"

namespace QAlphaCloud
{

class OneDayPowerModelPrivate;

/**
 * @brief Historic power data for a day
 *
 * Provides historic power data for a given day.
 *
 * Wraps the @c /getOneDayPower API endpoint.
 */
class QALPHACLOUD_EXPORT OneDayPowerModel : public QAbstractListModel
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
     * @brief The earliest date in the model
     *
     * Useful for determining the range of the X axis on a plot.
     */
    Q_PROPERTY(QDateTime fromDateTime READ fromDateTime NOTIFY fromDateTimeChanged)
    /**
     * @brief The latest date in the model
     *
     * Useful for determining the range of the X axis on a plot.
     */
    Q_PROPERTY(QDateTime toDateTime READ toDateTime NOTIFY toDateTimeChanged)

    /**
     * @brief Peak photovoltaic production in W
     *
     * Useful for determining the range of the Y axis on a plot.
     */
    Q_PROPERTY(int peakPhotovoltaic READ peakPhotovoltaic NOTIFY peakPhotovoltaicChanged)
    /**
     * @brief Peak load in W
     *
     * Useful for determining the range of the Y axis on a plot.
     */
    Q_PROPERTY(int peakLoad READ peakLoad NOTIFY peakLoadChanged)
    /**
     * @brief Peak grid feed in W
     *
     * Useful for determining the range of the Y axis on a plot.
     */
    Q_PROPERTY(int peakGridFeed READ peakGridFeed NOTIFY peakGridFeedChanged)
    /**
     * @brief Peak grid charge in W
     *
     * Useful for determining the range of the Y axis on a plot.
     */
    Q_PROPERTY(int peakGridCharge READ peakGridCharge NOTIFY peakGridChargeChanged)

    /**
     * @brief The number of items in the model
     */
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

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
    explicit OneDayPowerModel(QObject *parent = nullptr);
    /**
     * @brief Creates a OneDateEnergy intsance
     * @param connector The connector
     * @param serialNumber The serial number of the storage system whose data should be queried
     * @param date The date for which to query the data
     * @param parent The owner
     */
    OneDayPowerModel(Connector *connector, const QString &serialNumber, const QDate &date, QObject *parent = nullptr);
    ~OneDayPowerModel() override;

    /**
     * @brief The model roles
     */
    enum class Roles {
        PhotovoltaicEnergy = Qt::UserRole, ///< The photovoltaic production in W (int)
        CurrentLoad, ///< The current load in W (int)
        GridFeed, ///< The current grid feed in W (int)
        GridCharge, ///< The current grid charge in W (int)
        BatterySoc, ///< The battery state of charge in per-cent % (qreal)
        UploadTime, ///< When this entry was recorded. (QDateTime)
        RawJson = Qt::UserRole
            + 99, ///< Returns the raw JSON data for this system. Useful for extracting properties that aren't provided through the model (QJsonObject).
    };
    Q_ENUM(Roles)

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

    Q_REQUIRED_RESULT QDateTime fromDateTime() const;
    Q_SIGNAL void fromDateTimeChanged(const QDateTime &fromDateTime);

    Q_REQUIRED_RESULT QDateTime toDateTime() const;
    Q_SIGNAL void toDateTimeChanged(const QDateTime &toDateTime);

    Q_REQUIRED_RESULT int peakPhotovoltaic() const;
    Q_SIGNAL void peakPhotovoltaicChanged(int peakPhotovoltaicChanged);

    Q_REQUIRED_RESULT int peakLoad() const;
    Q_SIGNAL void peakLoadChanged(int peakLoad);

    Q_REQUIRED_RESULT int peakGridFeed() const;
    Q_SIGNAL void peakGridFeedChanged(int peakGridFeed);

    Q_REQUIRED_RESULT int peakGridCharge() const;
    Q_SIGNAL void peakGridChargeChanged(int peakGridCharge);

    QAlphaCloud::RequestStatus status() const;
    Q_SIGNAL void statusChanged(QAlphaCloud::RequestStatus status);

    QAlphaCloud::ErrorCode error() const;
    Q_SIGNAL void errorChanged(QAlphaCloud::ErrorCode error);

    QString errorString() const;
    Q_SIGNAL void errorStringChanged(const QString &errorString);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

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

Q_SIGNALS:
    void countChanged();

private:
    friend class OneDayPowerModelPrivate;
    std::unique_ptr<OneDayPowerModelPrivate> const d;
};

} // namespace QAlphaCloud
