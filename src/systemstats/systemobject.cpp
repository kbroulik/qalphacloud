/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "systemobject.h"

#include <KPluginFactory>

#include <QAlphaCloud/QAlphaCloud>
#include <QAlphaCloud/StorageSystemsModel>

#include <systemstats/AggregateSensor.h>
#include <systemstats/SensorContainer.h>
#include <systemstats/SensorObject.h>
#include <systemstats/SensorProperty.h>

#include "config-alphacloud.h"

using namespace QAlphaCloud;

SystemObject::SystemObject(const QString &serialNumber,
                           // DO NOT store this QModelIndex. It is only for
                           // initial popuplation of the container.
                           KSysGuard::SensorContainer *parent)
    // TODO strip out dangerous special characters when serial is used as ID.
    // TODO can we use a sub hierarchy?
    : SensorObject(serialNumber + QLatin1String("_system"), parent)
{
    m_serialNumberProperty = new KSysGuard::SensorProperty(QStringLiteral("serialNumber"), tr("Serial Number"), this);
    m_serialNumberProperty->setShortName(tr("Serial"));

    m_inverterModelProperty = new KSysGuard::SensorProperty(QStringLiteral("inverterModel"), tr("Inverter Model"), this);
    m_inverterModelProperty->setShortName(tr("Inverter"));

    m_batteryModelProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryModel"), tr("Battery Model"), this);
    m_batteryModelProperty->setShortName(tr("Battery"));

    m_batteryGrossCapacityProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryGrossCapacity"), tr("Battery Gross Capacity"), this);
    m_batteryGrossCapacityProperty->setShortName(tr("Capacity"));
    m_batteryGrossCapacityProperty->setDescription(tr("Gross capacity of the battery"));
    m_batteryGrossCapacityProperty->setUnit(KSysGuard::UnitWattHour);
    m_batteryGrossCapacityProperty->setVariantType(QVariant::Int);
    m_batteryGrossCapacityProperty->setMin(0);

    m_batteryRemainingCapacityProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryRemainingCapacity"), tr("Battery Remaining Capacity"), this);
    m_batteryRemainingCapacityProperty->setShortName(tr("Remaining"));
    m_batteryRemainingCapacityProperty->setDescription(tr("Remaining capacity of the battery"));
    m_batteryRemainingCapacityProperty->setUnit(KSysGuard::UnitWattHour);
    m_batteryRemainingCapacityProperty->setVariantType(QVariant::Int);
    m_batteryRemainingCapacityProperty->setMin(0);

    m_batteryUsableCapacityProperty = new KSysGuard::SensorProperty(QStringLiteral("batteryUsableCapacity"), tr("Battery Usable Capacity"), this);
    m_batteryUsableCapacityProperty->setShortName(tr("Usable"));
    m_batteryUsableCapacityProperty->setDescription(tr("Percentage of usable battery capacity"));
    m_batteryUsableCapacityProperty->setUnit(KSysGuard::UnitPercent);
    m_batteryUsableCapacityProperty->setVariantType(QVariant::Int);
    m_batteryUsableCapacityProperty->setMin(0);

    // TODO ems status?
    // TODO inverterPower max sensor?
    // TODO photovoltaic max power?
    // Tried setting this as "max" for the PV power sensor but that caused the
    // scale to always use that value which was annoying.

#if PRESENTATION_BUILD
    //: Sensor object name with system information
    setName(tr("System"));
#else
    //: Sensor object name with system information
    setName(tr("%1 (System)").arg(serialNumber));
#endif
}

void SystemObject::update(const QModelIndex &index)
{
#if PRESENTATION_BUILD
    m_serialNumberProperty->setValue(tr("<Serial Number>"));
#else
    const QString serialNumber = index.data(static_cast<int>(StorageSystemsModel::Roles::SerialNumber)).toString();
    m_serialNumberProperty->setValue(serialNumber);
#endif

#if PRESENTATION_BUILD
    m_inverterModelProperty->setValue(tr("<Inverter Model>"));
#else
    const QString inverterModel = index.data(static_cast<int>(StorageSystemsModel::Roles::InverterModel)).toString();
    m_inverterModelProperty->setValue(inverterModel);
#endif

#if PRESENTATION_BUILD
    m_batteryModelProperty->setValue(tr("<Battery Model>"));
#else
    const QString batteryModel = index.data(static_cast<int>(StorageSystemsModel::Roles::BatteryModel)).toString();
    m_batteryModelProperty->setValue(batteryModel);
#endif

    const auto batteryGrossCapacity = index.data(static_cast<int>(StorageSystemsModel::Roles::BatteryGrossCapacity)).value<int>();
    m_batteryGrossCapacityProperty->setValue(batteryGrossCapacity);

    const auto batteryRemainingCapacity = index.data(static_cast<int>(StorageSystemsModel::Roles::BatteryRemainingCapacity)).value<int>();
    m_batteryRemainingCapacityProperty->setValue(batteryRemainingCapacity);

    const auto batteryUsableCapacity = index.data(static_cast<int>(StorageSystemsModel::Roles::BatteryUsableCapacity)).value<qreal>();
    m_batteryUsableCapacityProperty->setValue(batteryUsableCapacity);
}
