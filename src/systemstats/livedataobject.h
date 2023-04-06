/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <systemstats/SensorObject.h>

#include <QAlphaCloud/QAlphaCloud>

#include <QTimer>

namespace KSysGuard
{
class SensorContainer;
class SensorProperty;
} // namespace KSysGuard

namespace QAlphaCloud
{
class Connector;
class LastPowerData;
} // namespace QAlphaCloud

class LiveDataObject : public KSysGuard::SensorObject
{
public:
    LiveDataObject(QAlphaCloud::Connector *connector, const QString &serialNumber, KSysGuard::SensorContainer *parent);

    void update();

private:
    void updateGridPower(int power);
    void updateBatteryPower(int power);

    QAlphaCloud::LastPowerData *m_liveData = nullptr;

    // Live data:
    KSysGuard::SensorProperty *m_photovoltaicPowerProperty = nullptr;
    KSysGuard::SensorProperty *m_currentLoadProperty = nullptr;
    // TODO generic grid property
    KSysGuard::SensorProperty *m_gridFeedProperty = nullptr;
    KSysGuard::SensorProperty *m_gridConsumptionProperty = nullptr;

    KSysGuard::SensorProperty *m_batterySocProperty = nullptr;
    KSysGuard::SensorProperty *m_batteryChargeProperty = nullptr;
    KSysGuard::SensorProperty *m_batteryDischargeProperty = nullptr;
    // TODO battery status property

    QTimer m_rateLimitTimer;
    bool m_updatePending = false;
};
