/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: "Bona" window, note entry by voice. Select a measure, press Kaydet, sing the syllables
 * with the metronome after the count-in, press Dur: the notes are written from that measure on.
 * Stage 1: the rhythm only (every note is La).
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

    title: "Bona: sesle nota girişi"

    contentWidth: 480
    contentHeight: content.implicitHeight
    margins: 16

    modal: false
    resizable: true

    BonaModel {
        id: bonaModel
    }

    Component.onCompleted: {
        bonaModel.load()
    }

    onClosed: {
        bonaModel.close()
    }

    readonly property bool busy: bonaModel.state !== "idle"

    ColumnLayout {
        id: content
        anchors.fill: parent
        spacing: 12

        // Microphone and its level
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledTextLabel {
                text: "Mikrofon:"
            }

            StyledTextLabel {
                Layout.fillWidth: true
                text: bonaModel.micName !== "" ? bonaModel.micName : "—"
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 8
            radius: 4
            color: ui.theme.buttonColor

            Rectangle {
                width: parent.width * bonaModel.level
                height: parent.height
                radius: 4
                color: bonaModel.level > 0.9 ? "#d9534f" : ui.theme.accentColor
            }
        }

        // Settings
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledTextLabel {
                text: "Birim:"
            }

            StyledDropdown {
                Layout.preferredWidth: 120
                enabled: !root.busy
                model: [
                    { text: "8'lik", value: 8 },
                    { text: "16'lık", value: 16 }
                ]
                currentIndex: indexOfValue(bonaModel.grid)
                onActivated: function(index, value) {
                    bonaModel.grid = value
                }
            }

            Item {
                Layout.fillWidth: true
            }

            StyledTextLabel {
                text: "Gecikme:"
            }

            IncrementalPropertyControl {
                id: latencyControl
                Layout.preferredWidth: 110
                enabled: !root.busy
                currentValue: bonaModel.latencyMs
                step: 10
                decimals: 0
                minValue: -500
                maxValue: 500
                measureUnitsSymbol: "ms"
                onValueEdited: function(newValue) {
                    latencyControl.currentValue = newValue
                    bonaModel.latencyMs = newValue
                }
            }
        }

        // Record / Stop and the count
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            FlatButton {
                Layout.preferredWidth: 140
                Layout.preferredHeight: 48
                accentButton: true
                icon: root.busy ? IconCode.STOP : IconCode.MICROPHONE
                text: root.busy ? "Dur" : "Kaydet"
                orientation: Qt.Horizontal
                onClicked: {
                    bonaModel.toggleRecording()
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                text: bonaModel.countText
                font.pixelSize: bonaModel.state === "countin" ? 40 : 20
                font.bold: true
                horizontalAlignment: Text.AlignLeft
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: bonaModel.statusText
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        StyledTextLabel {
            Layout.fillWidth: true
            visible: bonaModel.resultText !== ""
            text: bonaModel.resultText
            font: ui.theme.bodyBoldFont
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        StyledTextLabel {
            Layout.fillWidth: true
            text: "Bu ilk sürümde yalnız ritim yazılır; bütün notalar La olur. Kulaklık takarsanız metronom kayda girmez."
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            opacity: 0.7
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            FlatButton {
                text: "Yeniden çözümle"
                enabled: bonaModel.canReanalyze
                onClicked: {
                    bonaModel.reanalyze()
                }
            }

            Item {
                Layout.fillWidth: true
            }

            FlatButton {
                text: "Kapat"
                onClicked: {
                    root.hide()
                }
            }
        }
    }
}
