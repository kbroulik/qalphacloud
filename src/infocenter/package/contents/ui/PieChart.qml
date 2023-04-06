/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.1

import org.kde.kirigami 2.20 as Kirigami
//import org.kde.kquickcontrolsaddons 2.0
import org.kde.kcm 1.4 as KCM

import org.kde.quickcharts 1.0 as Charts
import org.kde.quickcharts.controls 1.0 as ChartControls
import org.kde.ksysguard.formatter 1.0 as Formatter
import org.kde.ksysguard.faces 1.0 as Faces

import de.broulik.qalphacloud 1.0 as QAlphaCloud

ChartControls.PieChartControl {
    id: pieChart

    property color color
    // TODO Qt6: Qt.alpha
    property color backgroundColor: Qt.lighter(color, 1.5)
    property color textColor: color

    property int totalValue
    property int highlightValue

    property alias title: heading.text
    property alias text: label.text


    chart.backgroundColor: pieChart.backgroundColor

    range {
        from: 0
        to: pieChart.totalValue
        automatic: false
    }

    valueSources: Charts.SingleValueSource {
        value: pieChart.highlightValue
    }

    chart.colorSource: Charts.SingleValueSource {
        value: pieChart.color
    }

    ColumnLayout {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        spacing: 0

        Kirigami.Heading {
            id: heading
            Layout.fillWidth: true
            color: pieChart.textColor
            horizontalAlignment: Text.AlignHCenter
        }

        QQC2.Label {
            id: label
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
