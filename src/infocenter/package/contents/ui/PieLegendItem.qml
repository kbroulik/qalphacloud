/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.1

import org.kde.kirigami 2.20 as Kirigami

RowLayout {
    id: legendItem

    property color color
    property string value
    property alias text: label.text
    property bool checked: true
    property bool valid: true

    Layout.maximumHeight: Kirigami.Units.gridUnit * 2

    Rectangle {
        id: dot
        Layout.fillHeight: true
        width: Kirigami.Units.iconSizes.small / 2
        radius: height / 2
        color: legendItem.checked ? legendItem.color : "transparent"
        border {
            width: legendItem.checked ? 0 : 2
            color: legendItem.color
        }
    }

    ColumnLayout {
        spacing: 0

        QQC2.Label {
            id: valueLabel
            Layout.fillWidth: true
            font.bold: true
            text: legendItem.valid ? legendItem.value : qsTr("–")
            visible: text !== ""
        }

        QQC2.Label {
            id: label
            Layout.fillWidth: true
            visible: text !== ""
        }
    }
}
