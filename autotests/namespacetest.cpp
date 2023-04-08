/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include <QAlphaCloud/QAlphaCloud>

using namespace QAlphaCloud;

class NamespaceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testErrorText_data();
    void testErrorText();
};

void NamespaceTest::testErrorText_data()
{
    QTest::addColumn<ErrorCode>("code");
    QTest::addColumn<QVariant>("details");
    QTest::addColumn<QString>("expected");

    QTest::newRow("API error with API msg") << ErrorCode::ParameterError << QVariant(QStringLiteral("API said no.")) << QStringLiteral("API said no.");

    const QString jsonError = QStringLiteral("Unexpected , on line 2.");
    QTest::newRow("JSON parse error") << ErrorCode::JsonParseError << QVariant(jsonError)
                                      << QCoreApplication::translate("errorText", "Failed to parse JSON: %1").arg(jsonError);

    QTest::newRow("Unexpected JSON error") << ErrorCode::UnexpectedJsonDataError << QVariant(QJsonDocument())
                                           << QCoreApplication::translate("errorText", "Unexpected JSON content received.");

    QTest::newRow("QNetworkReply error") << static_cast<ErrorCode>(QNetworkReply::TimeoutError) << QVariant()
                                         << QCoreApplication::translate("errorText", "Operation timed out.");

    QJsonArray array;
    QJsonDocument arrayDoc(array);
    QTest::newRow("Unexpected JSON Array error") << ErrorCode::UnexpectedJsonDataError << QVariant(arrayDoc)
                                                 << QCoreApplication::translate("errorText", "Unexpected JSON Array received.");

    QTest::newRow("Out of bounds") << static_cast<ErrorCode>(9999) << QVariant() << QString();

    const QString outOfBounds = QStringLiteral("Out of bounds");
    QTest::newRow("Out of bounds with msg") << static_cast<ErrorCode>(9999) << QVariant(outOfBounds) << outOfBounds;
}

void NamespaceTest::testErrorText()
{
    // We don't want to test translatable strings
    // but at least some of the expected behavior.

    QFETCH(ErrorCode, code);
    QFETCH(QVariant, details);
    QFETCH(QString, expected);

    QCOMPARE(QAlphaCloud::errorText(code, details), expected);
}

QTEST_GUILESS_MAIN(NamespaceTest)
#include "namespacetest.moc"
