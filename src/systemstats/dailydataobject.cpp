/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dailydataobject.h"

#include <QDate>

#include <algorithm>
#include <cmath>

#include <QAlphaCloud/OneDateEnergy>
#include <QAlphaCloud/QAlphaCloud>

#include <systemstats/SensorContainer.h>
#include <systemstats/SensorObject.h>
#include <systemstats/SensorProperty.h>

#include "config-alphacloud.h"

using namespace QAlphaCloud;

DailyDataObject::DailyDataObject(QAlphaCloud::Connector *connector, const QString &serialNumber, KSysGuard::SensorContainer *parent)
    : SensorObject(serialNumber + QLatin1String("_daily"), parent)
    , m_dailyData(new OneDateEnergy(connector, serialNumber, QDate::currentDate(), this))
{
    // TODO Investigate how often this updates. OneDateEnergyModel is in 5 minute intervals.
    m_rateLimitTimer.setInterval(1 * 60 * 1000); // 1 min.
    m_rateLimitTimer.setSingleShot(true);
    connect(&m_rateLimitTimer, &QTimer::timeout, this, [this] {
        if (m_updatePending) {
            m_updatePending = false;
            reload();
        }
    });

    // Photovoltaic energy production:
    m_photovoltaicProperty = new KSysGuard::SensorProperty(QStringLiteral("photovoltaic"), tr("Photovoltaic Energy"), 0, this);
    m_photovoltaicProperty->setShortName(tr("Photovoltaic"));
    m_photovoltaicProperty->setDescription(tr("Amount of photovoltiac energy that has been produced today."));
    m_photovoltaicProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_photovoltaicProperty->setVariantType(QVariant::Int);
    m_photovoltaicProperty->setMin(0);
    connect(m_photovoltaicProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

    // Total load:
    m_totalLoadProperty = new KSysGuard::SensorProperty(QStringLiteral("totalLoad"), tr("Total Load"), 0, this);
    m_totalLoadProperty->setShortName(tr("Total"));
    m_totalLoadProperty->setDescription(tr("Total amount of energy consumed today."));
    m_totalLoadProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_totalLoadProperty->setVariantType(QVariant::Int);
    m_totalLoadProperty->setMin(0);
    connect(m_totalLoadProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

    // Power input from grid:
    m_inputProperty = new KSysGuard::SensorProperty(QStringLiteral("input"), tr("Energy Input"), 0, this);
    m_inputProperty->setShortName(tr("Input"));
    m_inputProperty->setDescription(tr("Amount of energy that has input from the grid today."));
    m_inputProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_inputProperty->setVariantType(QVariant::Int);
    m_inputProperty->setMin(0);
    connect(m_inputProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

    // Power output to grid:
    m_outputProperty = new KSysGuard::SensorProperty(QStringLiteral("output"), tr("Energy Output"), 0, this);
    m_outputProperty->setShortName(tr("Output"));
    m_outputProperty->setDescription(tr("Amount of energy that has output to the grid today."));
    m_outputProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_outputProperty->setVariantType(QVariant::Int);
    m_outputProperty->setMin(0);
    connect(m_outputProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

    // Battery charge:
    m_chargeProperty = new KSysGuard::SensorProperty(QStringLiteral("charge"), tr("Battery Charge"), 0, this);
    m_chargeProperty->setShortName(tr("Charge"));
    m_chargeProperty->setDescription(tr("Amount of energy that has been charged into the battery today."));
    m_chargeProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_chargeProperty->setVariantType(QVariant::Int);
    m_chargeProperty->setMin(0);
    connect(m_chargeProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

    // Battery discharge:
    m_dischargeProperty = new KSysGuard::SensorProperty(QStringLiteral("discharge"), tr("Battery Discharge"), 0, this);
    m_dischargeProperty->setShortName(tr("Discharge"));
    m_dischargeProperty->setDescription(tr("Amount of energy that has been discharged from the battery today."));
    m_dischargeProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_dischargeProperty->setVariantType(QVariant::Int);
    m_dischargeProperty->setMin(0);
    connect(m_dischargeProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

    // Battery charge:
    m_gridChargeProperty = new KSysGuard::SensorProperty(QStringLiteral("gridCharge"), tr("Grid Charge"), 0, this);
    // m_gridChargeProperty->setShortName(tr("Grid Charge"));
    m_gridChargeProperty->setDescription(tr("Amount of energy that has been charged into the battery from the grid today."));
    m_gridChargeProperty->setUnit(KSysGuard::Unit::UnitWattHour);
    m_gridChargeProperty->setVariantType(QVariant::Int);
    m_gridChargeProperty->setMin(0);
    connect(m_gridChargeProperty, &KSysGuard::SensorProperty::subscribedChanged, this, &DailyDataObject::update);

#if PRESENTATION_BUILD
    //: Sensor object name with daily data
    setName(tr("Daily"));
#else
    //: Sensor object name with daily data
    setName(tr("%1 (Daily)").arg(serialNumber));
#endif

    // Update once initially but make sure the subscriptions have been processed.
    // TODO check if this actually works.
    QMetaObject::invokeMethod(this, &DailyDataObject::update, Qt::QueuedConnection);
}

// Update all values in lock-step when ksystemstats asks us to rather than updating them
// when the relevant property change is emitted.
void DailyDataObject::update()
{
    reload();

    if (!m_dailyData->valid()) {
        return;
    }

    m_photovoltaicProperty->setValue(m_dailyData->photovoltaic());
    m_totalLoadProperty->setValue(m_dailyData->totalLoad());
    m_inputProperty->setValue(m_dailyData->input());
    m_outputProperty->setValue(m_dailyData->output());
    m_chargeProperty->setValue(m_dailyData->charge());
    m_dischargeProperty->setValue(m_dailyData->discharge());
    m_gridChargeProperty->setValue(m_dailyData->gridCharge());
}

void DailyDataObject::reload()
{
    if (!m_photovoltaicProperty->isSubscribed() && !m_totalLoadProperty->isSubscribed() && !m_inputProperty->isSubscribed() && !m_outputProperty->isSubscribed()
        && !m_chargeProperty->isSubscribed() && !m_dischargeProperty->isSubscribed() && !m_gridChargeProperty->isSubscribed()) {
        return;
    }

    if (m_rateLimitTimer.isActive()) {
        m_updatePending = true;
        return;
    }

    m_dailyData->resetDate();
    m_dailyData->reload();

    m_rateLimitTimer.start();
}
