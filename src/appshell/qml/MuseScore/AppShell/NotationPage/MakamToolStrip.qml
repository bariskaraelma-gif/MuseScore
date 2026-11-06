/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the Finale-like tool bar on the left of the score, shown when Preferences >
 * Appearance > Arayüz is "Finale benzeri". Two columns of tools in groups. Each tool shows a
 * column beside the bar: the AEU accidentals for note input, its palette for the others; the
 * mızrap buttons put a strum arrow above the selected notes; Özellikler floats the Properties
 * panel as a small window.
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
    width: visible ? toolBar.width + (sideColumn.visible ? sideColumn.width : 0) : 0

    readonly property int buttonSize: 36
    readonly property int buttonSpacing: 4
    readonly property int groupSpacing: 14
    readonly property int sideMargin: 6

    // SMuFL code points of the drawn icons
    readonly property string reversedFlat: ""   // koma bemol
    readonly property string slashedFlat: ""    // bakiye bemol
    readonly property string timeSig5: ""
    readonly property string timeSig8: ""

    // icon: an IconCode; drawn: "makam" or "usul" (music font glyphs) or "label" (the title);
    // action: a button that does something at once instead of choosing a tool
    readonly property var toolGroups: [
        [
            { id: "selection", title: "Seçim", icon: IconCode.PAN_SCORE,
              description: "Nota girişini kapatır; tıklayarak seçersiniz" },
            { id: "noteinput", title: "Nota girişi", icon: IconCode.NOTE_PLUS,
              description: "Nota girişini açar; yanında makam arızaları" }
        ],
        [
            { id: "makam", title: "Makam", drawn: "makam",
              description: "Makam donanımları yanda açılır. Ölçüyü seçip donanıma tıklayın" },
            { id: "usul", title: "Usul", drawn: "usul",
              description: "Usuller yanda açılır. Ölçüyü seçip usule tıklayın" },
            { id: "barline", title: "Ölçü çizgisi", icon: IconCode.DOUBLE_BAR_LINE,
              description: "Ölçü çizgileri" },
            { id: "layout", title: "Yerleşim", icon: IconCode.LINE_BREAK,
              description: "Satır sonu, sayfa sonu, boşluklar" }
        ],
        [
            { id: "mizrap-down", title: "Mızrap aşağı", icon: IconCode.ARROW_DOWN, action: true,
              description: "Seçili notaların üstüne aşağı ok koyar (bir daha basınca kalkar)" },
            { id: "mizrap-up", title: "Mızrap yukarı", icon: IconCode.ARROW_UP, action: true,
              description: "Seçili notaların üstüne yukarı ok koyar (bir daha basınca kalkar)" },
            { id: "articulation", title: "Artikülasyon", icon: IconCode.ARTICULATION,
              description: "Artikülasyonlar" },
            { id: "dynamics", title: "Nüans", icon: IconCode.DYNAMIC_FORTE,
              description: "Nüanslar, kreşendo ve dekreşendo" },
            { id: "text", title: "Metin", drawn: "label",
              description: "Metinler (Hane, Teslim, Saz…); Özellikler penceresi de açılır" },
            { id: "lyrics", title: "Söz", icon: IconCode.LYRICS, action: true,
              description: "Seçili notanın altına söz yazmaya başlar; Özellikler penceresi de açılır" },
            { id: "repeat", title: "Tekrar", icon: IconCode.REPEAT_START,
              description: "Tekrarlar ve atlamalar" }
        ],
        [
            { id: "properties", title: "Özellikler", icon: IconCode.CONFIGURE, action: true,
              description: "Seçili öğenin özelliklerini küçük bir pencerede açar ya da kapatır" },
            { id: "allpalettes", title: "Bütün paletler", icon: IconCode.GRID, action: true,
              description: "Palet panelini açar ya da kapatır (kiriş, süsleme notası, çizgiler…)" }
        ]
    ]

    MakamToolsModel {
        id: toolsModel
    }

    Component.onCompleted: {
        toolsModel.load()
    }

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

    function onToolClicked(toolId) {
        if (toolId === "allpalettes") {
            toolsModel.showAllPalettes()
        } else if (toolId === "properties") {
            toolsModel.toggleProperties()
        } else if (toolId === "mizrap-down" || toolId === "mizrap-up") {
            toolsModel.addMizrap(toolId === "mizrap-down")
        } else {
            toolsModel.selectTool(toolId)
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

                            accentButton: !tool.action && toolsModel.activeTool === tool.id
                            transparent: !accentButton

                            onClicked: {
                                root.onToolClicked(tool.id)
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

                            // Usul: 5 over 8 without a fraction line, the time signature digits of the
                            // music font (as in the Usul palette)
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
                                }

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    y: 25 - baselineOffset
                                    text: root.timeSig8
                                    color: ui.theme.fontPrimaryColor
                                    font.family: ui.theme.musicalFont.family
                                    font.pixelSize: 28
                                }
                            }

                            // the title as the icon (Metin)
                            StyledTextLabel {
                                anchors.centerIn: parent
                                visible: toolButton.tool.drawn === "label"
                                text: toolButton.tool.title
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
    }

    // The column beside the tool bar: the accidentals of note input, or the palette of the tool
    Rectangle {
        id: sideColumn

        readonly property bool showsAccidentals: toolsModel.activeTool === "noteinput"
        readonly property bool showsPalette: toolsModel.activeTool !== "noteinput" && toolsModel.activeTool !== "selection"

        anchors.left: toolBar.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: (showsPalette ? paletteColumn.implicitWidth : 2 * root.buttonSize + root.buttonSpacing) + 2 * root.sideMargin

        visible: showsAccidentals || showsPalette
        color: ui.theme.backgroundPrimaryColor

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: ui.theme.strokeColor
        }

        // the cells of the two accidental columns, row by row (null: an empty cell): naturel
        // alone on the first row, then sharp | flat pairs from koma to büyük mücennep
        function accidentalCells() {
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

        Grid {
            anchors.top: parent.top
            anchors.topMargin: 6
            anchors.horizontalCenter: parent.horizontalCenter
            visible: sideColumn.showsAccidentals

            columns: 2
            spacing: root.buttonSpacing

            Repeater {
                model: sideColumn.accidentalCells()

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

        ToolPaletteColumn {
            id: paletteColumn

            anchors.top: parent.top
            anchors.topMargin: 6
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: implicitWidth

            visible: sideColumn.showsPalette
            tool: sideColumn.showsPalette ? toolsModel.activeTool : ""
            title: root.toolTitle(tool)
        }
    }
}
