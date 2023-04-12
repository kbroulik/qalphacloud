/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDate>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMetaType>
#include <QNetworkAccessManager>
#include <QTimer>

#include <iostream>

#include <QAlphaCloud/Configuration>
#include <QAlphaCloud/Connector>
#include <QAlphaCloud/LastPowerData>
#include <QAlphaCloud/OneDateEnergy>
#include <QAlphaCloud/OneDayPowerModel>
#include <QAlphaCloud/StorageSystemsModel>

#include "config-alphacloud.h"
#include "qalphacloud_version.h"

static bool g_jsonOutput = false;
static int g_updateInterval = 0;

using namespace QAlphaCloud;
using namespace std;

void printModelContents(QAbstractItemModel *model, const QMetaEnum &rolesEnum)
{
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex idx = model->index(i, 0);

        if (i > 0) {
            cout << endl;
        }

        for (int enumIndex = 0; enumIndex < rolesEnum.keyCount(); ++enumIndex) {
            const auto enumKey = rolesEnum.key(enumIndex);
            const int enumValue = rolesEnum.value(enumIndex);
            if (qstrcmp(enumKey, "RawJson") == 0) {
                continue;
            }

#if PRESENTATION_BUILD
            if (qstrcmp(enumKey, "SerialNumber") == 0 || qstrcmp(enumKey, "InverterModel") == 0 || qstrcmp(enumKey, "BatteryModel") == 0) {
                continue;
            }
#endif

            cout << enumKey << ": " << qPrintable(idx.data(enumValue).toString()) << endl;
        }
    }
}

void printQObject(QObject *item)
{
    const QMetaObject *mo = item->metaObject();
    const int rawJsonPropertyIdx = mo->indexOfProperty("rawJson");
    const int statusPropertyIdx = mo->indexOfProperty("status");
    const int errorPropertyIdx = mo->indexOfProperty("error");
    const int errorStringPropertyIdx = mo->indexOfProperty("errorString");

    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        if (i == rawJsonPropertyIdx || i == statusPropertyIdx || i == errorPropertyIdx || i == errorStringPropertyIdx) {
            continue;
        }

        const QMetaProperty prop = mo->property(i);
        if (prop.isWritable() || !prop.isStored()) {
            continue;
        }

        cout << prop.name() << ": ";

        const QVariant value = prop.read(item);

        if (value.isValid()) {
            cout << qPrintable(value.toString());
        } else {
            cout << "N/A";
        }

        cout << endl;
    }
}

void listStorageSystems(QAlphaCloud::Connector *connector)
{
    auto *storages = new StorageSystemsModel(connector);
    storages->setCached(false);

    QObject::connect(storages, &StorageSystemsModel::statusChanged, [storages](QAlphaCloud::RequestStatus status) {
        if (status == QAlphaCloud::RequestStatus::Error) {
            cerr << "Failed to list storage systems" << endl;
            QCoreApplication::exit(1);
        } else if (status == QAlphaCloud::RequestStatus::Finished) {
            cerr << storages->rowCount() << " storage system(s) found:" << endl;

            if (g_jsonOutput) {
                QJsonArray systems;

                for (int i = 0; i < storages->rowCount(); ++i) {
                    const QModelIndex idx = storages->index(i, 0);
                    systems.append(idx.data(static_cast<int>(QAlphaCloud::StorageSystemsModel::Roles::RawJson)).toJsonObject());
                }

                cout << qPrintable(QJsonDocument(systems).toJson());
            } else {
                cerr << endl;

                printModelContents(storages, QMetaEnum::fromType<QAlphaCloud::StorageSystemsModel::Roles>());
            }

            if (g_updateInterval == 0) {
                QCoreApplication::quit();
            } else {
                cout << endl;
            }
        }
    });

    if (g_updateInterval > 0) {
        auto *timer = new QTimer();
        timer->setInterval(g_updateInterval);
        QObject::connect(timer, &QTimer::timeout, storages, &StorageSystemsModel::reload);
        timer->start();
    }

    storages->reload();
}

void showLastPowerData(Connector *connector, const QString &serialNumber)
{
    auto *data = new LastPowerData(connector, serialNumber);

    QObject::connect(data, &LastPowerData::statusChanged, [data](QAlphaCloud::RequestStatus status) {
        if (status == QAlphaCloud::RequestStatus::Error) {
            cerr << "Failed to load last power data" << endl;
            QCoreApplication::exit(1);
        } else if (status == QAlphaCloud::RequestStatus::Finished) {
            if (g_jsonOutput) {
                cout << qPrintable(QJsonDocument(data->rawJson()).toJson());
            } else {
                printQObject(data);
            }

            if (g_updateInterval == 0) {
                QCoreApplication::quit();
            } else {
                cout << endl;
            }
        }
    });

    if (g_updateInterval > 0) {
        auto *timer = new QTimer();
        timer->setInterval(g_updateInterval);
        QObject::connect(timer, &QTimer::timeout, data, &LastPowerData::reload);
        timer->start();
    }

    data->reload();
}

void showEnergy(Connector *connector, const QString &serialNumber, const QDate &date)
{
    auto *data = new OneDateEnergy(connector, serialNumber, date);
    data->setCached(false);

    QObject::connect(data, &OneDateEnergy::statusChanged, [data](QAlphaCloud::RequestStatus status) {
        if (status == QAlphaCloud::RequestStatus::Error) {
            cerr << "Failed to load last power data" << endl;
            QCoreApplication::exit(1);
        } else if (status == QAlphaCloud::RequestStatus::Finished) {
            if (!data->valid()) {
                cerr << "No data available for this date" << endl;
                // TODO exit1?
            } else {
                if (g_jsonOutput) {
                    cout << qPrintable(QJsonDocument(data->rawJson()).toJson());
                } else {
                    printQObject(data);
                }
            }

            if (g_updateInterval == 0) {
                QCoreApplication::quit();
            } else {
                cout << endl;
            }
        }
    });

    if (g_updateInterval > 0) {
        auto *timer = new QTimer();
        timer->setInterval(g_updateInterval);
        QObject::connect(timer, &QTimer::timeout, data, &OneDateEnergy::reload);
        timer->start();
    }

    data->reload();
}

void showHistory(Connector *connector, const QString &serialNumber, const QDate &date)
{
    auto *model = new OneDayPowerModel(connector, serialNumber, date);
    model->setCached(false);

    QObject::connect(model, &OneDayPowerModel::statusChanged, [model](QAlphaCloud::RequestStatus status) {
        if (status == QAlphaCloud::RequestStatus::Error) {
            cerr << "Failed to load last power data" << endl;
            QCoreApplication::exit(1);
        } else if (status == QAlphaCloud::RequestStatus::Finished) {
            cerr << model->rowCount() << " history entries found:" << endl;

            if (g_jsonOutput) {
                QJsonArray entries;

                for (int i = 0; i < model->rowCount(); ++i) {
                    const QModelIndex idx = model->index(i, 0);
                    entries.append(idx.data(static_cast<int>(OneDayPowerModel::Roles::RawJson)).toJsonObject());
                }

                cout << qPrintable(QJsonDocument(entries).toJson());
            } else {
                cerr << endl;

                printModelContents(model, QMetaEnum::fromType<OneDayPowerModel::Roles>());
            }

            if (g_updateInterval == 0) {
                QCoreApplication::quit();
            }
        }
    });

    if (g_updateInterval > 0) {
        auto *timer = new QTimer();
        timer->setInterval(g_updateInterval);
        QObject::connect(timer, &QTimer::timeout, model, &OneDayPowerModel::reload);
        timer->start();
    }

    model->reload();
}

QString getPrimarySerial(Connector *connector)
{
    cerr << "Fetching primary serial number..." << endl;
    StorageSystemsModel storages(connector, nullptr);
    storages.setCached(false);

    QEventLoop loop;
    QObject::connect(&storages, &StorageSystemsModel::statusChanged, [&loop](QAlphaCloud::RequestStatus status) {
        if (status == QAlphaCloud::RequestStatus::Error) {
            cerr << "Failed to list storage systems" << endl;
            loop.exit(1);
        } else if (status == QAlphaCloud::RequestStatus::Finished) {
            loop.quit();
        }
    });
    storages.reload();
    loop.exec();

    return storages.primarySerialNumber();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QALPHACLOUD_VERSION_STRING);

    QCommandLineParser parser;

    QCommandLineOption apiUrlOpt({QStringLiteral("u"), QStringLiteral("url")}, QStringLiteral("API URL"), QStringLiteral("apiUrl"));
    parser.addOption(apiUrlOpt);
    QCommandLineOption apiKeyOpt({QStringLiteral("k"), QStringLiteral("key")}, QStringLiteral("App ID"), QStringLiteral("appId"));
    parser.addOption(apiKeyOpt);
    QCommandLineOption apiSecretOpt({QStringLiteral("p"), QStringLiteral("secret")}, QStringLiteral("App Secret"), QStringLiteral("appSecret"));
    parser.addOption(apiSecretOpt);

    QCommandLineOption serialOpt({QStringLiteral("s"), QStringLiteral("sn")}, QStringLiteral("Serial Number"), QStringLiteral("serialNumber"));
    parser.addOption(serialOpt);
    QCommandLineOption dateOpt({QStringLiteral("d"), QStringLiteral("day"), QStringLiteral("date")}, QStringLiteral("Date"), QStringLiteral("date"));
    parser.addOption(dateOpt);

    QCommandLineOption jsonOpt({QStringLiteral("j"), QStringLiteral("json")}, QStringLiteral("Output JSON"));
    parser.addOption(jsonOpt);
    // TODO Cannot create an option that can take an optional argument (e.g. have
    // "-w" use the default or allow "-w 1000")
    QCommandLineOption followOpt({QStringLiteral("w"), QStringLiteral("follow")}, QStringLiteral("Update periodically"));
    parser.addOption(followOpt);

    parser.addPositionalArgument(QStringLiteral("endpoint"),
                                 QStringLiteral("The API endpoint to talk to (essList/storageSystems, "
                                                "lastPowerData/live, oneDateEnergy/energy, oneDayPower/history)"));

    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    const QUrl apiUrl = QUrl(parser.value(apiUrlOpt)); // TODO fromUserInput?
    const QString apiKey = parser.value(apiKeyOpt);
    const QString apiSecret = parser.value(apiSecretOpt);
    QString serialNumber = parser.value(serialOpt);

    g_jsonOutput = parser.isSet(jsonOpt);

    QDate date = QDate::fromString(parser.value(dateOpt), Qt::ISODate);
    // TODO exit with an error if date explicitly provided but invalid?
    if (!date.isValid()) {
        date = QDate::currentDate();
    }

    cerr << "QAlphaCloud CLI" << endl;

    Configuration config;
    config.loadDefault();

    if (apiUrl.isValid()) {
        config.setApiUrl(apiUrl);
    }
    if (!apiKey.isEmpty()) {
        config.setAppId(apiKey);
    }
    if (!apiSecret.isEmpty()) {
        config.setAppSecret(apiSecret);
    }

    if (config.apiUrl().isEmpty()) {
        cerr << "No API URL provided" << endl;
    } else if (!config.apiUrl().isValid()) {
        cerr << "Invalid API URL provided: " << qPrintable(config.apiUrl().errorString()) << endl;
    }
    if (config.appId().isEmpty()) {
        cerr << "No API key provided" << endl;
    }
    if (config.appSecret().isEmpty()) {
        cerr << "No API secret provided" << endl;
    }

    if (parser.isSet(followOpt)) {
        g_updateInterval = 10000; // TODO allow specifying.
    }

    if (!apiUrl.isEmpty()) {
        config.setApiUrl(apiUrl);
    }
    if (!apiKey.isEmpty()) {
        config.setAppId(apiKey);
    }
    if (!apiSecret.isEmpty()) {
        config.setAppSecret(apiSecret);
    }

    if (config.apiUrl().isEmpty() || !config.apiUrl().isValid() || config.appId().isEmpty() || config.appSecret().isEmpty()) {
        parser.showHelp(1);
    }

    if (parser.positionalArguments().count() != 1) {
        cerr << "No endpoint provided" << endl;
        parser.showHelp(1);
    }

    const QString endpoint = parser.positionalArguments().first();

    QNetworkAccessManager manager;
    manager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    Connector connector;
    connector.setConfiguration(&config);

    connector.setNetworkAccessManager(&manager);

    cerr << "  API URL: " << qPrintable(config.apiUrl().toDisplayString()) << endl << endl;

    if (endpoint.compare(QLatin1String("essList"), Qt::CaseInsensitive) == 0 || endpoint.compare(QLatin1String("storagesystems"), Qt::CaseInsensitive) == 0) {
        cerr << "List storage systems:" << endl;
        listStorageSystems(&connector);

    } else if (endpoint.compare(QLatin1String("lastPowerData"), Qt::CaseInsensitive) == 0
               || endpoint.compare(QLatin1String("live"), Qt::CaseInsensitive) == 0) {
        cerr << "Read last power data:" << endl;

        if (serialNumber.isEmpty()) {
            serialNumber = getPrimarySerial(&connector);
        }

        if (serialNumber.isEmpty()) {
            cerr << "No serial number provided" << endl;
            return 1;
        }

#if !PRESENTATION_BUILD
        cerr << "Serial number: " << qPrintable(serialNumber) << endl;
#endif
        showLastPowerData(&connector, serialNumber);

    } else if (endpoint.compare(QLatin1String("oneDateEnergyBySn"), Qt::CaseInsensitive) == 0
               || endpoint.compare(QLatin1String("oneDateEnergy"), Qt::CaseInsensitive) == 0
               || endpoint.compare(QLatin1String("energy"), Qt::CaseInsensitive) == 0) {
        cerr << "One date energy:" << endl;

        if (serialNumber.isEmpty()) {
            serialNumber = getPrimarySerial(&connector);
        }

        if (serialNumber.isEmpty()) {
            cerr << "No serial number provided" << endl;
            return 1;
        }

#if !PRESENTATION_BUILD
        cerr << "Serial number: " << qPrintable(serialNumber) << endl;
#endif
        cerr << "Date: " << qPrintable(date.toString(Qt::ISODate)) << endl;
        showEnergy(&connector, serialNumber, date);

    } else if (endpoint.compare(QLatin1String("oneDayPowerBySn"), Qt::CaseInsensitive) == 0
               || endpoint.compare(QLatin1String("oneDayPower"), Qt::CaseInsensitive) == 0
               || endpoint.compare(QLatin1String("history"), Qt::CaseInsensitive) == 0) {
        cerr << "One day power:" << endl;

        if (serialNumber.isEmpty()) {
            serialNumber = getPrimarySerial(&connector);
        }

        if (serialNumber.isEmpty()) {
            cerr << "No serial number provided" << endl;
            return 1;
        }

#if !PRESENTATION_BUILD
        cerr << "Serial number: " << qPrintable(serialNumber) << endl;
#endif
        cerr << "Date: " << qPrintable(date.toString(Qt::ISODate)) << endl;
        showHistory(&connector, serialNumber, date);

    } else {
        cerr << "Unknown endpoint provided: " << qPrintable(endpoint) << endl;
        parser.showHelp(1);
    }

    return app.exec();
}
