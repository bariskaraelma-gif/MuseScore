/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the Finale-like tool bar on the left of the score, shown when Preferences >
 * Appearance > Arayüz is "Finale benzeri". Two columns of tools in groups; each tool opens its
 * palettes in a popup, the Makam and Usul tools open theirs again on every measure clicked
 * while they are on. The note input tool shows a column of the AEU accidentals beside the bar.
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
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene
import MuseScore.Palette

Item {
    id: root

    visible: toolsModel.finaleMode
    width: visible ? toolBar.width + (accidentalColumn.visible ? accidentalColumn.width : 0) : 0

    readonly property int buttonSize: 36
    readonly property int buttonSpacing: 4
    readonly property int groupSpacing: 14
    readonly property int sideMargin: 6

    // SMuFL code points of the drawn icons
    readonly property string reversedFlat: ""   // koma bemol
    readonly property string slashedFlat: ""    // bakiye bemol
    readonly property string timeSig5: ""
    readonly property string timeSig8: ""

    // icon: an IconCode; drawn: "makam" or "usul" (music font glyphs)
    readonly property var toolGroups: [
        [
            { id: "selection", title: "Seçim", icon: IconCode.PAN_SCORE,
              description: "Nota girişini kapatır; tıklayarak seçersiniz" },
            { id: "noteinput", title: "Nota girişi", icon: IconCode.NOTE_PLUS,
              description: "Nota girişini açar; yanında makam arızaları" }
        ],
        [
            { id: "makam", title: "Makam", drawn: "makam",
              description: "Makam donanımları. Bu araç açıkken tıkladığınız ölçüde kutu kendiliğinden açılır" },
            { id: "usul", title: "Usul", drawn: "usul",
              description: "Usuller. Bu araç açıkken tıkladığınız ölçüde kutu kendiliğinden açılır" },
            { id: "barline", title: "Ölçü çizgisi", icon: IconCode.DOUBLE_BAR_LINE,
              description: "Ölçü çizgileri" },
            { id: "layout", title: "Yerleşim", icon: IconCode.LINE_BREAK,
              description: "Satır sonu, sayfa sonu, boşluklar" }
        ],
        [
            { id: "articulation", title: "Artikülasyon", icon: IconCode.ARTICULATION,
              description: "Artikülasyonlar" },
            { id: "ornament", title: "Süsleme", icon: IconCode.ORNAMENT,
              description: "Süslemeler, nefes ve durak" },
            { id: "dynamics", title: "Nüans", icon: IconCode.DYNAMIC_FORTE,
              description: "Nüanslar, kreşendo ve dekreşendo" },
            { id: "tempo", title: "Tempo", icon: IconCode.METRONOME,
              description: "Tempo işaretleri" },
            { id: "text", title: "Metin", icon: IconCode.AUTO_TEXT,
              description: "Metinler (Hane, Teslim, Saz…)" },
            { id: "lyrics", title: "Söz", icon: IconCode.LYRICS,
              description: "Seçili notanın altına söz yazmaya başlar" },
            { id: "repeat", title: "Tekrar", icon: IconCode.REPEAT_START,
              description: "Tekrarlar ve atlamalar" },
            { id: "allpalettes", title: "Bütün paletler", icon: IconCode.GRID,
              description: "Palet panelini açar ya da kapatır (kiriş, süsleme notası, çizgiler…)" }
        ]
    ]

    MakamToolsModel {
        id: toolsModel
    }

    Component.onCompleted: {
        toolsModel.load()
    }

    property var toolButtons: ({})

    function toolTitle(toolId) {
        for (let g = 0; g < root.toolGroups.length; ++g) {
            for (let i = 0; i < root.toolGroups[g].length; ++i) {
                if (root.toolGroups[g][i].id === toolId) {
                    return root.toolGroups[g][i].title
                }
            }
        }
        return ""
    }

    Connections {
        target: toolsModel

        function onPopupRequested(tool) {
            const button = root.toolButtons[tool]
            if (button) {
                palettePopup.openFor(tool, root.toolTitle(tool), button)
            }
        }
    }

    ToolPalettePopup {
        id: palettePopup

        onOpened: {
            toolsModel.setPopupOpen(true)
        }

        onClosed: {
            toolsModel.setPopupOpen(false)
        }
    }

    Rectangle {
        id: toolBar

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2 * root.buttonSize + root.buttonSpacing + 2 * root.sideMargin

        color: ui.theme.backgroundPrimaryColor

        Rectangle {
            // the line on the right of the tool bar
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: ui.theme.strokeColor
        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 6
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.groupSpacing

            Repeater {
                model: root.toolGroups

                delegate: Grid {
                    id: group

                    required property var modelData

                    columns: 2
                    spacing: root.buttonSpacing

                    Repeater {
                        model: group.modelData

                        // a tool button: an icon of the UI font, or glyphs of the music font
                        delegate: FlatButton {
                            id: toolButton

                            required property var modelData
                            readonly property var tool: modelData

                            width: root.buttonSize
                            height: root.buttonSize

                            enabled: toolsModel.hasNotation
                            icon: tool.icon !== undefined ? tool.icon : IconCode.NONE
                            toolTipTitle: tool.title
                            toolTipDescription: tool.description

                            accentButton: toolsModel.activeTool === tool.id
                            transparent: !accentButton

                            Component.onCompleted: {
                                root.toolButtons[tool.id] = toolButton
                            }

                            onClicked: {
                                if (tool.id === "allpalettes") {
                                    toolsModel.showAllPalettes()
                                } else {
                                    toolsModel.selectTool(tool.id)
                                }
                            }

                            // Makam: a reversed flat low on the left, a slashed flat high on the right
                            Item {
                                anchors.fill: parent
                                visible: toolButton.tool.drawn === "makam"

                                Text {
                                    x: 7
                                    y: 27 - baselineOffset
                                    text: root.reversedFlat
                                    color: ui.theme.fontPrimaryColor
                                    font.family: ui.theme.musicalFont.family
                                    font.pixelSize: 28
                                }

                                Text {
                                    x: 17
                                    y: 19 - baselineOffset
                                    text: root.slashedFlat
                                    color: ui.theme.fontPrimaryColor
                                    font.family: ui.theme.musicalFont.family
                                    font.pixelSize: 28
                                }
                            }

                            // Usul: 5 over 8, bold, without a fraction line
                            Item {
                                anchors.fill: parent
                                visible: toolButton.tool.drawn === "usul"

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 11 - baselineOffset
                                    text: root.timeSig5
                                    color: ui.theme.fontPrimaryColor
                                    font.family: ui.theme.musicalFont.family
                                    font.pixelSize: 28
                                    font.bold: true
                                }

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 25 - baselineOffset
                                    text: root.timeSig8
                                    color: ui.theme.fontPrimaryColor
                                    font.family: ui.theme.musicalFont.family
                                    font.pixelSize: 28
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // The accidentals of note input: naturel on top, sharps on the left, flats on the right
    Rectangle {
        id: accidentalColumn

        anchors.left: toolBar.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2 * root.buttonSize + root.buttonSpacing + 2 * root.sideMargin

        visible: toolsModel.activeTool === "noteinput"
        color: ui.theme.backgroundPrimaryColor

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: ui.theme.strokeColor
        }

        // the cells of the two columns, row by row (null: an empty cell)
        function rows() {
            const all = toolsModel.accidentals
            const naturals = all.filter(a => a.column === "natural")
            const sharps = all.filter(a => a.column === "sharp")
            const flats = all.filter(a => a.column === "flat")
            let cells = []
            for (const n of naturals) {
                cells.push(n, null)
            }
            for (let i = 0; i < Math.max(sharps.length, flats.length); ++i) {
                cells.push(i < sharps.length ? sharps[i] : null, i < flats.length ? flats[i] : null)
            }
            return cells
        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 6
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.buttonSpacing

            // naturel alone on the first row, then sharp | flat pairs from koma to büyük mücennep
            Grid {
                columns: 2
                spacing: root.buttonSpacing

                Repeater {
                    model: accidentalColumn.rows()

                    delegate: FlatButton {
                        id: accidentalButton

                        required property var modelData

                        width: root.buttonSize
                        height: root.buttonSize

                        opacity: modelData ? 1 : 0
                        enabled: Boolean(modelData) && toolsModel.hasNotation
                        toolTipTitle: modelData ? modelData.title : ""
                        transparent: true

                        onClicked: {
                            toolsModel.applyAccidental(accidentalButton.modelData.id)
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            y: 23 - baselineOffset
                            text: accidentalButton.modelData ? accidentalButton.modelData.glyph : ""
                            color: ui.theme.fontPrimaryColor
                            font.family: ui.theme.musicalFont.family
                            font.pixelSize: 26
                        }
                    }
                }
            }
        }
    }
}
