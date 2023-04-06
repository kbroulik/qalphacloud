/*
 * SPDX-FileCopyrightText: 2023 Kai Uwe Broulik <ghqalpha@broulik.de>
 * SPDX-License-Identifier: BSD-2-Clause
 */

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.12 as QQC2

import de.broulik.qalphacloud 1.0 as QAlphaCloud

Item {
    id: root

    width: 400
    height: 400

    implicitWidth: grid.implicitWidth
    implicitHeight: grid.implicitHeight

    QAlphaCloud.Connector {
        id: cloudConnector
        configuration: QAlphaCloud.Configuration {
            // NOTE put your credentials here!
            //appId: "alpha..."
            //appSecret: "..."
        }
        Component.onCompleted: {
            storageSystems.reload();
        }
    }

    QAlphaCloud.StorageSystemsModel {
        id: storageSystems
        connector: cloudConnector
    }

    QAlphaCloud.LastPowerData {
        id: liveData
        connector: cloudConnector
        serialNumber: storageSystems.primarySerialNumber
    }

    GridLayout {
        id: grid
        columns: 2
        width: parent.width
        opacity: busy.running ? 0.5 : 1

        QQC2.Label {
            text: qsTr("System:")
        }

        RowLayout {
            Layout.fillWidth: true

            QQC2.Label {
                Layout.fillWidth: true
                visible: storageSystems.status === QAlphaCloud.QAlphaCloud.RequestStatus.Error
                text: qsTr("Error: %1").arg(storageSystems.errorString)
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: storageSystems.primarySerialNumber
            }

            QQC2.Button {
                Accessible.name: qsTr("Reload")
                icon.name: "view-refresh"
                onClicked: {
                    liveData.reload();
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            height: 10
        }

        QQC2.Label {
            text: qsTr("Photovoltaic:")
        }

        QQC2.Label {
            text: liveData.valid ? qsTr("%L1 W").arg(liveData.photovoltaicPower) : "–"
        }

        QQC2.Label {
            text: qsTr("CurrentLoad:")
        }

        QQC2.Label {
            text: liveData.valid ? qsTr("%L1 W").arg(liveData.currentLoad) : "–"
        }

        QQC2.Label {
            text: qsTr("Grid Power:")
        }

        QQC2.Label {
            text: liveData.valid ? qsTr("%L1 W").arg(liveData.gridPower) : "–"
        }

        QQC2.Label {
            text: qsTr("Battery Power:")
        }

        QQC2.Label {
            text: liveData.valid ? qsTr("%L1 W").arg(liveData.batteryPower) : "–"
        }

        QQC2.Label {
            text: qsTr("Battery SOC:")
        }

        QQC2.Label {
            text: liveData.valid ? qsTr("%L1 %").arg(liveData.batterySoc) : "–"
        }
    }

    QQC2.BusyIndicator {
        id: busy
        anchors.centerIn: parent
        running: storageSystems.status === QAlphaCloud.QAlphaCloud.RequestStatus.Loading
            || liveData.status === QAlphaCloud.QAlphaCloud.RequestStatus.Loading
    }
}
