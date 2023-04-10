/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.1

import org.kde.kirigami 2.20 as Kirigami
import org.kde.kcm 1.4 as KCM

import org.kde.quickcharts 1.0 as Charts
import org.kde.quickcharts.controls 1.0 as ChartControls
import org.kde.ksysguard.formatter 1.0 as Formatter
import org.kde.ksysguard.faces 1.0 as Faces

import de.broulik.qalphacloud 1.0 as QAlphaCloud
import de.broulik.qalphacloud.private.kcm 1.0 as KCMPrivate

KCM.SimpleKCM {
    id: root

    // TODO minimum size
    implicitWidth: Kirigami.Units.gridUnit * 40
    implicitHeight: Kirigami.Units.gridUnit * 22

    readonly property string currentSerialNumber: systemCombo.currentValue || ""

    property date currentDate: new Date()
    readonly property bool isToday: {
        const now = new Date();
        return currentDate.getDate() == now.getDate()
            && currentDate.getMonth() === now.getMonth()
            && currentDate.getFullYear() === now.getFullYear();
    }

    readonly property color loadBlue: "#3ea5ff"
    readonly property color batteryGreen: "#805bff3e"
    readonly property color feedPurple: "#883de9"
    readonly property color feedGreen: "#3de996"
    readonly property color gridRed: "#ff3e3e"
    readonly property color solarYellow: "#e9de3d"

    readonly property bool anyLegendHovered: [solarLegendArea, feedLegendArea, loadLegendArea, batteryLegendArea].some((area) => {
        return area.containsMouse || area.pressed;
    })

    readonly property var allSensors: [
        batterySocSource, currentLoadSource, gridChargeSource, gridFeedSource, photovoltaicEnergySource
    ]

    readonly property var enabledSensors: allSensors.filter(sensor => sensor.enabled);

    function formatAsWatt(watt : int) {
        return qsTr("%L1 W").arg(watt);
    }

    function formatAsKilowattHours(wattHours : int) {
        return qsTr("%L1 kWh").arg((wattHours / 1000).toLocaleString(Qt.locale(), 'f', 1));
    }

    function formatAsPercent(percent : real) {
        return qsTr("%L1 %").arg( (percent * 100).toLocaleString(Qt.locale(), 'f', 1));
    }

    function goDayOffset(dayOffset : int) {
        let newDate = new Date(+currentDate);
        newDate.setDate(newDate.getDate() + dayOffset);
        currentDate = newDate;
    }

    function goToday() {
        currentDate = new Date();
    }

    QAlphaCloud.Connector {
        id: cloudConnector
        configuration: QAlphaCloud.Configuration {
            id: cloudConfig
        }
        Component.onCompleted: {
            storageSystems.reload();
        }
    }

    QAlphaCloud.StorageSystemsModel {
        id: storageSystems

        property bool valid: false
        property int batteryGrossCapacity: 0
        property int batteryRemainingCapacity: 0
        property real batteryUsableCapacity: 0

        connector: cloudConnector
        onStatusChanged: {
            if (status === QAlphaCloud.QAlphaCloud.RequestStatus.Finished) {
                // liveData isn't reloaded here as it is reloaded periodically anyway
                // and live data doesn't really change if you reload it more frequently than that.
                // Also force a reload so that the reload button truly reloads.
                cumulativeData.forceReload();
                historyModel.forceReload();
            }
        }
        onModelReset: update()
        onRowsInserted: update()
        onRowsRemoved: update()
        onDataChanged: update()

        function update() {
            const idx = index(systemCombo.currentIndex, 0);
            valid = idx.valid;
            batteryGrossCapacity = data(idx, QAlphaCloud.StorageSystemsModel.Roles.BatteryGrossCapacity);
            batteryRemainingCapacity = data(idx, QAlphaCloud.StorageSystemsModel.Roles.BatteryRemainingCapacity);
            batteryUsableCapacity = data(idx, QAlphaCloud.StorageSystemsModel.Roles.BatteryUsableCapacity);
        }
    }

    QAlphaCloud.LastPowerData {
        id: liveData
        connector: cloudConnector
        serialNumber: root.currentSerialNumber
    }

    QAlphaCloud.OneDateEnergy {
        id: cumulativeData
        connector: cloudConnector
        serialNumber: root.currentSerialNumber
        date: root.currentDate
    }

    QAlphaCloud.OneDayPowerModel {
        id: historyModel
        readonly property int entriesInADay: 60 / 5 * 24
        connector: cloudConnector
        serialNumber: root.currentSerialNumber
        date: root.currentDate
    }

    Charts.ModelSource {
        id: batterySocSource
        readonly property color color: root.batteryGreen
        readonly property string name: qsTr("Battery %")
        readonly property int role: QAlphaCloud.OneDayPowerModel.Roles.BatterySoc
        // TODO remember this in a config file.
        property bool enabled: false

        model: KCMPrivate.BatterySocScaleProxyModel {
            sourceModel: historyModel
            max: historyChart.yRange.to
        }
        roleName: "batterySoc"
    }
    Charts.ModelSource {
        id: currentLoadSource
        readonly property color color: root.loadBlue
        readonly property string name: qsTr("Current Load")
        readonly property int role: QAlphaCloud.OneDayPowerModel.Roles.CurrentLoad
        property bool enabled: true

        model: historyModel
        roleName: "currentLoad"
    }
    Charts.ModelSource {
        id: gridFeedSource
        readonly property color color: root.feedPurple
        readonly property string name: qsTr("Grid Feed")
        readonly property int role: QAlphaCloud.OneDayPowerModel.Roles.GridFeed
        property bool enabled: true

        model: historyModel
        roleName: "gridFeed"
    }
    Charts.ModelSource {
        id: gridChargeSource
        readonly property color color: root.gridRed
        readonly property string name: qsTr("Grid Charge")
        readonly property int role: QAlphaCloud.OneDayPowerModel.Roles.GridCharge
        property bool enabled: true

        model: historyModel
        roleName: "gridCharge"
    }
    Charts.ModelSource {
        id: photovoltaicEnergySource
        readonly property color color: root.solarYellow
        readonly property string name: qsTr("Photovoltaic")
        readonly property int role: QAlphaCloud.OneDayPowerModel.Roles.PhotovoltaicEnergy
        property bool enabled: true

        model: historyModel
        roleName: "photovoltaicEnergy"
    }

    Timer {
        interval: 10000
        running: Qt.application.active
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            liveData.reload();
        }
        onRunningChanged: {
            root.currentDate = new Date();
        }
    }

    component ReloadAction : QQC2.Action {
        text: qsTr("Reload")
        icon.name: "view-refresh"
    }

    header: RowLayout {
        // HACK add padding
        Item {
            width: 1
            height: 1
        }

        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: liveDataBusy.width
            visible: liveDataBusy.running

            QQC2.BusyIndicator {
                id: liveDataBusy
                height: parent.height
                running: !liveData.valid && liveData.status === QAlphaCloud.QAlphaCloud.RequestStatus.Loading
            }
        }

        // TODO show text when loading live data failed.

        RowLayout {
            opacity: liveData.valid ? 1 : 0
            spacing: 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.shortDuration
                }
            }

            MouseArea {
                id: solarLegendArea
                Layout.preferredWidth: solarLegendItem.implicitWidth + 10
                Layout.preferredHeight: solarLegendItem.implicitHeight
                hoverEnabled: true

                PieLegendItem {
                    id: solarLegendItem
                    anchors.fill: parent
                    color: root.solarYellow
                    text: qsTr("Photovoltaic")
                    value: root.formatAsWatt(liveData.photovoltaicPower)
                    opacity: !root.anyLegendHovered || parent.containsMouse || parent.pressed ? 1 : 0.5
                }
            }

            MouseArea {
                id: feedLegendArea
                Layout.preferredWidth: feedLegendItem.implicitWidth + 10
                Layout.preferredHeight: feedLegendItem.implicitHeight
                hoverEnabled: true

                PieLegendItem {
                    id: feedLegendItem
                    color: root.feedPurple
                    text: liveData.gridPower < 0 ? qsTr("Grid Feed") : qsTr("Grid Consumption")
                    value: root.formatAsWatt(Math.abs(liveData.gridPower))
                    opacity: !root.anyLegendHovered || parent.containsMouse || parent.pressed ? 1 : 0.5
                }
            }

            MouseArea {
                id: loadLegendArea
                Layout.preferredWidth: loadLegendItem.implicitWidth + 10
                Layout.preferredHeight: loadLegendItem.implicitHeight
                hoverEnabled: true

                PieLegendItem {
                    id: loadLegendItem
                    color: root.loadBlue
                    text: qsTr("Current Load")
                    // This sometimes gets negative.
                    value: root.formatAsWatt(Math.max(0, liveData.currentLoad))
                    opacity: !root.anyLegendHovered || parent.containsMouse || parent.pressed ? 1 : 0.5
                }
            }

            MouseArea {
                id: batteryLegendArea
                Layout.preferredWidth: batteryLegendItem.implicitWidth + 10
                Layout.preferredHeight: batteryLegendItem.implicitHeight
                hoverEnabled: true

                PieLegendItem {
                    id: batteryLegendItem
                    color: root.feedGreen
                    // TODO is this really what the values mean?
                    text: {
                        if (liveData.batteryPower < 0) {
                            return qsTr("Battery, Charging");
                        } else if (liveData.batteryPower > 0) {
                            return qsTr("Battery, Discharging")
                        } else {
                            return qsTr("Battery");
                        }
                    }
                    value: {
                        const batteryPercent = root.formatAsPercent(liveData.batterySoc / 100);

                        if (liveData.batteryPower !== 0) {
                            return qsTr("%1 (%2)").arg(root.formatAsWatt(Math.abs(liveData.batteryPower)))
                                                  .arg(batteryPercent);
                        } else {
                            return batteryPercent;
                        }
                    }
                    opacity: !root.anyLegendHovered || parent.containsMouse || parent.pressed ? 1 : 0.5
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        ColumnLayout {
            Layout.topMargin: 5
            Layout.bottomMargin: 5
            // As long as we have data, live or cached.
            opacity: storageSystems.count > 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: Kirigami.Units.shortDuration
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                QQC2.Label {
                    text: systemCombo.Accessible.name
                    visible: systemCombo.count > 0
                }

                // Single system display
                QQC2.Label {
                    text: root.currentSerialNumber
                    textFormat: Text.PlainText
                    visible: systemCombo.count === 1
                }

                // Multiple system combo
                QQC2.ComboBox {
                    id: systemCombo
                    Accessible.name: qsTr("System:")
                    model: storageSystems
                    textRole: "serialNumber"
                    valueRole: "serialNumber"
                    currentIndex: 0
                    visible: count > 1
                }

                QQC2.Button {
                    Accessible.name: qsTr("Reload")
                    icon.name: "view-refresh"
                    onClicked: {
                        cumulativeData.reset();
                        historyModel.reset();
                        storageSystems.reload()
                    }

                    QQC2.ToolTip {
                        text: parent.Accessible.name
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight

                QQC2.Button {
                    Accessible.name: qsTr("Go to previous day")
                    // TODO RTL language
                    icon.name: "go-previous"
                    onClicked: {
                        root.goDayOffset(-1);
                    }

                    QQC2.ToolTip {
                        text: parent.Accessible.name
                    }
                }

                QQC2.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                    horizontalAlignment: Text.AlignHCenter
                    text: root.currentDate.toDateString()
                    // TODO on click open calendar
                }

                QQC2.Button {
                    Accessible.name: qsTr("Go to next day")
                    icon.name: "go-next"
                    enabled: !root.isToday
                    onClicked: {
                        root.goDayOffset(+1);
                    }

                    QQC2.ToolTip {
                        text: parent.Accessible.name
                    }
                }

                QQC2.Button {
                    Accessible.name: qsTr("Go to today")
                    icon.name: "go-jump-today"
                    enabled: !root.isToday
                    onClicked: {
                        root.goToday();
                    }

                    QQC2.ToolTip {
                        text: parent.Accessible.name
                    }
                }
            }
        }

        // HACK add padding
        Item {
            width: 1
            height: 1
        }
    }

    QQC2.BusyIndicator {
        anchors.centerIn: parent
        running: storageSystems.count === 0 && storageSystems.status === QAlphaCloud.QAlphaCloud.RequestStatus.Loading
    }

    Kirigami.PlaceholderMessage {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        icon.name: "dialog-error"
        text: qsTr("Invalid configuration")
        explanation: qsTr("There has been no valid API URL, application ID, or application secret provided in the configuration file.")
        visible: !cloudConfig.valid
    }

    Kirigami.PlaceholderMessage {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        icon.name: "dialog-error"
        text: qsTr("Failed to load storage systems")
        explanation: storageSystems.errorString
        visible: storageSystems.count === 0 && storageSystems.status === QAlphaCloud.QAlphaCloud.RequestStatus.Error
    }

    RowLayout {
        anchors.fill: parent
        opacity: cloudConfig.valid && storageSystems.count > 0
        Behavior on opacity {
            OpacityAnimator {
                duration: Kirigami.Units.shortDuration
            }
        }

        // Cumulative data section
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 2

            QQC2.BusyIndicator {
                id: cumulativeBusy
                anchors.centerIn: parent
                running: !cumulativeData.valid && cumulativeData.status === QAlphaCloud.QAlphaCloud.RequestStatus.Loading
            }

            Kirigami.PlaceholderMessage {
                anchors.fill: parent
                visible: !cumulativeData.valid && historyModel.status === QAlphaCloud.QAlphaCloud.RequestStatus.Finished
                text: qsTr("No cumulative data available for this date.")
            }

            Kirigami.PlaceholderMessage {
                anchors.fill: parent
                visible: !cumulativeData.valid && cumulativeData.status === QAlphaCloud.QAlphaCloud.RequestStatus.Error
                icon.name: "dialog-error"
                text: qsTr("Failed to load cumulative data.")
                explanation: cumulativeData.errorString
                helpfulAction: ReloadAction {
                    onTriggered: {
                        cumulativeData.reload();
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                visible: cumulativeData.valid

                ColumnLayout {
                    id: cumulativeColumn
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1

                    PieChart {
                        id: loadPie
                        readonly property int selfSufficient: cumulativeData.totalLoad - cumulativeData.input

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.loadBlue
                        totalValue: Math.max(1, cumulativeData.totalLoad)
                        highlightValue: selfSufficient
                        title: root.formatAsKilowattHours(cumulativeData.totalLoad)
                        text: qsTr("Consumed")
                    }

                    PieLegend {
                        Layout.fillWidth: true
                        model: [{
                            text: qsTr("Self-sufficient"),
                            color: loadPie.color,
                            value: root.formatAsPercent(loadPie.selfSufficient / cumulativeData.totalLoad),
                            valid: cumulativeData.totalLoad > 100
                        }, {
                            text: qsTr("Grid Use"),
                            color: loadPie.backgroundColor,
                            value: qsTr("%1 – %2").arg(root.formatAsPercent(cumulativeData.input / cumulativeData.totalLoad))
                                                  .arg(root.formatAsKilowattHours(cumulativeData.input)),
                            valid: cumulativeData.totalLoad > 100
                        }]
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1

                    PieChart {
                        id: solarPie
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.solarYellow
                        // TODO dark mode
                        textColor: Qt.darker(root.solarYellow, 1.3)
                        // Use at least 1 so the chart doesn't completely disappear.
                        totalValue: Math.max(1, cumulativeData.photovoltaic)
                        highlightValue: Math.max(0, cumulativeData.photovoltaic - cumulativeData.output)
                        title: root.formatAsKilowattHours(cumulativeData.photovoltaic)
                        text: qsTr("Produced")
                    }

                    PieLegend {
                        Layout.fillWidth: true
                        model: [{
                            text: qsTr("Own Use"),
                            color: solarPie.color,
                            value: root.formatAsPercent((cumulativeData.photovoltaic - cumulativeData.output) / cumulativeData.photovoltaic),
                            valid: cumulativeData.photovoltaic > 100
                        }, {
                            text: qsTr("Grid Feed"),
                            color: solarPie.backgroundColor,
                            value: qsTr("%1 – %2").arg(root.formatAsPercent(cumulativeData.output / cumulativeData.photovoltaic))
                                                  .arg(root.formatAsKilowattHours(cumulativeData.output)),
                            valid: cumulativeData.photovoltaic > 100
                        }]
                    }
                }
            }
        }

        QQC2.Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 3
            Layout.preferredHeight: chartColumn.implicitHeight

            background: Rectangle {
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                Kirigami.Theme.inherit: false
                color: Kirigami.Theme.backgroundColor
                readonly property color borderColor: Kirigami.Theme.textColor
                border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)
            }

            QQC2.BusyIndicator {
                anchors.centerIn: parent
                z: 2
                running: historyModel.count === 0 && historyModel.status === QAlphaCloud.QAlphaCloud.RequestStatus.Loading
            }

            Kirigami.PlaceholderMessage {
                anchors.fill: parent
                visible: historyModel.count === 0 && historyModel.status === QAlphaCloud.QAlphaCloud.RequestStatus.Finished
                text: qsTr("No historic data available for this date.")
            }

            Kirigami.PlaceholderMessage {
                anchors.fill: parent
                visible: historyModel.count === 0 && historyModel.status === QAlphaCloud.QAlphaCloud.RequestStatus.Error
                icon.name: "dialog-error"
                text: qsTr("Failed to load history data.")
                explanation: historyModel.errorString
                helpfulAction: ReloadAction {
                    onTriggered: {
                        historyModel.reload();
                    }
                }
            }

            ColumnLayout {
                id: chartColumn
                anchors {
                    fill: parent
                    topMargin: 5
                }
                visible: historyModel.count > 0

                GridLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Charts.AxisLabels {
                        Layout.fillHeight: true
                        Layout.column: 0
                        Layout.row: 0
                        constrainToBounds: false
                        direction: Charts.AxisLabels.VerticalBottomTop
                        delegate: QQC2.Label {
                            anchors.right: parent.right
                            font: Kirigami.Theme.smallFont
                            // don't use ShowNull.
                            text: Formatter.Formatter.formatValue(Charts.AxisLabels.label,
                                Formatter.Units.UnitWatts)
                            color: Kirigami.Theme.disabledTextColor
                        }
                        source: Charts.ChartAxisSource {
                            chart: historyChart
                            axis: Charts.ChartAxisSource.YAxis
                            itemCount: 5
                        }
                    }

                    Charts.LineChart {
                        id: historyChart
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.row: 0
                        Layout.column: 1

                        stacked: false
                        smooth: false
                        fillOpacity: 0.1

                        xRange {
                            // FIXME Take into account fromDate.
                            from: 0
                            // There's an entry every five minutes.
                            to: Math.max(historyModel.count, historyModel.entriesInADay)
                            automatic: false
                        }
                        yRange {
                            from: 0
                            to: {
                                const peak = Math.max(historyModel.peakPhotovoltaic,
                                    historyModel.peakLoad,
                                    historyModel.peakGridFeed,
                                    historyModel.peakGridCharge);
                                return Math.ceil(peak / 1000) * 1000;
                            }
                            automatic: false
                        }

                        valueSources: {
                            if (solarLegendArea.containsMouse || solarLegendArea.pressed) {
                                return [photovoltaicEnergySource];
                            } else if (feedLegendArea.containsMouse || feedLegendArea.pressed) {
                                return [gridFeedSource, gridChargeSource];
                            } else if (loadLegendArea.containsMouse || loadLegendArea.pressed) {
                                return [currentLoadSource];
                            } else if (batteryLegendArea.containsMouse || batteryLegendArea.pressed) {
                                return [batterySocSource];
                            } else {
                                return root.enabledSensors;
                            }
                        }

                        colorSource: Charts.ArraySource {
                            array: {
                                const sources = historyChart.valueSources;

                                // TODO Array.filter in Qt 6.
                                let colors = [];
                                for (let i = 0; i < sources.length; ++i) {
                                    colors.push(sources[i].color);
                                }
                                return colors;
                            }
                        }

                        // Horizontal Lines
                        Charts.GridLines {
                            id: horizontalLines
                            direction: Charts.GridLines.Vertical
                            anchors.fill: parent
                            z: -1
                            chart: historyChart

                            major.count: 3
                            major.lineWidth: 1
                            // The same color as a Kirigami.Separator
                            major.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.2)
                            minor.visible: false
                            minor.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.05)
                        }

                        // Vertical Lines
                        Charts.GridLines {
                            direction: Charts.GridLines.Horizontal
                            anchors.fill: parent
                            z: -1
                            chart: historyChart

                            major.count: 3
                            major.lineWidth: 1
                            // The same color as a Kirigami.Separator
                            major.color: horizontalLines.major.color
                            minor.visible: true
                            minor.count: 23
                            minor.color: horizontalLines.minor.color
                            minor.lineWidth: 1
                        }

                        MouseArea {
                            id: chartArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }

                        QQC2.ToolTip {
                            id: chartToolTip
                            readonly property real hitAreaWidth: chartArea.width / historyModel.entriesInADay
                            readonly property var modelIndex: {
                                const row = chartArea.mouseX / hitAreaWidth - hitAreaWidth / 2;
                                if (row < historyModel.count) {
                                    return historyModel.index(row, 0);
                                } else {
                                    return null;
                                }
                            }

                            readonly property var uploadTime: modelIndex && modelIndex.valid ? historyModel.data(modelIndex, QAlphaCloud.OneDayPowerModel.Roles.UploadTime) : null

                            // QQC2 ToolTip auto-positioning is borked...
                            x: chartArea.mouseX > chartArea.width - width ? chartArea.mouseX - width : chartArea.mouseX
                            y: chartArea.mouseY > chartArea.height - height ? chartArea.mouseY - height : chartArea.mouseY
                            visible: chartArea.containsMouse && modelIndex && modelIndex.valid

                            TextMetrics {
                                id: toolTipMetrics
                                text: root.formatAsWatt(10000)
                                font.bold: true
                            }

                            ColumnLayout {
                                Kirigami.ListSectionHeader {
                                    Layout.fillWidth: true
                                    padding: 0
                                    topPadding: 0
                                    text: chartToolTip.uploadTime ? chartToolTip.uploadTime.toLocaleTimeString(Qt.locale(), Locale.ShortFormat) : ""
                                }

                                Repeater {
                                    model: root.enabledSensors

                                    RowLayout {
                                        id: toolTipValueRow
                                        readonly property var value: chartToolTip.modelIndex && chartToolTip.modelIndex.valid ? historyModel.data(chartToolTip.modelIndex, modelData.role) : 0
                                        Layout.fillWidth: true

                                        LegendDot {
                                            Layout.fillHeight: true
                                            fillColor: modelData.color
                                        }

                                        QQC2.Label {
                                            Layout.fillWidth: true
                                            text: modelData.name
                                        }

                                        QQC2.Label {
                                            Layout.fillWidth: true
                                            Layout.minimumWidth: toolTipMetrics.advanceWidth
                                            horizontalAlignment: Text.AlignRight
                                            text: {
                                                if (modelData.role === QAlphaCloud.OneDayPowerModel.Roles.BatterySoc) {
                                                    return root.formatAsPercent(toolTipValueRow.value / 100);
                                                } else {
                                                    return root.formatAsWatt(toolTipValueRow.value);
                                                }
                                            }
                                            font.bold: true
                                            opacity: toolTipValueRow.value > 0 ? 1 : 0.3
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Charts.AxisLabels {
                        id: xAxisLabels
                        Layout.fillWidth: true
                        Layout.row: 1
                        Layout.column: 1
                        constrainToBounds: true
                        direction: Charts.AxisLabels.HorizontalLeftRight
                        delegate: QQC2.Label {
                            font: Kirigami.Theme.smallFont
                            text: qsTr("%1:00").arg(Math.round(Charts.AxisLabels.label / 12))
                            color: Kirigami.Theme.disabledTextColor
                        }
                        source: Charts.ChartAxisSource {
                            chart: historyChart
                            axis: Charts.ChartAxisSource.XAxis
                            itemCount: 5
                        }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3

                    Repeater {
                        model: root.allSensors

                        QQC2.Button {
                            id: legendButton
                            Layout.fillWidth: true
                            checkable: true
                            checked: {
                                const sources = historyChart.valueSources;

                                // TODO Array.find in Qt 6
                                for (let i = 0; i < sources.length; ++i) {
                                    if (sources[i] === modelData) {
                                        return true;
                                    }
                                }
                                return false;
                            }
                            text: modelData.name
                            onToggled: {
                                modelData.enabled = checked
                            }

                            background: PieLegendItem {
                                id: legendItem
                                color: modelData.color
                                text: legendButton.text
                                checked: legendButton.checked
                            }
                        }
                    }
                }
            }
        }
    }
}
