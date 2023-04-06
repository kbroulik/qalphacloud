/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QSignalSpy>
#include <QTest>

#include <QAlphaCloud/Configuration>

#include "config-alphacloud.h"

using namespace QAlphaCloud;

static int g_defaultTimeout = 30000;

static const QUrl g_defaultUrl = QUrl(QStringLiteral(API_URL));

class ConfigurationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testLoadFromFile();
    void testLoadFromEmptyFile();
    void testLoadFromBrokenFile();
    // TODO test load from QSettings.
    // TODO test resetApiUrl/resetTimeout
};

void ConfigurationTest::testLoadFromFile()
{
    Configuration config;
    QCOMPARE(config.apiUrl(), g_defaultUrl);
    QVERIFY(config.appId().isEmpty());
    QVERIFY(config.appSecret().isEmpty());
    QCOMPARE(config.requestTimeout(), g_defaultTimeout);

    QSignalSpy validChangedSpy(&config, &Configuration::validChanged);
    QSignalSpy apiUrlChangedSpy(&config, &Configuration::apiUrlChanged);
    QSignalSpy appSecretChangedSpy(&config, &Configuration::appSecretChanged);
    QSignalSpy requestTimeoutChangedSpy(&config, &Configuration::requestTimeoutChanged);
    config.loadFromFile(QFINDTESTDATA("data/config_good.ini"));

    QVERIFY(config.valid());
    QCOMPARE(config.apiUrl(), QUrl(QStringLiteral("https://www.example.com/api/")));
    QCOMPARE(config.appId(), QStringLiteral("alpha123456"));
    QCOMPARE(config.appSecret(), QStringLiteral("abc123456789"));
    QCOMPARE(config.requestTimeout(), 31337);

    QCOMPARE(validChangedSpy.count(), 1);
    QCOMPARE(apiUrlChangedSpy.count(), 1);
    QCOMPARE(appSecretChangedSpy.count(), 1);
    QCOMPARE(requestTimeoutChangedSpy.count(), 1);
}

void ConfigurationTest::testLoadFromEmptyFile()
{
    Configuration config;
    config.loadFromFile(QFINDTESTDATA("data/config_empty.ini"));

    QVERIFY(!config.valid());
    QCOMPARE(config.apiUrl(), QUrl(QStringLiteral(API_URL)));
    QVERIFY(config.appId().isEmpty());
    QVERIFY(config.appSecret().isEmpty());
    QCOMPARE(config.requestTimeout(), g_defaultTimeout);
}

void ConfigurationTest::testLoadFromBrokenFile()
{
    Configuration config;
    config.loadFromFile(QFINDTESTDATA("data/config_garbled.ini"));

    QVERIFY(!config.valid());
    QCOMPARE(config.apiUrl(), g_defaultUrl);
    QVERIFY(config.appId().isEmpty());
    QVERIFY(config.appSecret().isEmpty());
    QCOMPARE(config.requestTimeout(), g_defaultTimeout);
}

QTEST_GUILESS_MAIN(ConfigurationTest)
#include "configurationtest.moc"
