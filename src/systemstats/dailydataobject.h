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
class OneDateEnergy;
} // namespace QAlphaCloud

class QModelIndex;

class DailyDataObject : public KSysGuard::SensorObject
{
public:
    DailyDataObject(QAlphaCloud::Connector *connector, const QString &serialNumber, KSysGuard::SensorContainer *parent);

    void update();
    void updateSystem(const QModelIndex &index);

private:
    void reload();

    QAlphaCloud::OneDateEnergy *m_dailyData = nullptr;

    KSysGuard::SensorProperty *m_photovoltaicProperty = nullptr;

    KSysGuard::SensorProperty *m_totalLoadProperty = nullptr;

    KSysGuard::SensorProperty *m_inputProperty = nullptr;
    KSysGuard::SensorProperty *m_outputProperty = nullptr;

    KSysGuard::SensorProperty *m_chargeProperty = nullptr;
    KSysGuard::SensorProperty *m_dischargeProperty = nullptr;
    KSysGuard::SensorProperty *m_gridChargeProperty = nullptr;

    QTimer m_rateLimitTimer;
    bool m_updatePending = false;
};
