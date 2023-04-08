/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <QAlphaCloud/Connector>
#include <QAlphaCloud/LastPowerData>
#include <QAlphaCloud/QAlphaCloud>

#include "testnetworkaccessmanager.h"

using namespace QAlphaCloud;

static QString g_serialNumber = QStringLiteral("SERIAL");

class LastPowerDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testInitialState();

    void testData();
    void testReloadEmpty();
    void testReloadError();
    void testReset();
    void testReloadInFlight();

    void testApiError();
    void testGarbledJson();

private:
    TestNetworkAccessManager m_networkAccessManager;
    Connector m_connector;
};

void LastPowerDataTest::initTestCase()
{
    // Don't rely on defaultConfiguration in tests!
    auto *configuration = new Configuration(&m_connector);
    configuration->setAppId(QStringLiteral("lastPowerTestApp"));
    configuration->setAppSecret(QStringLiteral("testSecret"));
    m_connector.setConfiguration(configuration);

    m_connector.setNetworkAccessManager(&m_networkAccessManager);
}

void LastPowerDataTest::testInitialState()
{
    {
        LastPowerData data;
        QCOMPARE(data.status(), RequestStatus::NoRequest);
        QVERIFY(!data.valid());

        // Can't load without a connector.
        QVERIFY(!data.reload());
    }

    {
        // Careful not to use the LastPowerData(QObject*) constructor!
        LastPowerData data(&m_connector, QString() /*serialNumber*/);
        QCOMPARE(data.connector(), &m_connector);

        QCOMPARE(data.status(), RequestStatus::NoRequest);
        QVERIFY(!data.valid());

        // Can't load without a seral number.
        QVERIFY(!data.reload());

        data.setSerialNumber(g_serialNumber);
        QCOMPARE(data.serialNumber(), g_serialNumber);

        QVERIFY(data.reload());
    }

    {
        LastPowerData data(&m_connector, g_serialNumber);
        QCOMPARE(data.connector(), &m_connector);
        QCOMPARE(data.serialNumber(), g_serialNumber);

        QCOMPARE(data.status(), RequestStatus::NoRequest);
        QVERIFY(!data.valid());

        QVERIFY(data.reload());
    }
}

void LastPowerDataTest::testData()
{
    LastPowerData data(&m_connector, g_serialNumber);

    const QString testData1Path = QFINDTESTDATA("data/lastpowerdata_1.json");
    const QString testData2Path = QFINDTESTDATA("data/lastpowerdata_2.json");

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testData1Path));

    // Load our first test data set.
    QVERIFY(data.reload());

    QCOMPARE(data.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(data.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(data.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(data.errorString().isEmpty());
    QVERIFY(data.valid());

    QCOMPARE(data.photovoltaicPower(), 4397);
    QCOMPARE(data.currentLoad(), 610);
    QCOMPARE(data.gridPower(), -4358);
    QCOMPARE(data.batteryPower(), 111);
    QCOMPARE(data.batterySoc(), 98);

    // Also verify the raw JSON with the JSON from the file.
    QFile testFile1(testData1Path);
    QVERIFY(testFile1.open(QIODevice::ReadOnly | QIODevice::Text));

    const QJsonObject testJson1 = QJsonDocument::fromJson(testFile1.readAll()).object().value(QStringLiteral("data")).toObject();
    QCOMPARE(data.rawJson(), testJson1);

    // Now load our second test data set.
    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testData2Path));

    QVERIFY(data.reload());

    QCOMPARE(data.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(data.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(data.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(data.errorString().isEmpty());
    QVERIFY(data.valid());

    QCOMPARE(data.photovoltaicPower(), 10);
    QCOMPARE(data.currentLoad(), 2500);
    QCOMPARE(data.gridPower(), 2400);
    QCOMPARE(data.batteryPower(), 101);
    QCOMPARE(data.batterySoc(), 55);

    // Also verify the raw JSON with the JSON from the file.
    QFile testFile2(testData2Path);
    QVERIFY(testFile2.open(QIODevice::ReadOnly | QIODevice::Text));

    const QJsonObject testJson2 = QJsonDocument::fromJson(testFile2.readAll()).object().value(QStringLiteral("data")).toObject();
    QCOMPARE(data.rawJson(), testJson2);
}

void LastPowerDataTest::testReloadEmpty()
{
}

void LastPowerDataTest::testReloadError()
{
}

void LastPowerDataTest::testReset()
{
}

void LastPowerDataTest::testReloadInFlight()
{
}

void LastPowerDataTest::testApiError()
{
    LastPowerData data(&m_connector, g_serialNumber);

    const QString testDataPath = QFINDTESTDATA("data/api_error.json");

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    QVERIFY(data.reload());

    QCOMPARE(data.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(data.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(data.error(), QAlphaCloud::ErrorCode::ParameterError);
    QCOMPARE(data.errorString(), QStringLiteral("Parameter error"));
    QVERIFY(!data.valid());
}

void LastPowerDataTest::testGarbledJson()
{
    LastPowerData data(&m_connector, g_serialNumber);

    const QString testDataPath = QFINDTESTDATA("data/garbled.json");

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    QVERIFY(data.reload());

    QCOMPARE(data.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(data.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(data.error(), QAlphaCloud::ErrorCode::JsonParseError);
    QVERIFY(!data.errorString().isEmpty());
    QVERIFY(!data.valid());
}

QTEST_GUILESS_MAIN(LastPowerDataTest)
#include "lastpowerdatatest.moc"
