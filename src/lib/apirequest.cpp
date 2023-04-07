/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "apirequest_p.h"

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
#include <QTimer>
#include <QUrlQuery>

namespace QAlphaCloud
{

ApiRequest::ApiRequest(Connector *connector, EndPoint endPoint, QObject *parent)
    : QObject(parent)
    , m_connector(connector)
    , m_endPoint(endPoint)
{
    Q_ASSERT(m_endPoint != EndPoint::NoEndPoint);

    connect(this, &ApiRequest::finished, this, &ApiRequest::deleteLater);
}

ApiRequest::~ApiRequest() = default;

QString ApiRequest::sysSn() const
{
    return m_sysSn;
}

void ApiRequest::setSysSn(const QString &sysSn)
{
    m_sysSn = sysSn;
}

QDate ApiRequest::queryDate() const
{
    return m_queryDate;
}

void ApiRequest::setQueryDate(const QDate &date)
{
    m_queryDate = date;
}

QAlphaCloud::ErrorCode ApiRequest::error() const
{
    return m_error;
}

QString ApiRequest::errorString() const
{
    return m_errorString;
}

QJsonValue ApiRequest::data() const
{
    return m_data;
}

bool ApiRequest::send()
{
    if (!m_connector->networkAccessManager()) {
        qCCritical(QALPHACLOUD_LOG) << "Cannot send request without QNetworkAccessManager";
        return false;
    }

    auto *configuration = m_connector->configuration();
    if (!configuration) {
        qCCritical(QALPHACLOUD_LOG) << "Cannot send request on a Connector without no configuration";
        return false;
    }

    if (!configuration->valid()) {
        qCCritical(QALPHACLOUD_LOG) << "Cannot send request on a Connector with an invalid configuration";
        return false;
    }

    if (configuration->requestTimeout() > 0) {
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, [this] {
            qCWarning(QALPHACLOUD_LOG) << "Request timed out after" << m_timeoutTimer->interval() << "ms";
            // TODO set error to QNetworkReply::TimeoutError
            abort();
        });
        // TODO Qt6 QNetworkReply::requestSent
        m_timeoutTimer->start(configuration->requestTimeout());
    }

    // Calculate Header fields (appId, timeStamp, sign).
    const QByteArray timeStampStr = QByteArray::number(QDateTime::currentSecsSinceEpoch(), 'f', 0);

    const QByteArray appId = configuration->appId().toUtf8(); // toLatin1?
    const QByteArray secret = configuration->appSecret().toUtf8();

    const QByteArray sign = appId + secret + timeStampStr;
    const QByteArray hashedSign = QCryptographicHash::hash(sign, QCryptographicHash::Sha512).toHex();

    // Generate URL.
    QUrl url = configuration->apiUrl();

    QMetaEnum me = QMetaEnum::fromType<EndPoint>();
    const QString endPointName = QString::fromUtf8(me.valueToKey(static_cast<int>(m_endPoint)));

    url.setPath(QDir::cleanPath(url.path() + QLatin1String("/get") + endPointName));
    // url.setPath(url.path() + QLatin1String("/get") + endPointName);

    // Add any additional request parameters.
    QUrlQuery query(url);
    if (!m_sysSn.isEmpty()) {
        query.addQueryItem(QStringLiteral("sysSn"), m_sysSn);
    }
    if (m_queryDate.isValid()) {
        query.addQueryItem(QStringLiteral("queryDate"), m_queryDate.toString(QStringLiteral("yyyy-MM-dd")));
    }
    url.setQuery(query);

    QNetworkRequest request(url);
    // request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    // TODO allow spoofing stuff like User Agent.
    request.setHeader(QNetworkRequest::ContentLengthHeader, 0);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    request.setRawHeader(QByteArrayLiteral("appId"), appId);
    request.setRawHeader(QByteArrayLiteral("timeStamp"), timeStampStr);
    request.setRawHeader(QByteArrayLiteral("sign"), hashedSign);

    qCDebug(QALPHACLOUD_LOG) << "Sending API request to" << url;

    m_error = QAlphaCloud::ErrorCode::NoError;
    m_errorString.clear();
    m_data = QJsonObject();

    auto *reply = m_connector->networkAccessManager()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_timeoutTimer) {
            m_timeoutTimer->stop();
        }

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::OperationCanceledError) {
                qCDebug(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "was canceled";
            } else {
                qCWarning(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "failed with network error" << reply->errorString();
            }
            m_error = static_cast<QAlphaCloud::ErrorCode>(reply->error());
            m_errorString = reply->errorString();
            Q_EMIT errorOccurred();
        } else {
            int code = -1;
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);

            if (error.error != QJsonParseError::NoError) {
                m_error = ErrorCode::JsonParseError;
                m_errorString = error.errorString();
            } else if (!jsonDocument.isObject()) {
                m_error = ErrorCode::UnexpectedJsonDataError;
            } else {
                const QJsonObject jsonObject = jsonDocument.object();
                if (jsonObject.isEmpty()) {
                    m_error = ErrorCode::EmptyJsonObjectError;
                } else {
                    code = jsonObject.value(QStringLiteral("code")).toInt();
                    if (code != 200) {
                        m_error = static_cast<QAlphaCloud::ErrorCode>(code);
                    }
                    m_errorString = jsonObject.value(QStringLiteral("msg")).toString();

                    m_data = jsonObject.value(QStringLiteral("data"));
                }
            }

            if (m_error != QAlphaCloud::ErrorCode::NoError) {
                qCWarning(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "failed with API error" << m_error << code << m_errorString;
                Q_EMIT errorOccurred();
            } else {
                qCDebug(QALPHACLOUD_LOG) << "API request for endpoint" << reply->url() << "succeeded";
                Q_EMIT result();
            }
        }

        Q_EMIT finished();
    });
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

    m_reply = reply;

    return true;
}

void ApiRequest::abort()
{
    if (m_reply) {
        m_reply->abort();
        m_reply = nullptr;
    }
}

} // namespace QAlphaCloud
