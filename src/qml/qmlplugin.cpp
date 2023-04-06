/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQmlParserStatus>

#include <QAlphaCloud/Configuration>
#include <QAlphaCloud/Connector>
#include <QAlphaCloud/LastPowerData>
#include <QAlphaCloud/OneDateEnergy>
#include <QAlphaCloud/OneDayPowerModel>
#include <QAlphaCloud/StorageSystemsModel>

class QAlphaCloudQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "de.broulik.qalphacloud")

public:
    QAlphaCloudQmlPlugin(QObject *parent = nullptr)
        : QQmlExtensionPlugin(parent)
    {
    }

    void registerTypes(const char *uri) override;
};

using namespace QAlphaCloud;

class QmlConfiguration : public QAlphaCloud::Configuration, public QQmlParserStatus
{
    Q_INTERFACES(QQmlParserStatus)
public:
    explicit QmlConfiguration(QObject *parent = nullptr)
        : QAlphaCloud::Configuration(parent)
    {
    }

    void classBegin() override
    {
        // Here rather than in componentComplete so that that
        // it can be overwritten by property assignment.
        loadDefault();
    }

    void componentComplete() override
    {
    }
};

class QmlConnector : public QAlphaCloud::Connector, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

public:
    explicit QmlConnector(QObject *parent = nullptr)
        : QAlphaCloud::Connector(parent)
    {
    }

    void classBegin() override
    {
    }

    void componentComplete() override
    {
        if (!networkAccessManager()) {
            setNetworkAccessManager(qmlEngine(this)->networkAccessManager());
        }
    }
};

class QmlLastPowerData : public QAlphaCloud::LastPowerData, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool active MEMBER m_active NOTIFY activeChanged)

public:
    explicit QmlLastPowerData(QObject *parent = nullptr)
        : QAlphaCloud::LastPowerData(parent)
    {
    }

    void classBegin() override
    {
    }

    void componentComplete() override
    {
        connect(this, &QmlLastPowerData::serialNumberChanged, this, &QmlLastPowerData::reloadIfActive);
        // Suppress warnings on autoload.
        if (connector() && connector()->configuration() && connector()->configuration()->valid() && !serialNumber().isEmpty()) {
            reloadIfActive();
        }
    }

Q_SIGNALS:
    void activeChanged(bool active);

private:
    void reloadIfActive()
    {
        if (m_active) {
            reload();
        }
    }
    bool m_active = true;
};

class QmlOneDateEnergy : public QAlphaCloud::OneDateEnergy, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool active MEMBER m_active NOTIFY activeChanged)

public:
    explicit QmlOneDateEnergy(QObject *parent = nullptr)
        : QAlphaCloud::OneDateEnergy(parent)
    {
    }

    void classBegin() override
    {
    }

    void componentComplete() override
    {
        connect(this, &QmlOneDateEnergy::serialNumberChanged, this, &QmlOneDateEnergy::reloadIfActive);
        connect(this, &QmlOneDateEnergy::dateChanged, this, &QmlOneDateEnergy::reloadIfActive);
        // Suppress warnings on autoload.
        if (connector() && connector()->configuration() && connector()->configuration()->valid() && !serialNumber().isEmpty() && date().isValid()) {
            reloadIfActive();
        }
    }

Q_SIGNALS:
    void activeChanged(bool active);

private:
    void reloadIfActive()
    {
        if (m_active) {
            reload();
        }
    }
    bool m_active = true;
};

class QmlOneDayPowerModel : public QAlphaCloud::OneDayPowerModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool active MEMBER m_active NOTIFY activeChanged)

public:
    explicit QmlOneDayPowerModel(QObject *parent = nullptr)
        : QAlphaCloud::OneDayPowerModel(parent)
    {
    }

    void classBegin() override
    {
    }

    void componentComplete() override
    {
        connect(this, &QmlOneDayPowerModel::serialNumberChanged, this, &QmlOneDayPowerModel::reloadIfActive);
        connect(this, &QmlOneDayPowerModel::dateChanged, this, &QmlOneDayPowerModel::reloadIfActive);
        // Suppress warnings on autoload.
        if (connector() && connector()->configuration() && connector()->configuration()->valid() && !serialNumber().isEmpty() && date().isValid()) {
            reloadIfActive();
        }
    }

Q_SIGNALS:
    void activeChanged(bool active);

private:
    void reloadIfActive()
    {
        if (m_active) {
            reload();
        }
    }

    bool m_active = true;
};

void QAlphaCloudQmlPlugin::registerTypes(const char *uri)
{
    //@uri de.broulik.qalphacloudpl
    qmlRegisterType<QmlConfiguration>(uri, 1, 0, "Configuration");
    qmlRegisterType<QmlConnector>(uri, 1, 0, "Connector");
    qmlRegisterType<QmlLastPowerData>(uri, 1, 0, "LastPowerData");
    qmlRegisterType<QmlOneDateEnergy>(uri, 1, 0, "OneDateEnergy");
    qmlRegisterType<QmlOneDayPowerModel>(uri, 1, 0, "OneDayPowerModel");
    // TODO figure out autoload, i.e. wait for Connector to become valid (when its QNAM is set) and then reload.
    qmlRegisterType<QAlphaCloud::StorageSystemsModel>(uri, 1, 0, "StorageSystemsModel");

    qmlRegisterUncreatableMetaObject(QAlphaCloud::staticMetaObject, uri, 1, 0, "QAlphaCloud", tr("Cannot create instances of QAlphaCloud."));
}

#include "qmlplugin.moc"
