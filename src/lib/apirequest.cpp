/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "apirequest.h"

#include "connector.h"
#include "qalphacloud_log.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QScopeGuard>
#include <QUrlQuery>

namespace QAlphaCloud
{

class ApiRequestPrivate
{
public:
    explicit ApiRequestPrivate(ApiRequest *q)
        : q(q)
    {
    }

    void finalize()
    {
        if (m_autoDelete) {
            q->deleteLater();
        }
    }

    ApiRequest *const q;
    QPointer<QNetworkReply> m_reply;

    Connector *m_connector = nullptr;
    QString m_endPoint;
    bool m_autoDelete = true;

    QString m_sysSn;
    QDate m_queryDate;
    QUrlQuery m_query;

    QAlphaCloud::ErrorCode m_error = QAlphaCloud::ErrorCode::NoError;
    QString m_errorString;
    QJsonValue m_data;
};

ApiRequest::ApiRequest(Connector *connector, QObject *parent)
    : ApiRequest(connector, QString(), parent)
{
}

ApiRequest::ApiRequest(Connector *connector, const QString &endPoint, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ApiRequestPrivate>(this))
{
    Q_ASSERT(connector);
    d->m_connector = connector;
    d->m_endPoint = endPoint;

    connect(this, &ApiRequest::finished, this, [this] {
        d->finalize();
    });
}

ApiRequest::~ApiRequest() = default;

QString ApiRequest::endPoint() const
{
    return d->m_endPoint;
}

void ApiRequest::setEndPoint(const QString &endPoint)
{
    d->m_endPoint = endPoint;
}

QString ApiRequest::sysSn() const
{
    return d->m_sysSn;
}

void ApiRequest::setSysSn(const QString &sysSn)
{
    d->m_sysSn = sysSn;
}

QDate ApiRequest::queryDate() const
{
    return d->m_queryDate;
}

void ApiRequest::setQueryDate(const QDate &date)
{
    d->m_queryDate = date;
}

QUrlQuery ApiRequest::query() const
{
    return d->m_query;
}

void ApiRequest::setQuery(const QUrlQuery &query)
{
    d->m_query = query;
}

bool ApiRequest::autoDelete() const
{
    return d->m_autoDelete;
}

void ApiRequest::setAutoDelete(bool autoDelete)
{
    d->m_autoDelete = autoDelete;
}

QAlphaCloud::ErrorCode ApiRequest::error() const
{
    return d->m_error;
}

QString ApiRequest::errorString() const
{
    return d->m_errorString;
}

QJsonValue ApiRequest::data() const
{
    return d->m_data;
}

bool ApiRequest::send()
{
    auto cleanup = qScopeGuard([this] {
        deleteLater();
    });

    if (!d->m_connector->networkAccessManager()) {
        qCCritical(QALPHACLOUD_LOG) << "Cannot send request without QNetworkAccessManager";
        return false;
    }

    auto *configuration = d->m_connector->configuration();
    if (!configuration) {
        qCCritical(QALPHACLOUD_LOG) << "Cannot send request on a Connector with no configuration";
        return false;
    }

    if (!configuration->valid()) {
        qCCritical(QALPHACLOUD_LOG) << "Cannot send request on a Connector with an invalid configuration";
        return false;
    }

    // Calculate Header fields (appId, timeStamp, sign).
    const QByteArray timeStampStr = QByteArray::number(QDateTime::currentSecsSinceEpoch(), 'f', 0);

    const QByteArray appId = configuration->appId().toUtf8(); // toLatin1?
    const QByteArray secret = configuration->appSecret().toUtf8();

    const QByteArray sign = appId + secret + timeStampStr;
    const QByteArray hashedSign = QCryptographicHash::hash(sign, QCryptographicHash::Sha512).toHex();

    // Generate URL.
    QUrl url = configuration->apiUrl();

    url.setPath(QDir::cleanPath(url.path() + QLatin1Char('/') + d->m_endPoint));

    // Add any additional request parameters.
    QUrlQuery query = d->m_query;
    if (!d->m_sysSn.isEmpty()) {
        query.addQueryItem(QStringLiteral("sysSn"), d->m_sysSn);
    }
    if (d->m_queryDate.isValid()) {
        query.addQueryItem(QStringLiteral("queryDate"), d->m_queryDate.toString(QStringLiteral("yyyy-MM-dd")));
    }
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setTransferTimeout(configuration->requestTimeout());

    // request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    // TODO allow spoofing stuff like User Agent.
    request.setHeader(QNetworkRequest::ContentLengthHeader, 0);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    request.setRawHeader(QByteArrayLiteral("appId"), appId);
    request.setRawHeader(QByteArrayLiteral("timeStamp"), timeStampStr);
    request.setRawHeader(QByteArrayLiteral("sign"), hashedSign);

    qCDebug(QALPHACLOUD_LOG) << "Sending API request to" << url;

    d->m_error = QAlphaCloud::ErrorCode::NoError;
    d->m_errorString.clear();
    d->m_data = QJsonObject();

    auto *reply = d->m_connector->networkAccessManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::OperationCanceledError) {
                qCDebug(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "was canceled";
            } else {
                qCWarning(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "failed with network error" << reply->errorString();
            }
            d->m_error = static_cast<QAlphaCloud::ErrorCode>(reply->error());
            d->m_errorString = reply->errorString();
            Q_EMIT errorOccurred();
        } else {
            int code = -1;
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);

            if (error.error != QJsonParseError::NoError) {
                d->m_error = ErrorCode::JsonParseError;
                d->m_errorString = QAlphaCloud::errorText(d->m_error, error.errorString());
            } else if (!jsonDocument.isObject()) {
                d->m_error = ErrorCode::UnexpectedJsonDataError;
                d->m_errorString = QAlphaCloud::errorText(d->m_error, jsonDocument);
            } else {
                const QJsonObject jsonObject = jsonDocument.object();
                if (jsonObject.isEmpty()) {
                    d->m_error = ErrorCode::EmptyJsonObjectError;
                } else {
                    code = jsonObject.value(QStringLiteral("code")).toInt();
                    if (code != 200) {
                        d->m_error = static_cast<QAlphaCloud::ErrorCode>(code);
                        const QString msg = jsonObject.value(QStringLiteral("msg")).toString();
                        d->m_errorString = QAlphaCloud::errorText(d->m_error, msg);
                    }

                    d->m_data = jsonObject.value(QStringLiteral("data"));
                }
            }

            if (d->m_error != QAlphaCloud::ErrorCode::NoError) {
                qCWarning(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "failed with API error" << d->m_error << code << d->m_errorString;
                Q_EMIT errorOccurred();
            } else {
                qCDebug(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "succeeded";
                Q_EMIT result();
            }
        }

        Q_EMIT finished();
    });
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    d->m_reply = reply;

    cleanup.dismiss();

    return true;
}

void ApiRequest::abort()
{
    if (d->m_reply) {
        d->m_reply->abort();
        d->m_reply = nullptr;
    }

    d->finalize();
}

} // namespace QAlphaCloud
