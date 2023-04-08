/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "configuration.h"

#include "config-alphacloud.h"
#include "qalphacloud_log.h"

#include <QSettings>
#include <QStandardPaths>

namespace QAlphaCloud
{

static int g_defaultTimeout = 30000;

class ConfigurationPrivate
{
public:
    QUrl apiUrl;
    QString appId;
    QString appSecret;
    int requestTimeout = g_defaultTimeout;
};

Configuration::Configuration(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ConfigurationPrivate>())
{
    resetApiUrl();
}

Configuration::~Configuration() = default;

Configuration *Configuration::defaultConfiguration(QObject *parent)
{
    auto *configuration = new Configuration(parent);
    configuration->loadDefault();
    return configuration;
}

QString Configuration::defaultConfigurationPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/qalphacloud.ini");
}

QUrl Configuration::apiUrl() const
{
    return d->apiUrl;
}

void Configuration::setApiUrl(const QUrl &apiUrl)
{
    if (d->apiUrl == apiUrl) {
        return;
    }

    const bool oldValid = valid();

    d->apiUrl = apiUrl;
    Q_EMIT apiUrlChanged(apiUrl);

    if (oldValid != valid()) {
        Q_EMIT validChanged(valid());
    }
}

void Configuration::resetApiUrl()
{
    setApiUrl(QUrl(QStringLiteral(API_URL)));
}

QString Configuration::appId() const
{
    return d->appId;
}

void Configuration::setAppId(const QString &appId)
{
    if (d->appId == appId) {
        return;
    }

    const bool oldValid = valid();

    d->appId = appId;
    Q_EMIT appIdChanged(appId);

    if (oldValid != valid()) {
        Q_EMIT validChanged(valid());
    }
}

QString Configuration::appSecret() const
{
    return d->appSecret;
}

void Configuration::setAppSecret(const QString &appSecret)
{
    if (d->appSecret == appSecret) {
        return;
    }

    const bool oldValid = valid();

    d->appSecret = appSecret;
    Q_EMIT appSecretChanged(appSecret);

    if (oldValid != valid()) {
        Q_EMIT validChanged(valid());
    }
}

int Configuration::requestTimeout() const
{
    return d->requestTimeout;
}

void Configuration::setRequestTimeout(int requestTimeout)
{
    if (d->requestTimeout == requestTimeout) {
        return;
    }

    if (requestTimeout < 0) {
        return;
    }

    d->requestTimeout = requestTimeout;
    Q_EMIT requestTimeoutChanged(requestTimeout);
}

void Configuration::resetRequestTimeout()
{
    setRequestTimeout(g_defaultTimeout);
}

bool Configuration::valid() const
{
    return d->apiUrl.isValid() && !d->appId.isEmpty() && !d->appSecret.isEmpty();
}

bool Configuration::loadFromFile(const QString &path)
{
    QSettings settings(path, QSettings::IniFormat);
    return loadFromSettings(&settings);
}

bool Configuration::loadFromSettings(QSettings *settings)
{
    if (!settings) {
        return false;
    }

    if (settings->status() != QSettings::NoError) {
        qCWarning(QALPHACLOUD_LOG) << "Failed to load configuration" << settings->fileName() << settings->status();
        return false;
    }

    qCDebug(QALPHACLOUD_LOG) << "Reading configuration from" << settings->fileName();

    const QUrl defaultUrl = QUrl(QStringLiteral(API_URL));

    QUrl apiUrl = settings->value(QStringLiteral("Api/ApiUrl"), defaultUrl).toUrl();
    if (!apiUrl.isValid()) {
        apiUrl = defaultUrl;
    }
    const QString appId = settings->value(QStringLiteral("Api/AppId"), QString()).toString();
    const QString appSecret = settings->value(QStringLiteral("Api/AppSecret"), QString()).toString();

    bool ok;
    int timeout = settings->value(QStringLiteral("Api/Timeout"), g_defaultTimeout).toInt(&ok);
    if (!ok) {
        timeout = g_defaultTimeout;
    }

    setApiUrl(apiUrl);
    setAppId(appId);
    setAppSecret(appSecret);
    setRequestTimeout(timeout);

    return valid();
}

bool Configuration::loadDefault()
{
    return loadFromFile(defaultConfigurationPath());
}

} // namespace QAlphaCloud
