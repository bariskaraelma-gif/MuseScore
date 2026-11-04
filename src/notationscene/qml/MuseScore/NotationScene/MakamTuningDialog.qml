/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: "Makam akordu" window, the calibration of the makam tuning engine (the second tab of
 * the Makam plugin). Values are shown from the perde's natural note, in kanun mandals or cents;
 * every change retunes the open score at once.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

StyledDialogView {
    id: root

    title: "Makam akordu"

    contentWidth: 560
    contentHeight: content.implicitHeight
    margins: 16

    modal: false
    resizable: true

    MakamTuningModel {
        id: tuningModel
    }

    Component.onCompleted: {
        tuningModel.load()
    }

    readonly property string unitSymbol: tuningModel.unit === "cent" ? "sent" : "mandal"
    readonly property int valueWidth: 116

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 12

        StyledTextLabel {
            Layout.fillWidth: true
            text: tuningModel.scoreInfo
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledTextLabel {
                text: "Makam:"
            }

            StyledDropdown {
                Layout.preferredWidth: 200
                model: tuningModel.makamList
                currentIndex: indexOfValue(tuningModel.currentMakam)
                onActivated: function(index, value) {
                    tuningModel.currentMakam = value
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StyledTextLabel {
                text: "Birim:"
            }

            StyledDropdown {
                Layout.preferredWidth: 150
                model: [
                    { text: "Kanun mandalı", value: "mandal" },
                    { text: "Sent", value: "cent" }
                ]
                currentIndex: indexOfValue(tuningModel.unit)
                onActivated: function(index, value) {
                    tuningModel.unit = value
                }
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: "Değerler perdenin doğal notasından sayılır (ör. Eviç = Fa +5 mandal). Değişiklik açık esere hemen uygulanır."
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            opacity: 0.8
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: 12
            rowSpacing: 6
            visible: tuningModel.rows.length > 0

            StyledTextLabel {
                Layout.fillWidth: true
                text: "Perde"
                font: ui.theme.bodyBoldFont
                horizontalAlignment: Text.AlignLeft
            }
            StyledTextLabel {
                Layout.preferredWidth: root.valueWidth
                text: "Çıkış"
                font: ui.theme.bodyBoldFont
            }
            StyledTextLabel {
                Layout.preferredWidth: root.valueWidth
                text: "İniş"
                font: ui.theme.bodyBoldFont
            }
            StyledTextLabel {
                Layout.preferredWidth: root.valueWidth
                text: "Durak"
                font: ui.theme.bodyBoldFont
            }

            Repeater {
                model: tuningModel.rows

                delegate: Item {
                    // a Repeater in a GridLayout must create the 4 cells itself: use a row item
                    // that spans the grid
                    Layout.columnSpan: 4
                    Layout.fillWidth: true
                    implicitHeight: rowLayout.implicitHeight

                    readonly property var row: modelData

                    RowLayout {
                        id: rowLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 12

                        StyledTextLabel {
                            Layout.fillWidth: true
                            text: row.label + (row.changed ? "  •" : "")
                            horizontalAlignment: Text.AlignLeft
                        }

                        IncrementalPropertyControl {
                            id: upControl
                            Layout.preferredWidth: root.valueWidth
                            currentValue: row.up
                            step: 1
                            decimals: tuningModel.unit === "cent" ? 1 : 0
                            minValue: -1200
                            maxValue: 1200
                            measureUnitsSymbol: root.unitSymbol
                            onValueEdited: function(newValue) {
                                upControl.currentValue = newValue
                                tuningModel.setValue(row.perde, "up", newValue)
                            }
                        }

                        IncrementalPropertyControl {
                            id: downControl
                            Layout.preferredWidth: root.valueWidth
                            currentValue: row.down
                            step: 1
                            decimals: tuningModel.unit === "cent" ? 1 : 0
                            minValue: -1200
                            maxValue: 1200
                            measureUnitsSymbol: root.unitSymbol
                            onValueEdited: function(newValue) {
                                downControl.currentValue = newValue
                                tuningModel.setValue(row.perde, "down", newValue)
                            }
                        }

                        IncrementalPropertyControl {
                            id: stopControl
                            Layout.preferredWidth: root.valueWidth
                            currentValue: row.stop
                            step: 1
                            decimals: tuningModel.unit === "cent" ? 1 : 0
                            minValue: -1200
                            maxValue: 1200
                            measureUnitsSymbol: root.unitSymbol
                            onValueEdited: function(newValue) {
                                stopControl.currentValue = newValue
                                tuningModel.setValue(row.perde, "stop", newValue)
                            }
                        }
                    }
                }
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            visible: tuningModel.rows.length === 0
            text: "Bu makamda kalibre edilen perde yok: perdeleri tampere, mikrotonlar AEU'ya göre çalınır."
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StyledTextLabel {
                Layout.fillWidth: true
                text: "Yedenli Segah (La♯ yanında Si)"
                horizontalAlignment: Text.AlignLeft
            }

            IncrementalPropertyControl {
                id: yedenControl
                Layout.preferredWidth: root.valueWidth
                currentValue: tuningModel.yeden
                step: 1
                decimals: tuningModel.unit === "cent" ? 1 : 0
                minValue: -1200
                maxValue: 1200
                measureUnitsSymbol: root.unitSymbol
                onValueEdited: function(newValue) {
                    yedenControl.currentValue = newValue
                    tuningModel.setYeden(newValue)
                }
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: "• : varsayılandan farklı değer"
            horizontalAlignment: Text.AlignLeft
            opacity: 0.7
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            FlatButton {
                text: "Bu makamı varsayılana döndür"
                onClicked: {
                    tuningModel.resetCurrentMakam()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            FlatButton {
                text: "Kapat"
                accentButton: true
                onClicked: {
                    root.hide()
                }
            }
        }
    }
}
