/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QDate>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include <QAlphaCloud/QAlphaCloud>
#include <QAlphaCloud/Connector>
#include <QAlphaCloud/OneDayPowerModel>

#include "testnetworkaccessmanager.h"

using namespace QAlphaCloud;

static QString g_serialNumber = QStringLiteral("SERIAL");

class OneDayPowerModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testInitialState();
    void testRoleNames();

    void testData();
    // TODO testReload
    // TODO testCache / testForceReload

    void testApiError();
    void testGarbledJson();
    // TODO testReloadError

private:
    TestNetworkAccessManager m_networkAccessManager;
    Connector m_connector;
};

void OneDayPowerModelTest::initTestCase()
{
    // Don't rely on defaultConfiguration in tests!
    auto *configuration = new Configuration(&m_connector);
    configuration->setAppId(QStringLiteral("oneDayPowerModelApp"));
    configuration->setAppSecret(QStringLiteral("testSecret"));
    m_connector.setConfiguration(configuration);

    m_connector.setNetworkAccessManager(&m_networkAccessManager);
}

void OneDayPowerModelTest::testInitialState()
{
    {
        OneDayPowerModel model;
        QCOMPARE(model.status(), RequestStatus::NoRequest);
        QCOMPARE(model.rowCount(), 0);

        // Can't load without a connector.
        QVERIFY(!model.reload());

        const QModelIndex idx = model.index(0);
        QVERIFY(!idx.isValid());
        const QVariant indexData = idx.data();
        QVERIFY(!indexData.isValid());
        const QVariant modelData = model.data(idx);
        QVERIFY(!modelData.isValid());
    }

    {
        OneDayPowerModel model(&m_connector, QString() /*serialNumber*/, QDate());
        QCOMPARE(model.connector(), &m_connector);

        QCOMPARE(model.status(), RequestStatus::NoRequest);
        QCOMPARE(model.rowCount(), 0);

        // Can't load without a seral number.
        QVERIFY(!model.reload());

        model.setSerialNumber(g_serialNumber);
        QCOMPARE(model.serialNumber(), g_serialNumber);

        // Can't load without a date.
        QVERIFY(!model.reload());

        const QDate date = QDate(2023, 01, 01);
        model.setDate(date);
        QCOMPARE(model.date(), date);

        model.resetDate();
        QCOMPARE(model.date(), QDate::currentDate());

        QVERIFY(model.reload());
    }
}

void OneDayPowerModelTest::testRoleNames()
{
    OneDayPowerModel model;

    QList<QByteArray> expectedRoleNames{
        QByteArrayLiteral("photovoltaicEnergy"),
        QByteArrayLiteral("currentLoad"),
        QByteArrayLiteral("gridFeed"),
        QByteArrayLiteral("gridCharge"),
        QByteArrayLiteral("batterySoc"),
        QByteArrayLiteral("uploadTime"),
        QByteArrayLiteral("rawJson"),
    };
    std::sort(expectedRoleNames.begin(), expectedRoleNames.end());

    auto actualRoleNames = model.roleNames().values();
    std::sort(actualRoleNames.begin(), actualRoleNames.end());

    QCOMPARE(actualRoleNames, expectedRoleNames);
}

void OneDayPowerModelTest::testData()
{
    const QDate date(2023, 01, 01);
    OneDayPowerModel model(&m_connector, g_serialNumber, date);

    const QString testDataPath = QFINDTESTDATA("data/onedaypower.json");

    QSignalSpy countChangedSpy(&model, &OneDayPowerModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(countChangedSpy.count(), 1);

    // Also verify the raw JSON with the JSON from the file.
    QFile testFile(testDataPath);
    QVERIFY(testFile.open(QIODevice::ReadOnly | QIODevice::Text));

    const QJsonArray testJsonArray = QJsonDocument::fromJson(testFile.readAll()).object().value(QStringLiteral("data")).toArray();

    // Now compare that the data is correct in all three systems.
    using Roles = OneDayPowerModel::Roles;
    QString suffix;
    for (int i = 0; i < 3; ++i) {
        suffix = QChar('A' + i);

        const QModelIndex idx = model.index(i);
        QVERIFY(idx.isValid());

        const int photovoltaicEnergy = idx.data(static_cast<int>(Roles::PhotovoltaicEnergy)).toInt();
        QCOMPARE(photovoltaicEnergy, 3000 + 1000 * i);

        const int currentLoad = idx.data(static_cast<int>(Roles::CurrentLoad)).toInt();
        QCOMPARE(currentLoad, 1000 + 100 * i);

        const int gridFeed = idx.data(static_cast<int>(Roles::GridFeed)).toInt();
        QCOMPARE(gridFeed, 3372 + i);

        const int gridCharge = idx.data(static_cast<int>(Roles::GridCharge)).toInt();
        QCOMPARE(gridCharge, 101 + i);

        const qreal batterySoc = idx.data(static_cast<int>(Roles::BatterySoc)).toReal();
        QCOMPARE(batterySoc, 91 + i);

        const QDateTime uploadTime = idx.data(static_cast<int>(Roles::UploadTime)).toDateTime();
        QTime time(14, 59, 32);
        // Data is in 5 minute intervals:
        time = time.addSecs(i * 60 * 5);
        QCOMPARE(uploadTime, QDateTime(date, time));

        const QJsonObject rawJson = idx.data(static_cast<int>(Roles::RawJson)).toJsonObject();
        QCOMPARE(rawJson, testJsonArray.at(i).toObject());

        const QVariant invalidRole = idx.data(Qt::UserRole + 999);
        QVERIFY(!invalidRole.isValid());
    }

    // TODO Can this cause trouble with timezones?
    QCOMPARE(model.fromDateTime(), QDateTime(date, QTime(14, 59, 32)));
    QCOMPARE(model.toDateTime(), QDateTime(date, QTime(15, 9, 32)));

    QCOMPARE(model.peakPhotovoltaic(), 5000);
    QCOMPARE(model.peakLoad(), 1200);
    QCOMPARE(model.peakGridFeed(), 3374);
    QCOMPARE(model.peakGridCharge(), 103);
}

void OneDayPowerModelTest::testApiError()
{
    OneDayPowerModel model(&m_connector, g_serialNumber, QDate::currentDate());

    const QString testDataPath = QFINDTESTDATA("data/api_error.json");

    QSignalSpy countChangedSpy(&model, &OneDayPowerModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::ParameterError);
    QCOMPARE(model.errorString(), QStringLiteral("Parameter error"));
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(countChangedSpy.count(), 0);
}

void OneDayPowerModelTest::testGarbledJson()
{
    OneDayPowerModel model(&m_connector, g_serialNumber, QDate::currentDate());

    const QString testDataPath = QFINDTESTDATA("data/garbled.json");

    QSignalSpy countChangedSpy(&model, &OneDayPowerModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::JsonParseError);
    QVERIFY(!model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(countChangedSpy.count(), 0);
}

QTEST_GUILESS_MAIN(OneDayPowerModelTest)
#include "onedaypowermodeltest.moc"
