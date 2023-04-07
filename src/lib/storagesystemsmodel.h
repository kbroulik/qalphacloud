/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QAbstractListModel>

#include <memory>

#include "qalphacloud.h"
#include "qalphacloud_export.h"

#include "connector.h"

namespace QAlphaCloud
{

class StorageSystemsModelPrivate;

/**
 * @brief Storage Systems Model
 *
 * List all storage systems associated with the user.
 *
 * Wraps the @c /getEssList API endpoint.
 */
class QALPHACLOUD_EXPORT StorageSystemsModel : public QAbstractListModel
{
    Q_OBJECT

    /**
     * @brief The connector to use
     */
    Q_PROPERTY(QAlphaCloud::Connector *connector READ connector WRITE setConnector NOTIFY connectorChanged REQUIRED)

    /**
     * @brief The number of items in the model
     */
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

    /**
     * @brief The first serial number in the model
     *
     * This is provided for convenience, so one does not need to
     * deal with QAbstractListModel API for serving the common
     * case of having a single storage system.
     */
    Q_PROPERTY(QString primarySerialNumber READ primarySerialNumber NOTIFY primarySerialNumberChanged)

    /**
     * @brief The current request status
     */
    Q_PROPERTY(QAlphaCloud::RequestStatus status READ status NOTIFY statusChanged)

    /**
     * @brief The error, if any
     *
     * There can still be valid data in the model from a previous
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
     * @brief Creates a StorageSystemsModel instance
     * @param parent The owner
     *
     * @note A connector must be set before requests can be made.
     */
    explicit StorageSystemsModel(QObject *parent = nullptr);
    /**
     * @brief Creates a StorageSystemsModel intsance
     * @param connector The connector
     * @param parent The owner
     */
    explicit StorageSystemsModel(QAlphaCloud::Connector *connector, QObject *parent = nullptr);
    ~StorageSystemsModel() override;

    /**
     * @brief The model roles
     */
    enum class Roles {
        SerialNumber = Qt::UserRole, ///< System serial number (QString).
        Status, ///< Status of the energy management system (QAlphaCloud::SystemStatus).
        InverterModel, ///< Interver model (QString).
        InverterPower, ///< Gross power of the inverter system in W (int).
        BatteryModel, ///< Battery model (QString).
        BatteryGrossCapacity, ///< Gross battery capacity in Wh. Typically there's a capacity buffer that cannot be used (int).
        BatteryRemainingCapacity, ///< Remaining battery capacity in Wh (int).
        BatteryUsableCapacity, ///< Usable battery capacity in per-cent % (qreal).
        PhotovoltaicPower, ///< Gross power provided by the photovoltaic system in W (int).
        RawJson = Qt::UserRole
            + 99, ///< Returns the raw JSON data for this system. Useful for extracting properties that aren't provided through the model (QJsonObject).
    };
    Q_ENUM(Roles)

    Q_REQUIRED_RESULT QAlphaCloud::Connector *connector() const;
    void setConnector(QAlphaCloud::Connector *connector);
    Q_SIGNAL void connectorChanged(QAlphaCloud::Connector *connector);

    QAlphaCloud::RequestStatus status() const;
    Q_SIGNAL void statusChanged(QAlphaCloud::RequestStatus status);

    QString primarySerialNumber() const;
    Q_SIGNAL void primarySerialNumberChanged(const QString &primarySerialNumber);

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
     * @note Contrary to the other objects, this is *not* done automatically
     * in QML.
     *
     * @note You must set a connector before requests can be sent.
     *
     * @note When the request fails, the current data is not cleared.
     */
    bool reload();

Q_SIGNALS:
    void countChanged();

private:
    friend StorageSystemsModelPrivate;
    std::unique_ptr<StorageSystemsModelPrivate> const d;
};

} // namespace QAlphaCloud
