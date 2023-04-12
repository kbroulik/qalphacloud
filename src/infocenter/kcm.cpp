/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcm.h"

#include <KPluginFactory>

#include <QAlphaCloud/OneDayPowerModel>

#include "config-alphacloud.h"

K_PLUGIN_CLASS_WITH_JSON(KCMAlphaCloud, "kcm_qalphacloud.json")

BatterySocScaleProxyModel::BatterySocScaleProxyModel(QObject *parent)
    : QIdentityProxyModel(parent)
{
}

BatterySocScaleProxyModel::~BatterySocScaleProxyModel() = default;

int BatterySocScaleProxyModel::max() const
{
    return m_max;
}

void BatterySocScaleProxyModel::setMax(int max)
{
    if (m_max != max) {
        beginResetModel();
        m_max = max;
        m_factor = max / 100.0;
        endResetModel();

        Q_EMIT maxChanged(max);
    }
}

QVariant BatterySocScaleProxyModel::data(const QModelIndex &index, int role) const
{
    if (role != static_cast<int>(QAlphaCloud::OneDayPowerModel::Roles::BatterySoc)) {
        return QIdentityProxyModel::data(index, role);
    }

    const int batterySoc = sourceModel()->data(mapToSource(index), role).toInt();
    return batterySoc * m_factor;
}

KCMAlphaCloud::KCMAlphaCloud(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, data, args)
{
    qmlRegisterType<BatterySocScaleProxyModel>("de.broulik.qalphacloud.private.kcm", 1, 0, "BatterySocScaleProxyModel");

    setButtons(KQuickAddons::ConfigModule::NoAdditionalButton);
}

bool KCMAlphaCloud::presentationBuild() const
{
#if PRESENTATION_BUILD
    return true;
#else
    return false;
#endif
}

#include "kcm.moc"
