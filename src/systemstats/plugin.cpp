/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "plugin.h"
#include "livedataobject.h"
#include "systemobject.h"

#include <QNetworkAccessManager>

#include <KPluginFactory>

#include <systemstats/AggregateSensor.h>
#include <systemstats/SensorContainer.h>
#include <systemstats/SensorObject.h>
#include <systemstats/SensorProperty.h>

#include <QAlphaCloud/Connector>
#include <QAlphaCloud/StorageSystemsModel>

K_PLUGIN_CLASS_WITH_JSON(SystemStatsPlugin, "metadata.json")

using namespace QAlphaCloud;

static QString serialNumberFromIndex(const QModelIndex &index)
{
    return index.data(static_cast<int>(StorageSystemsModel::Roles::SerialNumber)).toString();
}

SystemStatsPlugin::SystemStatsPlugin(QObject *parent, const QVariantList &args)
    : SensorPlugin(parent, args)
    , m_container(new KSysGuard::SensorContainer("qalphacloud", tr("Alpha Cloud"), this))
    , m_networkAccessManager(new QNetworkAccessManager(this))
    , m_connector(new QAlphaCloud::Connector(QAlphaCloud::Configuration::defaultConfiguration(), this))
{
    m_networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    m_connector->setNetworkAccessManager(m_networkAccessManager);

    auto *storageSystems = new StorageSystemsModel(m_connector, this);

    connect(storageSystems, &StorageSystemsModel::modelAboutToBeReset, this, [this] {
        qDeleteAll(m_systems);
        m_systems.clear();
        qDeleteAll(m_liveData);
        m_liveData.clear();
    });

    connect(storageSystems, &StorageSystemsModel::modelReset, this, [this, storageSystems] {
        for (int i = 0; i < storageSystems->rowCount(); ++i) {
            const QModelIndex index = storageSystems->index(i, 0);
            if (!index.isValid()) { // shouldn't happen.
                continue;
            }

            addStorageSystem(index);
        }
    });

    connect(storageSystems, &StorageSystemsModel::rowsInserted, this, [this, storageSystems](const QModelIndex &parent, int from, int to) {
        for (int i = from; i <= to; ++i) {
            const QModelIndex index = storageSystems->index(i, 0, parent);
            if (!index.isValid()) { // shouldn't happen.
                continue;
            }

            addStorageSystem(index);
        }
    });
    connect(storageSystems, &StorageSystemsModel::rowsAboutToBeRemoved, this, [this, storageSystems](const QModelIndex &parent, int from, int to) {
        for (int i = from; i <= to; ++i) {
            const QModelIndex index = storageSystems->index(i, 0, parent);
            if (!index.isValid()) {
                continue;
            }

            removeStorageSystem(index);
        }
    });
    connect(storageSystems,
            &StorageSystemsModel::dataChanged,
            this,
            [this, storageSystems](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
                Q_UNUSED(roles);

                for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
                    const QModelIndex index = storageSystems->index(i, 0);
                    const QString serialNumber = serialNumberFromIndex(index);

                    if (auto *storageSystem = m_systems.value(serialNumber)) {
                        storageSystem->update(index);
                    }
                }
            });

    storageSystems->reload();
}

SystemStatsPlugin::~SystemStatsPlugin() = default;

void SystemStatsPlugin::addStorageSystem(const QModelIndex &index)
{
    const QString serialNumber = serialNumberFromIndex(index);
    if (m_systems.contains(serialNumber)) { // shouldn't happen.
        return;
    }

    auto *storageSystem = new SystemObject(serialNumber, m_container);
    storageSystem->update(index);
    m_systems.insert(serialNumber, storageSystem);

    auto *liveData = new LiveDataObject(m_connector, serialNumber, m_container);
    m_liveData.insert(serialNumber, liveData);
}

void SystemStatsPlugin::removeStorageSystem(const QModelIndex &index)
{
    const QString serialNumber = serialNumberFromIndex(index);
    // deleteLater?
    delete m_systems.take(serialNumber);
    delete m_liveData.take(serialNumber);
}

void SystemStatsPlugin::update()
{
    // TODO reload storage systems model occasionally

    for (auto *liveData : qAsConst(m_liveData)) {
        liveData->update();
    }
}

#include "plugin.moc"
