/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QDate>
#include <QJsonValue>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QString>

#include <variant>

#include "qalphacloud.h"

class QTimer;

namespace QAlphaCloud
{

class Connector;

class ApiRequest : public QObject
{
    Q_OBJECT

public:
    enum class EndPoint {
        NoEndPoint = 0,
        EssList,
        LastPowerData,
        OneDayPowerBySn,
        OneDateEnergyBySn,
    };
    Q_ENUM(EndPoint)

    ApiRequest(Connector *connector, EndPoint endPoint, QObject *parent = nullptr);
    ~ApiRequest() override;

    QString sysSn() const;
    void setSysSn(const QString &sysSn);

    QDate queryDate() const;
    void setQueryDate(const QDate &date);

    QAlphaCloud::ErrorCode error() const;
    QString errorString() const;

    QJsonValue data() const;

    bool send();

    void abort();

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
     * This is emitted when the API request completes successfully.
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

    void aborted();

private:
    QPointer<QNetworkReply> m_reply;

    Connector *m_connector = nullptr;
    EndPoint m_endPoint = EndPoint::NoEndPoint;

    QString m_sysSn;
    QDate m_queryDate;

    QAlphaCloud::ErrorCode m_error = QAlphaCloud::ErrorCode::NoError;
    QString m_errorString;
    QJsonValue m_data;

    QTimer *m_timeoutTimer = nullptr;
};

} // namespace QAlphaCloud
