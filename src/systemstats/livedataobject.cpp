/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "livedataobject.h"

#include <algorithm>
#include <cmath>

#include <QAlphaCloud/LastPowerData>
#include <QAlphaCloud/QAlphaCloud>
#include <QAlphaCloud/StorageSystemsModel>

#include <systemstats/SensorContainer.h>
#include <systemstats/SensorProperty.h>

#include "config-alphacloud.h"

using namespace QAlphaCloud;

LiveDataObject::LiveDataObject(QAlphaCloud::Connector *connector, const QString &serialNumber, KSysGuard::SensorContainer *parent)
    : SensorObject(serialNumber + QLatin1String("_live"), parent)
    , m_liveData(new LastPowerData(connector, serialNumber, this))
{
    m_rateLimitTimer.setInterval(10000);
    m_rateLimitTimer.setSingleShot(true);
    connect(&m_rateLimitTimer, &QTimer::timeout, this, [this] {
        if (m_updatePending) {
            m_updatePending = false;
            reload();
        }
    });

    // Photovoltaic power:
    // const int photovoltaicDesignPower =
    // index.data(static_cast<int>(StorageSystemsModel::Roles::PhotovoltaicPower)).toInt();
    m_photovoltaicPowerProperty = new KSysGuard::SensorProperty(QStringLiteral("photovoltaicPower"), tr("Photovoltaic Power"), 0, this);
    m_photovoltaicPowerProperty->setShortName(tr("Photovoltaic"));
    m_photovoltaicPowerProperty->setDescription(tr("Amount of power that is currently supplied by the photovoltaic system"));
    m_photovoltaicPowerProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_photovoltaicPowerProperty->setVariantType(QVariant::Int);
    m_photovoltaicPowerProperty->setMin(0);
    // m_photovoltaicPowerProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_photovoltaicPowerProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Current consumer load:
    m_currentLoadProperty = new KSysGuard::SensorProperty(QStringLiteral("currentLoad"), tr("Current Load"), 0, this);
    m_currentLoadProperty->setShortName(tr("Load"));
    m_currentLoadProperty->setDescription(tr("Amount of power that the household currently consumes"));
    m_currentLoadProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_currentLoadProperty->setVariantType(QVariant::Int);
    // TODO inverter power or something
    // m_currentLoadProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_currentLoadProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Power being fed to the grid:
    m_gridFeedProperty = new KSysGuard::SensorProperty(QStringLiteral("gridFeed"),
                                                       tr("Grid Feed"), // TODO proper English.
                                                       0,
                                                       this);
    m_gridFeedProperty->setShortName(tr("Feed"));
    m_gridFeedProperty->setDescription(tr("Amount of power that is currently being fed into the grid"));
    m_gridFeedProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_gridFeedProperty->setVariantType(QVariant::Int);
    m_gridFeedProperty->setMin(0);
    // TODO inverter power or something
    // m_gridFeedProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_gridFeedProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Poewr being consumed from the grid:
    m_gridConsumptionProperty = new KSysGuard::SensorProperty(QStringLiteral("gridConsumption"), tr("Grid Consumption"), 0, this);
    m_gridConsumptionProperty->setShortName(tr("Consumption"));
    m_gridConsumptionProperty->setDescription(tr("Amount of power that is currently being fed into the grid"));
    m_gridConsumptionProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_gridConsumptionProperty->setVariantType(QVariant::Int);
    m_gridConsumptionProperty->setMin(0);
    // TODO inverter power or something
    // m_gridConsumptionProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_gridConsumptionProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Battery state of charge percent:
    m_batterySocProperty = new KSysGuard::SensorProperty(QStringLiteral("batterySoc"), tr("State of Charge"), 0, this);
    m_batterySocProperty->setShortName(tr("SOC"));
    m_batterySocProperty->setDescription(tr("Percentage the battery is charged"));
    m_batterySocProperty->setUnit(KSysGuard::Unit::UnitPercent);
    m_batterySocProperty->setVariantType(QVariant::Double);
    m_batterySocProperty->setMax(100.0);
    m_batterySocProperty->setMin(0.0);
    connect(m_batterySocProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Battery state of charge Wh:
    m_batteryEnergyProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryEnergy"), tr("Battery Energy"), 0, this);
    m_batteryEnergyProperty->setShortName(tr("Energy"));
    m_batteryEnergyProperty->setDescription(tr("Amount of energy that is currently stored in the battery"));
    m_batteryEnergyProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_batteryEnergyProperty->setVariantType(QVariant::Int);
    // TODO max to battery Usable Capacity
    connect(m_batteryEnergyProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Battery charging rate:
    m_batteryChargeProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryCharge"), tr("Battery Charge"), 0, this);
    m_batteryChargeProperty->setShortName(tr("Charge"));
    m_batteryChargeProperty->setDescription(tr("Amount of power that is currently being fed into the battery"));
    m_batteryChargeProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_batteryChargeProperty->setVariantType(QVariant::Int);
    // TODO inverter power or something
    // m_batteryChargeProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_batteryChargeProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    // Battery discharging rate:
    m_batteryDischargeProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryDischarge"), tr("Battery Discharge"), 0, this);
    m_batteryDischargeProperty->setShortName(tr("Discharge"));
    m_batteryDischargeProperty->setDescription(tr("Amount of power that is currently being drawn from the battery"));
    m_batteryDischargeProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_batteryDischargeProperty->setVariantType(QVariant::Int);
    // TODO inverter power or something
    // m_batteryDischargeProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_batteryDischargeProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

    m_batteryTimeProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryTime"), tr("Remaining Time"), 0, this);
    m_batteryTimeProperty->setShortName(tr("Remaining"));
    m_batteryTimeProperty->setUnit(KSysGuard::Unit::UnitTime);
    m_batteryTimeProperty->setVariantType(QVariant::Int);
    connect(m_batteryTimeProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);

#if PRESENTATION_BUILD
    //: Sensor object name with live data
    setName(tr("Live"));
#else
    //: Sensor object name with live data
    setName(tr("%1 (Live)").arg(serialNumber));
#endif

    // Update once initially but make sure the subscriptions have been processed.
    // TODO check if this actually works.
    QMetaObject::invokeMethod(this, &LiveDataObject::update, Qt::QueuedConnection);
}

// Update all values in lock-step when ksystemstats asks us to rather than updating them
// when the relevant property change is emitted.
void LiveDataObject::update()
{
    reload();

    if (!m_liveData->valid()) {
        return;
    }

    m_photovoltaicPowerProperty->setValue(m_liveData->photovoltaicPower());
    // NOTE: This sometimes gets negative.
    m_currentLoadProperty->setValue(std::max(0, m_liveData->currentLoad()));

    // gridPower is negative when feeding power, positive when consuming power.
    // This is a convenience sensor only showing when power is fed into the grid.
    const int gridPower = m_liveData->gridPower();
    int gridFeed = 0;
    int gridConsumption = 0;

    if (gridPower < 0) {
        gridFeed = -gridPower;
    } else if (gridPower > 0) {
        gridConsumption = gridPower;
    }

    m_gridFeedProperty->setValue(gridFeed);
    m_gridConsumptionProperty->setValue(gridConsumption);

    m_batterySocProperty->setValue(m_liveData->batterySoc());
    const int batteryEnergy = std::round(m_batteryRemainingCapacityWh * m_liveData->batterySoc() / 100.0);
    m_batteryEnergyProperty->setValue(batteryEnergy);

    const int batteryPower = m_liveData->batteryPower();
    int batteryCharge = 0;
    int batteryDischarge = 0;

    if (batteryPower < 0) {
        batteryCharge = -batteryPower;
    } else if (batteryPower > 0) {
        batteryDischarge = batteryPower;
    }

    m_batteryChargeProperty->setValue(batteryCharge);
    m_batteryDischargeProperty->setValue(batteryDischarge);

    if (batteryCharge > 0) {
        m_batteryTimeProperty->setName(tr("Time until full"));

        if (m_batteryRemainingCapacityWh > 0) {
            const int neededCapacity = std::max(0, m_batteryRemainingCapacityWh - batteryEnergy);
            m_batteryTimeProperty->setValue(neededCapacity / qreal(batteryCharge) * 60 * 60);
        }
    } else if (batteryDischarge > 0) {
        m_batteryTimeProperty->setName(tr("Time until empty"));
        m_batteryTimeProperty->setValue(batteryEnergy / qreal(batteryDischarge) * 60 * 60);
    }
}

void LiveDataObject::updateSystem(const QModelIndex &index)
{
    m_batteryRemainingCapacityWh = index.data(static_cast<int>(StorageSystemsModel::Roles::BatteryRemainingCapacity)).toInt();

    m_batteryEnergyProperty->setMax(m_batteryRemainingCapacityWh);
}

void LiveDataObject::reload()
{
    if (!m_photovoltaicPowerProperty->isSubscribed() && !m_currentLoadProperty->isSubscribed() && !m_gridFeedProperty->isSubscribed()
        && !m_gridConsumptionProperty->isSubscribed() && !m_batterySocProperty->isSubscribed() && !m_batteryEnergyProperty->isSubscribed()
        && !m_batteryChargeProperty->isSubscribed() && !m_batteryDischargeProperty->isSubscribed()) {
        return;
    }

    if (m_rateLimitTimer.isActive()) {
        m_updatePending = true;
        return;
    }

    m_liveData->reload();
    m_rateLimitTimer.start();
}
