/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <QWidget>

#include <QAlphaCloud/QAlphaCloud>

#include "ui_livedatawidget.h"

class QNetworkAccessManager;

namespace QAlphaCloud
{
class Connector;
class LastPowerData;
class StorageSystemsModel;
}

class LiveDataWidget : public QWidget
{
    Q_OBJECT

public:
    LiveDataWidget(QWidget *parent = nullptr);
    ~LiveDataWidget() override;

private:
    void onStorageSystemsModelStatusChanged(QAlphaCloud::RequestStatus status);
    void onLiveDataStatusChanged(QAlphaCloud::RequestStatus status);

    QNetworkAccessManager *m_networkAccessManager;

    QAlphaCloud::Connector *m_connector;
    QAlphaCloud::StorageSystemsModel *m_storageSystems;
    QAlphaCloud::LastPowerData *m_liveData;

    Ui_LiveDataWidget m_ui;
};
