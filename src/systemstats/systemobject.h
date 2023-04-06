/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <systemstats/SensorObject.h>

namespace KSysGuard
{
class SensorContainer;
class SensorProperty;
} // namespace KSysGuard

class SystemObject : public KSysGuard::SensorObject
{
public:
    SystemObject(const QString &serialNumber, KSysGuard::SensorContainer *parent);

    void update(const QModelIndex &index);

private:
    KSysGuard::SensorProperty *m_serialNumberProperty = nullptr;
    KSysGuard::SensorProperty *m_inverterModelProperty = nullptr;
    KSysGuard::SensorProperty *m_batteryModelProperty = nullptr;

    KSysGuard::SensorProperty *m_batteryGrossCapacityProperty = nullptr;
    KSysGuard::SensorProperty *m_batteryRemainingCapacityProperty = nullptr;
    KSysGuard::SensorProperty *m_batteryUsableCapacityProperty = nullptr;
};
