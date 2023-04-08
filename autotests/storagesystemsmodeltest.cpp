/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <QAlphaCloud/Connector>
#include <QAlphaCloud/QAlphaCloud>
#include <QAlphaCloud/StorageSystemsModel>

#include "testnetworkaccessmanager.h"

using namespace QAlphaCloud;

class StorageSystemsModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testInitialState();
    void testRoleNames();

    void testSingleData();
    void testMultipleData();
    void testReload();
    void testReloadSameData();
    void testCache();

    void testApiError();
    void testGarbledJson();
    void testReloadError();

private:
    TestNetworkAccessManager m_networkAccessManager;
    Connector m_connector;
};

void StorageSystemsModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    // Don't rely on defaultConfiguration in tests!
    auto *configuration = new Configuration(&m_connector);
    configuration->setAppId(QStringLiteral("storageSystemsModelApp"));
    configuration->setAppSecret(QStringLiteral("testSecret"));
    m_connector.setConfiguration(configuration);

    m_connector.setNetworkAccessManager(&m_networkAccessManager);
}

void StorageSystemsModelTest::cleanupTestCase()
{
    const QString cachePath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/qalphacloud_storagesystems.json");
    QFile::remove(cachePath);
}

void StorageSystemsModelTest::testInitialState()
{
    {
        StorageSystemsModel model;
        model.setCached(false);
        QCOMPARE(model.status(), RequestStatus::NoRequest);
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.primarySerialNumber().isEmpty());

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
        StorageSystemsModel model(&m_connector);
        model.setCached(false);
        QCOMPARE(model.connector(), &m_connector);

        QCOMPARE(model.status(), RequestStatus::NoRequest);
        QCOMPARE(model.rowCount(), 0);
        QVERIFY(model.primarySerialNumber().isEmpty());

        QVERIFY(model.reload());
    }
}

void StorageSystemsModelTest::testRoleNames()
{
    StorageSystemsModel model;
    model.setCached(false);

    QList<QByteArray> expectedRoleNames{
        QByteArrayLiteral("serialNumber"),
        QByteArrayLiteral("status"),
        QByteArrayLiteral("inverterModel"),
        QByteArrayLiteral("inverterPower"),
        QByteArrayLiteral("batteryModel"),
        QByteArrayLiteral("batteryGrossCapacity"),
        QByteArrayLiteral("batteryRemainingCapacity"),
        QByteArrayLiteral("batteryUsableCapacity"),
        QByteArrayLiteral("photovoltaicPower"),
        QByteArrayLiteral("rawJson"),
    };
    std::sort(expectedRoleNames.begin(), expectedRoleNames.end());

    auto actualRoleNames = model.roleNames().values();
    std::sort(actualRoleNames.begin(), actualRoleNames.end());

    QCOMPARE(actualRoleNames, expectedRoleNames);
}

void StorageSystemsModelTest::testSingleData()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataPath = QFINDTESTDATA("data/storagesystems_single.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countChangedSpy.count(), 1);

    const QModelIndex idx = model.index(0);
    QVERIFY(idx.isValid());

    QCOMPARE(model.primarySerialNumber(), QStringLiteral("SERIAL"));

    // Now compare that the data is correct.
    using Roles = StorageSystemsModel::Roles;
    const QString serialNumber = idx.data(static_cast<int>(Roles::SerialNumber)).toString();
    QCOMPARE(serialNumber, QStringLiteral("SERIAL"));

    const auto status = idx.data(static_cast<int>(Roles::Status)).value<QAlphaCloud::SystemStatus>();
    QCOMPARE(status, SystemStatus::Normal);

    const QString inverterModel = idx.data(static_cast<int>(Roles::InverterModel)).toString();
    QCOMPARE(inverterModel, QStringLiteral("INVERTER"));

    const int inverterPower = idx.data(static_cast<int>(Roles::InverterPower)).toInt();
    QCOMPARE(inverterPower, 10000); // Watts.

    const QString batteryModel = idx.data(static_cast<int>(Roles::BatteryModel)).toString();
    QCOMPARE(batteryModel, QStringLiteral("BATTERY"));

    const int batteryGrossCapacity = idx.data(static_cast<int>(Roles::BatteryGrossCapacity)).toInt();
    QCOMPARE(batteryGrossCapacity, 8190); // WattHours.

    const int batteryRemainingCapacity = idx.data(static_cast<int>(Roles::BatteryRemainingCapacity)).toInt();
    QCOMPARE(batteryRemainingCapacity, 7800); // WattHours.

    const int batteryUsableCapacity = idx.data(static_cast<int>(Roles::BatteryUsableCapacity)).toInt();
    QCOMPARE(batteryUsableCapacity, 95); // Percent.

    const int photovoltaicPower = idx.data(static_cast<int>(Roles::PhotovoltaicPower)).toInt();
    QCOMPARE(photovoltaicPower, 10000); // Watts.

    const QJsonObject rawJson = idx.data(static_cast<int>(Roles::RawJson)).toJsonObject();

    // Also verify the raw JSON with the JSON from the file.
    QFile testFile(testDataPath);
    QVERIFY(testFile.open(QIODevice::ReadOnly | QIODevice::Text));

    const QJsonObject testJson = QJsonDocument::fromJson(testFile.readAll()).object().value(QStringLiteral("data")).toArray().first().toObject();
    QCOMPARE(rawJson, testJson);

    const QVariant invalidRole = idx.data(Qt::UserRole + 999);
    QVERIFY(!invalidRole.isValid());
}

void StorageSystemsModelTest::testMultipleData()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataPath = QFINDTESTDATA("data/storagesystems_multiple.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(countChangedSpy.count(), 1);

    QCOMPARE(model.primarySerialNumber(), QStringLiteral("SERIALA"));

    // Now compare that the data is correct in all three systems.
    using Roles = StorageSystemsModel::Roles;
    QString suffix;
    for (int i = 0; i < 3; ++i) {
        suffix = QChar('A' + i);

        const QModelIndex idx = model.index(i);
        QVERIFY(idx.isValid());

        const QString serialNumber = idx.data(static_cast<int>(Roles::SerialNumber)).toString();
        QCOMPARE(serialNumber, QStringLiteral("SERIAL") + suffix);

        const auto status = idx.data(static_cast<int>(Roles::Status)).value<QAlphaCloud::SystemStatus>();
        QCOMPARE(status, SystemStatus::Normal);

        const QString inverterModel = idx.data(static_cast<int>(Roles::InverterModel)).toString();
        QCOMPARE(inverterModel, QStringLiteral("INV") + suffix);

        const int inverterPower = idx.data(static_cast<int>(Roles::InverterPower)).toInt();
        QCOMPARE(inverterPower, (i + 1) * 1000); // Watts.

        const QString batteryModel = idx.data(static_cast<int>(Roles::BatteryModel)).toString();
        QCOMPARE(batteryModel, QStringLiteral("BAT") + suffix);

        const int batteryGrossCapacity = idx.data(static_cast<int>(Roles::BatteryGrossCapacity)).toInt();
        QCOMPARE(batteryGrossCapacity, 2010 + i * 1000); // Watts.

        const int batteryRemainingCapacity = idx.data(static_cast<int>(Roles::BatteryRemainingCapacity)).toInt();
        QCOMPARE(batteryRemainingCapacity, 1800 + i * 1000); // WattHours.

        const int batteryUsableCapacity = idx.data(static_cast<int>(Roles::BatteryUsableCapacity)).toInt();
        QCOMPARE(batteryUsableCapacity, 91 + i); // Percent.

        const int photovoltaicPower = idx.data(static_cast<int>(Roles::PhotovoltaicPower)).toInt();
        QCOMPARE(photovoltaicPower, (i + 1) * 1000); // Watts.
    }
}

void StorageSystemsModelTest::testReload()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataSinglePath = QFINDTESTDATA("data/storagesystems_single.json");
    const QString testDataMultiplePath = QFINDTESTDATA("data/storagesystems_multiple.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataSinglePath));

    // Load our first test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countChangedSpy.count(), 1);

    // We already verified that the data stuff works in the tests before, so just check one role here.
    using Roles = StorageSystemsModel::Roles;
    const QModelIndex idx = model.index(0);
    const QString serialNumber = idx.data(static_cast<int>(Roles::SerialNumber)).toString();
    QCOMPARE(serialNumber, QStringLiteral("SERIAL"));

    // Now pretend we installed a new system, reload with multiple data.
    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataMultiplePath));

    // Load our second test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(countChangedSpy.count(), 2);

    QString suffix;
    for (int i = 0; i < 3; ++i) {
        suffix = QChar('A' + i);

        const QModelIndex idx = model.index(i);
        QVERIFY(idx.isValid());

        const QString serialNumber = idx.data(static_cast<int>(Roles::SerialNumber)).toString();
        QCOMPARE(serialNumber, QStringLiteral("SERIAL") + suffix);
    }
}

void StorageSystemsModelTest::testReloadSameData()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataPath = QFINDTESTDATA("data/storagesystems_single.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countChangedSpy.count(), 1);

    // We already verified that the data stuff works in the tests before.

    // Load our test data again.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::NoError);
    QVERIFY(model.errorString().isEmpty());
    // No change here
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countChangedSpy.count(), 1);
}

void StorageSystemsModelTest::testCache()
{
    using Roles = StorageSystemsModel::Roles;

    {
        StorageSystemsModel model(&m_connector);
        model.setCached(true);

        const QString testDataPath = QFINDTESTDATA("data/storagesystems_multiple.json");

        m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataPath));

        // Load test data.
        QVERIFY(model.reload());

        QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

        QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);

        // We already verified that the data stuff works in the tests before, so just check one role here.
        const QModelIndex idx = model.index(0);
        const QString serialNumber = idx.data(static_cast<int>(Roles::SerialNumber)).toString();
        QCOMPARE(serialNumber, QStringLiteral("SERIALA"));
    }

    {
        // Now create a new instance which should get the data we just cached.
        StorageSystemsModel model(&m_connector);
        model.setCached(true);

        // Don't call reload, the data should be there magically.
        QCOMPARE(model.status(), QAlphaCloud::RequestStatus::NoRequest);
        // Data is loaded deferred.
        QTRY_COMPARE(model.rowCount(), 3);

        const QModelIndex idx = model.index(0);
        const QString serialNumber = idx.data(static_cast<int>(Roles::SerialNumber)).toString();
        QCOMPARE(serialNumber, QStringLiteral("SERIALA"));
    }
}

void StorageSystemsModelTest::testApiError()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataPath = QFINDTESTDATA("data/api_error.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

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

void StorageSystemsModelTest::testGarbledJson()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataPath = QFINDTESTDATA("data/garbled.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

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

void StorageSystemsModelTest::testReloadError()
{
    StorageSystemsModel model(&m_connector);
    model.setCached(false);

    const QString testDataGoodPath = QFINDTESTDATA("data/storagesystems_single.json");
    const QString testDataBadPath = QFINDTESTDATA("data/garbled.json");

    QSignalSpy countChangedSpy(&model, &StorageSystemsModel::countChanged);

    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataGoodPath));

    // Load our test data.
    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Finished);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countChangedSpy.count(), 1);

    const QModelIndex idx1 = model.index(0);
    QVERIFY(idx1.isValid());

    using Roles = StorageSystemsModel::Roles;
    const QString serialNumber1 = idx1.data(static_cast<int>(Roles::SerialNumber)).toString();
    QCOMPARE(serialNumber1, QStringLiteral("SERIAL"));

    // Test that when an error occurs during reloading, existing data isn't reset.
    m_networkAccessManager.setOverrideUrl(QUrl::fromLocalFile(testDataBadPath));

    QVERIFY(model.reload());

    QCOMPARE(model.status(), QAlphaCloud::RequestStatus::Loading);

    // Got an error...
    QTRY_COMPARE(model.status(), QAlphaCloud::RequestStatus::Error);
    QCOMPARE(model.error(), QAlphaCloud::ErrorCode::JsonParseError);
    QVERIFY(!model.errorString().isEmpty());

    // ...but still our old data
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(countChangedSpy.count(), 1);

    const QModelIndex idx2 = model.index(0);
    QVERIFY(idx2.isValid());

    const QString serialNumber2 = idx1.data(static_cast<int>(Roles::SerialNumber)).toString();
    QCOMPARE(serialNumber2, QStringLiteral("SERIAL"));
}

QTEST_GUILESS_MAIN(StorageSystemsModelTest)
#include "storagesystemsmodeltest.moc"
