/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <QAlphaCloud/QAlphaCloud>
#include <QAlphaCloud/Connector>
#include <QAlphaCloud/OneDateEnergy>

#include "testnetworkaccessmanager.h"

using namespace QAlphaCloud;

static QString g_serialNumber = QStringLiteral("SERIAL");

class OneDateEnergyTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testInitialState();

    void testData();
    // TODO testReloadEmpty
    // TODO testReloadError
    // TODO testReset
    // TODO testReloadInFlight
    // TODO testCache

    void testApiError();
    void testGarbledJson();

private:
    TestNetworkAccessManager m_networkAccessManager;
    Connector m_connector;
};

void OneDateEnergyTest::initTestCase()
{
    // Don't rely on defaultConfiguration in tests!
    auto *configuration = new Configuration(&m_connector);
    configuration->setAppId(QStringLiteral("oneDateEnergyApp"));
    configuration->setAppSecret(QStringLiteral("testSecret"));
    m_connector.setConfiguration(configuration);

    m_connector.setNetworkAccessManager(&m_networkAccessManager);
}

void OneDateEnergyTest::testInitialState()
{
    {
        OneDateEnergy energy;
        QCOMPARE(energy.status(), RequestStatus::NoRequest);
        QVERIFY(!energy.valid());

        // Can't load without a connector.
        QVERIFY(!energy.reload());
    }

    {
        // Careful not to use the OneDateEnergy(QObject*) constructor!
        OneDateEnergy energy(&m_connector, QString() /*serialNumber*/, QDate() /*date*/);
        QCOMPARE(energy.connector(), &m_connector);

        QCOMPARE(energy.status(), RequestStatus::NoRequest);
        QVERIFY(!energy.valid());

        // Can't load without a seral number.
        QVERIFY(!energy.reload());

        energy.setSerialNumber(g_serialNumber);
        QCOMPARE(energy.serialNumber(), g_serialNumber);

        // Can't load without a date.
        QVERIFY(!energy.reload());

        const QDate date = QDate(2023, 01, 01);
        energy.setDate(date);
        QCOMPARE(energy.date(), date);

        energy.resetDate();
        QCOMPARE(energy.date(), QDate::currentDate());

        QVERIFY(energy.reload());
    }

    {
        OneDateEnergy energy(&m_connector, g_serialNumber, QDate::currentDate());
        QCOMPARE(energy.connector(), &m_connector);
        QCOMPARE(energy.serialNumber(), g_serialNumber);

        QCOMPARE(energy.status(), RequestStatus::NoRequest);
        QVERIFY(!energy.valid());

        QVERIFY(energy.reload());
    }
}

void OneDateEnergyTest::testData()
{
    const QString testData1Path = QFINDTESTDATA("data/onedateenergy_1.json");
    const QString testData2Path = QFINDTESTDATA("data/onedateenergy_2.json");

    const QDate date1(2023, 01, 01);
    const QDate date2(2023, 02, 27);

    OneDateEnergy energy(&m_connector, g_serialNumber, date1);
    QCOMPARE(energy.date(), date1);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testData1Path));

    // Load our first test data set.
    QVERIFY(energy.reload());

    QCOMPARE(energy.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(energy.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(energy.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(energy.errorString().isEmpty());
    QVERIFY(energy.valid());

    QCOMPARE(energy.photovoltaic(), 20100); // WattHours.
    QCOMPARE(energy.input(), 30);
    QCOMPARE(energy.output(), 14630);
    //QCOMPARE(energy.charge(), 2800);
    //QCOMPARE(energy.discharge(), 1000);
    QCOMPARE(energy.gridCharge(), 10);

    // Also verify the raw JSON with the JSON from the file.
    QFile testFile1(testData1Path);
    QVERIFY(testFile1.open(QIODevice::ReadOnly | QIODevice::Text));

    const QJsonObject testJson1 = QJsonDocument::fromJson(testFile1.readAll()).object().value(QStringLiteral("data")).toObject();
    QCOMPARE(energy.rawJson(), testJson1);

    // Now switch date, which resets everything.
    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testData2Path));

    energy.setDate(date2);
    QCOMPARE(energy.date(), date2);

    QCOMPARE(energy.status(), RequestStatus::NoRequest);
    QVERIFY(!energy.valid());

    // Now load the second dataset.
    QVERIFY(energy.reload());

    QCOMPARE(energy.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(energy.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(energy.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(energy.errorString().isEmpty());
    QVERIFY(energy.valid());

    QCOMPARE(energy.photovoltaic(), 200); // WattHours.
    QCOMPARE(energy.input(), 3500);
    QCOMPARE(energy.output(), 100);
    //QCOMPARE(energy.charge(), 100);
    //QCOMPARE(energy.discharge(), 1100);
    QCOMPARE(energy.gridCharge(), 2800);

    // Also verify the raw JSON with the JSON from the file.
    QFile testFile2(testData2Path);
    QVERIFY(testFile2.open(QIODevice::ReadOnly | QIODevice::Text));

    const QJsonObject testJson2 = QJsonDocument::fromJson(testFile2.readAll()).object().value(QStringLiteral("data")).toObject();
    QCOMPARE(energy.rawJson(), testJson2);
}

void OneDateEnergyTest::testApiError()
{
    OneDateEnergy energy(&m_connector, g_serialNumber, QDate::currentDate());

    const QString testDataPath = QFINDTESTDATA("data/api_error.json");

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    QVERIFY(energy.reload());

    QCOMPARE(energy.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(energy.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(energy.error(), QAlphaCloud::ErrorCode::ParameterError);
    QCOMPARE(energy.errorString(), QStringLiteral("Parameter error"));
    QVERIFY(!energy.valid());
}

void OneDateEnergyTest::testGarbledJson()
{
    OneDateEnergy energy(&m_connector, g_serialNumber, QDate::currentDate());

    const QString testDataPath = QFINDTESTDATA("data/garbled.json");

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    QVERIFY(energy.reload());

    QCOMPARE(energy.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(energy.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(energy.error(), QAlphaCloud::ErrorCode::JsonParseError);
    QVERIFY(!energy.valid());
}

QTEST_GUILESS_MAIN(OneDateEnergyTest)
#include "onedateenergytest.moc"
