/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QDate>
#include <QJsonValue>
#include <QObject>
#include <QString>
#include <QUrlQuery>

#include "qalphacloud.h"

#include <memory>

namespace QAlphaCloud
{

class ApiRequestPrivate;
class Connector;

/**
 * @brief API request job
 *
 * This class handles all network communication to the API.
 * It implements a job which will self-delete when finished.
 *
 * Normally, you don't need to use this class directly but it can
 * be handy to issue API requests for which this library provides
 * no wrapper class.
 */
class Q_DECL_EXPORT ApiRequest : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief The API endpoints
     */
    struct EndPoint {
        static constexpr QLatin1String EssList{"getEssList"};
        static constexpr QLatin1String LastPowerData{"getLastPowerData"};
        static constexpr QLatin1String OneDayPowerBySn{"getOneDayPowerBySn"};
        static constexpr QLatin1String OneDateEnergyBySn{"getOneDateEnergyBySn"};
    };

    /**
     * @brief Create an API request
     * @param connector The Connector to use, must not be null.
     * @param parent The parent, if wanted.
     */
    explicit ApiRequest(Connector *connector, QObject *parent = nullptr);
    /**
     * @brief Create an API request
     * @param connector The Connector to use, must not be null.
     * @param endPoint The API endpoint to talk to.
     * @param parent The parent, if wanted.
     */
    ApiRequest(Connector *connector, const QString &endPoint, QObject *parent = nullptr);
    /**
     * @brief Destroys the API request
     */
    ~ApiRequest() override;

    /**
     * @brief The API endpoint to use
     */
    QString endPoint() const;
    /**
     * @brief Set the API endpoint to use
     *
     * This can be any one of EndPoint or any string.
     * A trailing slash is not required.
     */
    void setEndPoint(const QString &endPoint);

    /**
     * @brief The storage system serial number
     */
    QString sysSn() const;
    /**
     * @brief Set the storage system serial number
     *
     * For APIs that apply to a specific storage system.
     */
    void setSysSn(const QString &sysSn);

    /**
     * @brief The query date
     */
    QDate queryDate() const;
    /**
     * @brief Set the query date
     *
     * For APIs that return data for a given date.
     */
    void setQueryDate(const QDate &date);

    /**
     * @brief Custom query arguments
     */
    QUrlQuery query() const;
    /**
     * @brief Set custom query arguments
     *
     * In case sysSn and queryDate are not sufficient for a given
     * API call, arbitrary query parameters can be provided.
     */
    void setQuery(const QUrlQuery &query);

    /**
     * @brief Whether the job auto-deletes when finished
     */
    bool autoDelete() const;
    /**
     * @brief Set whether to auto-delete the job when finished
     *
     * Default is true.
     */
    void setAutoDelete(bool autoDelete);

    /**
     * @brief The error, if any.
     */
    QAlphaCloud::ErrorCode error() const;
    /**
     * @brief The error string, if any.
     *
     * @note Not every error code has an errorString associated with it.
     */
    QString errorString() const;

    /**
     * @brief The data the API returned.
     *
     * This can be either a JSON object or JSON array (or null)
     * depending on the API.
     */
    QJsonValue data() const;

    /**
     * @brief Send the event
     *
     * @return Whether the request was sent.
     */
    Q_INVOKABLE bool send();

    /**
     * @brief Abort the event
     */
    Q_INVOKABLE void abort();

Q_SIGNALS:
    /**
     * @brief Emitted when the request finished.
     *
     * This is emitted regardless of whether the request succeeded or failed.
     */
    void finished();

    /**
     * @brief Emitted when an API result received.
     *
     * This is emitted when the API request completed successfully.
     */
    void result();
    /**
     * @brief Emitted when an error occurred.
     *
     * This can either be a network error or an error returned by the API.
     *
     * @sa networkError()
     * @sa apiError()
     */
    void errorOccurred();

    // TODO void aborted(); ?

private:
    std::unique_ptr<ApiRequestPrivate> const d;
};

} // namespace QAlphaCloud
