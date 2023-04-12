/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "livedatawidget.h"

#include <QApplication>
#include <QNetworkAccessManager>

#include <QAlphaCloud/Connector>
#include <QAlphaCloud/LastPowerData>
#include <QAlphaCloud/StorageSystemsModel>

#include "config-alphacloud.h"

using namespace QAlphaCloud;

LiveDataWidget::LiveDataWidget(QWidget *parent)
    : QWidget(parent)
    , m_networkAccessManager(new QNetworkAccessManager(this))
    , m_connector(new Connector(this))
    , m_storageSystems(new StorageSystemsModel(m_connector, this))
    , m_liveData(new LastPowerData(this))
{
    m_ui.setupUi(this);
    // Wire up Reload button.
    connect(m_ui.reloadButton, &QToolButton::clicked, m_liveData, &LastPowerData::reload);

    // Set up network manager. Important to let it automatically follow redirects.
    m_networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_connector->setNetworkAccessManager(m_networkAccessManager);

    // Configure API.
    auto *config = new Configuration(m_connector);
    // NOTE put your credentials here!
    config->setAppId(QStringLiteral("alpha..."));
    config->setAppSecret(QStringLiteral("..."));
    // or use this to load it from the QAlphaCloud config file.
    // config->loadDefault();

    m_connector->setConfiguration(config);

    // Hook up the storage systems model signals.
    connect(m_storageSystems, &StorageSystemsModel::statusChanged, this, &LiveDataWidget::onStorageSystemsModelStatusChanged);

    // Set up live data. It needs a connector.
    m_liveData->setConnector(m_connector);
    connect(m_liveData, &LastPowerData::statusChanged, this, &LiveDataWidget::onLiveDataStatusChanged);
    onLiveDataStatusChanged(RequestStatus::NoRequest);

    if (!m_storageSystems->reload()) {
        qFatal("Failed to load storages, check that your Configuration object is correctly configured.");
    }
}

LiveDataWidget::~LiveDataWidget() = default;

void LiveDataWidget::onStorageSystemsModelStatusChanged(RequestStatus status)
{
    if (status == RequestStatus::Loading) {
        m_ui.serialNumberLabel->setText(tr("Loading…"));
    } else if (status == RequestStatus::Finished) {
        // Storage systems have loaded, now load live data from the primary serial number.
#if PRESENTATION_BUILD
        m_ui.serialNumberLabel->setText(tr("<Serial Number>"));
#else
        m_ui.serialNumberLabel->setText(m_storageSystems->primarySerialNumber());
#endif
        m_liveData->setSerialNumber(m_storageSystems->primarySerialNumber());
        m_liveData->reload();
    } else if (status == RequestStatus::Error) {
        m_ui.serialNumberLabel->setText(tr("Error: %1").arg(m_storageSystems->errorString()));
    }
}

void LiveDataWidget::onLiveDataStatusChanged(RequestStatus status)
{
    const QString noDataString = QStringLiteral("–");
    const QString loadingString = tr("Loading…");
    const QString wattTemplate = QStringLiteral("%L1 W");
    const QString errorString = tr("Error");

    switch (status) {
    case RequestStatus::NoRequest:
        m_ui.photovoltaicValue->setText(noDataString);
        m_ui.currentLoadValue->setText(noDataString);
        m_ui.gridPowerValue->setText(noDataString);
        m_ui.batteryPowerValue->setText(noDataString);
        m_ui.batterySocValue->setText(noDataString);
        break;

    case RequestStatus::Loading:
        m_ui.photovoltaicValue->setText(loadingString);
        m_ui.currentLoadValue->setText(loadingString);
        m_ui.gridPowerValue->setText(loadingString);
        m_ui.batteryPowerValue->setText(loadingString);
        m_ui.batterySocValue->setText(loadingString);
        break;

    case RequestStatus::Finished:
        m_ui.photovoltaicValue->setText(wattTemplate.arg(m_liveData->photovoltaicPower()));
        m_ui.currentLoadValue->setText(wattTemplate.arg(m_liveData->currentLoad()));
        m_ui.gridPowerValue->setText(wattTemplate.arg(m_liveData->gridPower()));
        m_ui.batteryPowerValue->setText(wattTemplate.arg(m_liveData->batteryPower()));
        m_ui.batterySocValue->setText(QStringLiteral("%L1 %").arg(m_liveData->batterySoc()));
        break;

    case RequestStatus::Error:
        m_ui.photovoltaicValue->setText(errorString);
        m_ui.currentLoadValue->setText(errorString);
        m_ui.gridPowerValue->setText(errorString);
        m_ui.batteryPowerValue->setText(errorString);
        m_ui.batterySocValue->setText(errorString);
        break;
    }
}
