/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "connector.h"

#include "qalphacloud_log.h"

namespace QAlphaCloud
{

class ConnectorPrivate
{
public:
    Configuration *configuration = nullptr;
    QNetworkAccessManager *networkAccessManager = nullptr;
};

Connector::Connector(QObject *parent)
    : Connector(nullptr, parent)
{
}

Connector::Connector(Configuration *configuration, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<ConnectorPrivate>())
{
    d->configuration = configuration;
    if (d->configuration) {
        d->configuration->setParent(this);
    }
}

Connector::~Connector() = default;

Configuration *Connector::configuration() const
{
    return d->configuration;
}

void Connector::setConfiguration(Configuration *configuration)
{
    if (d->configuration == configuration) {
        return;
    }

    if (d->configuration) {
        disconnect(d->configuration, nullptr, this, nullptr);
    }

    const bool oldValid = valid();
    d->configuration = configuration;

    if (d->configuration) {
        // TODO only emit if it effectively changed.
        connect(d->configuration, &Configuration::validChanged, this, &Connector::validChanged);
    }

    Q_EMIT configurationChanged(configuration);

    if (oldValid != valid()) {
        Q_EMIT validChanged(valid());
    }
}

bool Connector::valid() const
{
    return d->configuration && d->configuration->valid() && d->networkAccessManager != nullptr;
}

QNetworkAccessManager *Connector::networkAccessManager() const
{
    return d->networkAccessManager;
}

void Connector::setNetworkAccessManager(QNetworkAccessManager *networkAccessManager)
{
    if (d->networkAccessManager == networkAccessManager) {
        return;
    }

    const bool oldValid = valid();
    d->networkAccessManager = networkAccessManager;

    if (oldValid != valid()) {
        Q_EMIT validChanged(valid());
    }
}

} // namespace QAlphaCloud
