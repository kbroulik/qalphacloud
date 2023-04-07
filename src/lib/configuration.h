/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <memory>

#include "qalphacloud.h"

#include "qalphacloud_export.h"

class QIODevice;
class QSettings;

namespace QAlphaCloud
{

class ConfigurationPrivate;

/**
 * @brief API configuration
 *
 * This class provides the configuration of your API connector, i.e. the URL, App ID and secret.
 *
 * Additionally, the request timeout can be configured.
 *
 * The default configuration can be placed in an INI file `~/.config/Broulik/QAlphaloud`:
 * ```
 * [Api]
 * AppId=alpha...
 * AppSecret=...
 * ```
 */
class QALPHACLOUD_EXPORT Configuration : public QObject
{
    Q_OBJECT

    /**
     * @brief The URL to send requests to
     *
     * Default is the `API_URL` configure option, which by default is the official API endpoint.
     */
    Q_PROPERTY(QUrl apiUrl READ apiUrl WRITE setApiUrl RESET resetApiUrl NOTIFY apiUrlChanged)

    /**
     * @brief The App ID
     *
     * The application ID registered on the API. Typically starts with `alpha...`
     */
    Q_PROPERTY(QString appId READ appId WRITE setAppId NOTIFY appIdChanged)

    /**
     * @brief The API secret
     */
    Q_PROPERTY(QString appSecret READ appSecret WRITE setAppSecret NOTIFY appSecretChanged)

    /**
     * @brief Request timeout (ms).
     *
     * Time out requests automatically after ms.
     *
     * Default is 30,000 (30 seconds).
     */
    Q_PROPERTY(int requestTimeout READ requestTimeout WRITE setRequestTimeout RESET resetRequestTimeout NOTIFY requestTimeoutChanged)

    /**
     * @brief Whether the configuration is valid
     *
     * You cannot make requests with an invalid configuration.
     */
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
    /**
     * @brief Create a configuration
     *
     * It will be empty and invalid.
     *
     * @param parent The owner.
     */
    explicit Configuration(QObject *parent = nullptr);
    ~Configuration() override;

    /**
     * @brief Create a default configuration.
     *
     * This is typically what you want to pass to the @c Connector.
     * It will be read from the configuration file.
     */
    static Configuration *defaultConfiguration(QObject *parent = nullptr);

    Q_REQUIRED_RESULT QUrl apiUrl() const;
    void setApiUrl(const QUrl &apiUrl);
    /**
     * @brief Reset API URL to the default
     */
    void resetApiUrl();
    Q_SIGNAL void apiUrlChanged(const QUrl &apiUrl);

    Q_REQUIRED_RESULT QString appId() const;
    void setAppId(const QString &appId);
    Q_SIGNAL void appIdChanged(const QString &appId);

    Q_REQUIRED_RESULT QString appSecret() const;
    void setAppSecret(const QString &appSecret);
    Q_SIGNAL void appSecretChanged(const QString &appSecret);

    Q_REQUIRED_RESULT int requestTimeout() const;
    void setRequestTimeout(int requestTimeout);
    void resetRequestTimeout();
    Q_SIGNAL void requestTimeoutChanged(int requestTimeout);

    bool valid() const;
    Q_SIGNAL void validChanged(bool valid);

public Q_SLOTS:

    /**
     * @brief Load configuration from file
     * @param path The file path
     * @return Whether the configuration could be loaded and is valid.
     */
    bool loadFromFile(const QString &path);
    // TODO QSettings doesn't  support reading from QIODevice :-(
    // bool loadFromDevice(QIODevice *device);
    /**
     * @brief Load configuration from QSettings
     * @param settings The QSettings object
     * @return Whether the configuration could be loaded and is valid.
     */
    bool loadFromSettings(QSettings *settings);
    /**
     * @brief Load the default configuration
     * It will be loaded from the configuration file.
     * @return Whether the configuration could be loaded and is valid.
     */
    bool loadDefault();

private:
    std::unique_ptr<ConfigurationPrivate> const d;
};

} // namespace QAlphaCloud
