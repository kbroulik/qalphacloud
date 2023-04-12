/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "livedataobject.h"

#include <QDate>
#include <QNetworkAccessManager>

#include <KPluginFactory>

#include <algorithm>

#include <QAlphaCloud/LastPowerData>
#include <QAlphaCloud/QAlphaCloud>
//#include <QAlphaCloud/StorageSystemsModel>

#include <systemstats/SensorContainer.h>
#include <systemstats/SensorObject.h>
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
            update();
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
    // This is in Watt.
    connect(m_liveData, &QAlphaCloud::LastPowerData::photovoltaicPowerChanged, this, [this](int power) {
        m_photovoltaicPowerProperty->setValue(power);
    });
    m_photovoltaicPowerProperty->setValue(m_liveData->photovoltaicPower());

    // Current consumer load:
    m_currentLoadProperty = new KSysGuard::SensorProperty(QStringLiteral("currentLoad"), tr("Current Load"), 0, this);
    m_currentLoadProperty->setShortName(tr("Load"));
    m_currentLoadProperty->setDescription(tr("Amount of power that the household currently consumes"));
    m_currentLoadProperty->setUnit(KSysGuard::Unit::UnitWatt);
    m_currentLoadProperty->setVariantType(QVariant::Int);
    // TODO inverter power or something
    // m_currentLoadProperty->setMax(photovoltaicDesignPower * 1000);
    connect(m_currentLoadProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);
    connect(m_liveData, &QAlphaCloud::LastPowerData::currentLoadChanged, this, [this](int currentLoad) {
        // NOTE: This sometimes gets negative.
        m_currentLoadProperty->setValue(std::max(0, currentLoad));
    });
    m_currentLoadProperty->setValue(m_liveData->currentLoad());

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
    // TODO set value

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

    connect(m_liveData, &QAlphaCloud::LastPowerData::gridPowerChanged, this, &LiveDataObject::updateGridPower);

    // Battery state of charge:
    m_batterySocProperty = new KSysGuard::SensorProperty(QStringLiteral("batterySoc"), tr("State of Charge"), 0, this);
    m_batterySocProperty->setShortName(tr("SOC"));
    m_batterySocProperty->setDescription(tr("Percentage the battery is charged"));
    m_batterySocProperty->setUnit(KSysGuard::Unit::UnitPercent);
    m_batterySocProperty->setVariantType(QVariant::Double);
    m_batterySocProperty->setMax(100.0);
    m_batterySocProperty->setMin(0.0);
    connect(m_batterySocProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &LiveDataObject::update);
    connect(m_liveData, &QAlphaCloud::LastPowerData::batterySocChanged, this, [this](qreal soc) {
        m_batterySocProperty->setValue(soc);
    });

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

    connect(m_liveData, &QAlphaCloud::LastPowerData::batteryPowerChanged, this, &LiveDataObject::updateBatteryPower);

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

void LiveDataObject::updateGridPower(int power)
{
    // gridPower is negative when feeding power, positive when consuming power.
    // This is a convenience sensor only showing when power is fed into the grid.
    int gridFeed = 0;
    int gridConsumption = 0;

    if (power < 0) {
        gridFeed = -power;
    } else if (power > 0) {
        gridConsumption = power;
    }

    m_gridFeedProperty->setValue(gridFeed);
    m_gridConsumptionProperty->setValue(gridConsumption);
}

void LiveDataObject::updateBatteryPower(int power)
{
    int batteryCharge = 0;
    int batteryDischarge = 0;

    if (power < 0) {
        batteryCharge = -power;
    } else if (power > 0) {
        batteryDischarge = power;
    }

    m_batteryChargeProperty->setValue(batteryCharge);
    m_batteryDischargeProperty->setValue(batteryDischarge);
}

void LiveDataObject::update()
{
    if (!m_photovoltaicPowerProperty->isSubscribed() && !m_currentLoadProperty->isSubscribed() && !m_gridFeedProperty->isSubscribed()
        && !m_gridConsumptionProperty->isSubscribed() && !m_batterySocProperty->isSubscribed() && !m_batteryChargeProperty->isSubscribed()
        && !m_batteryDischargeProperty->isSubscribed()) {
        return;
    }

    if (m_rateLimitTimer.isActive()) {
        m_updatePending = true;
        return;
    }

    m_liveData->reload();
    m_rateLimitTimer.start();
}
