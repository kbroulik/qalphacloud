/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.1

import org.kde.kirigami 2.20 as Kirigami

ColumnLayout {
    property alias model: repeater.model

    Repeater {
        id: repeater

        PieLegendItem {
            color: modelData.color
            value: modelData.value
            text: modelData.text
            valid: !modelData.hasOwnProperty("valid") || modelData.valid === true
        }
    }
}
