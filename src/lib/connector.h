/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <memory>

#include "configuration.h"
#include "qalphacloud.h"

#include "qalphacloud_export.h"

class QNetworkAccessManager;

namespace QAlphaCloud
{

class ConnectorPrivate;

/**
 * @brief API Connection
 *
 * This class represents an API connection with the given configuration.
 * It is required by all request classes.
 *
 * @note You need to set a @a QNetworkAccessManager on this object
 * in order to send requests.
 *
 * In QML, the @a QNetworkAccessManager is automatically assigned from the
 * current QQmlEngine on component completion.
 */
class QALPHACLOUD_EXPORT Connector : public QObject
{
    Q_OBJECT

    /**
     * @brief The configuration to use
     */
    Q_PROPERTY(QAlphaCloud::Configuration *configuration READ configuration WRITE setConfiguration NOTIFY configurationChanged)

    /**
     * @brief Whether this connector is valid
     *
     * This means that it has a valid configuration and has a QNetworkAccessManager
     */
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

public:
    explicit Connector(QObject *parent = nullptr);
    explicit Connector(Configuration *configuration, QObject *parent = nullptr);
    ~Connector() override;

    Q_REQUIRED_RESULT Configuration *configuration() const;
    void setConfiguration(Configuration *configuration);
    Q_SIGNAL void configurationChanged(QAlphaCloud::Configuration *configuration);

    Q_REQUIRED_RESULT bool valid() const;
    Q_SIGNAL void validChanged(bool valid);

    Q_REQUIRED_RESULT QNetworkAccessManager *networkAccessManager() const;
    /**
     * @brief Set the QNetworkAccessManager
     *
     * It will be used for all network requests. Without it, no requests can be sent.
     * @param networkAccessManager The QNetworkAccessManager to use.
     *
     * In QML, the @a QNetworkAccessManager is automatically assigned from the
     * current QQmlEngine on component completion, unless one has already been
     * assigned manually before.
     */
    void setNetworkAccessManager(QNetworkAccessManager *networkAccessManager);

private:
    friend class ConnectorPrivate;
    std::unique_ptr<ConnectorPrivate> const d;
};

} // namespace QAlphaCloud
