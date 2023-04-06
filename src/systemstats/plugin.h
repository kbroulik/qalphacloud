/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <systemstats/SensorPlugin.h>

namespace KSysGuard
{
class SensorContainer;
}

namespace QAlphaCloud
{
class Connector;
}

class QNetworkAccessManager;
class QModelIndex;

class LiveDataObject;
class SystemObject;

class SystemStatsPlugin : public KSysGuard::SensorPlugin
{
    Q_OBJECT

public:
    SystemStatsPlugin(QObject *parent, const QVariantList &args);
    ~SystemStatsPlugin() override;

    void update() override;

    QString providerName() const override
    {
        return QStringLiteral("qalphacloud");
    }

private:
    void addStorageSystem(const QModelIndex &index);
    void removeStorageSystem(const QModelIndex &index);

    KSysGuard::SensorContainer *m_container;

    QNetworkAccessManager *m_networkAccessManager;

    QAlphaCloud::Connector *m_connector;

    QHash<QString, SystemObject *> m_systems;
    QHash<QString, LiveDataObject *> m_liveData;
};
