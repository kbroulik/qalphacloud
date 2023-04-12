/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KQuickAddons/ConfigModule>

#include <QIdentityProxyModel>

class BatterySocScaleProxyModel : public QIdentityProxyModel
{
    Q_OBJECT

    Q_PROPERTY(int max READ max WRITE setMax NOTIFY maxChanged)

public:
    BatterySocScaleProxyModel(QObject *parent = nullptr);
    ~BatterySocScaleProxyModel() override;

    int max() const;
    void setMax(int max);
    Q_SIGNAL void maxChanged(int max);

    QVariant data(const QModelIndex &proxyIndex, int role) const override;

private:
    int m_max = 0;
    qreal m_factor = 0;
};

class KCMAlphaCloud : public KQuickAddons::ConfigModule
{
    Q_OBJECT

    Q_PROPERTY(bool presentationBuild READ presentationBuild CONSTANT)

public:
    explicit KCMAlphaCloud(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    bool presentationBuild() const;
};
