/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the Finale-like tool bar on the left of the score, shown when Preferences >
 * Appearance > Arayüz is "Finale benzeri". Two columns of tools in groups. Each tool shows a
 * column beside the bar: the AEU accidentals for note input (the THM ones on a THM donanım),
 * its palette for the others (the mızrap arrows are at the top of the Artikülasyon palette);
 * Özellikler floats the Properties panel as a small window.
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
    readonly property string flat2Comma: ""     // THM ♭2 (Bravura)
    readonly property string sharp3Comma: ""    // THM ♯3 (Bravura)
    readonly property string timeSig5: ""
    readonly property string timeSig8: ""
    readonly property string eighthNote: ""   // note8thUp (Hızlı giriş)
    readonly property string finalBarline: "" // barlineFinal (Ölçü çizgisi; the user chose it)
    readonly property string repeatLeft: ""   // repeatLeft ||: (Tekrar)
    // the barline glyphs are one staff (an em) high and stand on the baseline: the same size and
    // baseline for both, so they look alike (the user asked for equal heights)
    readonly property int barlineGlyphSize: 20

    // icon: an IconCode; drawn: "makam", "thm" or "usul" (music font glyphs) or "label" (the title);
    // action: a button that does something at once instead of choosing a tool
    readonly property var toolGroups: [
        [
            { id: "selection", title: "Seçim", icon: IconCode.PAN_SCORE,
              description: "Nota girişini kapatır; tıklayarak seçersiniz" },
            { id: "noteinput", title: "Nota girişi", icon: IconCode.NOTE_PLUS,
              description: "Nota girişini açar; yanında makam arızaları" },
            { id: "speedy", title: "Hızlı giriş (S)", drawn: "speedy", action: true,
              description: "İmleç gölge notadır: ↑/↓ çizgi ve aralıklarda gezer (Ctrl ile oktav), rakam nota değerini seçer, Enter notayı yazar, * yükseltir, - alçaltır. Esc ya da S kapatır" },
            { id: "usul", title: "Usul", drawn: "usul",
              description: "Usuller yanda açılır. Ölçüyü seçip usule tıklayın; Oluştur ile yenisini yapın" }
        ],
        [
            { id: "makam", title: "Makam", drawn: "makam",
              description: "Makam donanımları yanda açılır. Ölçüyü seçip donanıma tıklayın; Oluştur ile yenisini yapın" },
            { id: "thm", title: "THM", drawn: "thm",
              description: "Halk müziği (TRT) donanımları yanda açılır: rakam koma sayısıdır (♭2 = 2 koma bemol). Ölçüyü seçip donanıma tıklayın" },
            { id: "barline", title: "Ölçü çizgisi", drawn: "barline",
              description: "Ölçü çizgileri" },
            { id: "repeat", title: "Tekrar", drawn: "repeat",
              description: "Tekrarlar ve atlamalar" }
        ],
        [
            { id: "articulation", title: "Artikülasyon", icon: IconCode.ARTICULATION,
              description: "Mızrap okları ve artikülasyonlar" },
            { id: "dynamics", title: "Nüans", icon: IconCode.DYNAMIC_FORTE,
              description: "Nüanslar, kreşendo ve dekreşendo" },
            { id: "text", title: "Metin", drawn: "label",
              description: "Metinler (Hane, Teslim, Saz…)" },
            { id: "lyrics", title: "Söz", icon: IconCode.LYRICS, action: true,
              description: "Seçili notanın altına söz yazmaya başlar" }
        ],
        [
            { id: "layout", title: "Yerleşim", icon: IconCode.LINE_BREAK,
              description: "Satır sonu, sayfa sonu, boşluklar" },
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
        } else if (toolId === "speedy") {
            toolsModel.toggleSpeedy()
        } else if (toolId === "properties") {
            toolsModel.toggleProperties()
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

                            accentButton: tool.id === "speedy" ? toolsModel.speedy
                                                               : !tool.action && toolsModel.activeTool === tool.id
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

                            // THM: Si ♭2 low on the left, Fa ♯3 high on the right (Bravura: Leland
                            // has no THM accidentals)
                            Item {
                                anchors.fill: parent
                                visible: toolButton.tool.drawn === "thm"

                                Text {
                                    x: 6
                                    y: 27 - baselineOffset
                                    text: root.flat2Comma
                                    color: ui.theme.fontPrimaryColor
                                    font.family: toolsModel.thmFontFamily
                                    font.pixelSize: 24
                                }

                                Text {
                                    x: 19
                                    y: 18 - baselineOffset
                                    text: root.sharp3Comma
                                    color: ui.theme.fontPrimaryColor
                                    font.family: toolsModel.thmFontFamily
                                    font.pixelSize: 24
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

                            // Hızlı giriş: an eighth note with speed lines on its left
                            Item {
                                anchors.fill: parent
                                visible: toolButton.tool.drawn === "speedy"

                                Repeater {
                                    model: [ { y: 12, w: 9 }, { y: 17, w: 12 }, { y: 22, w: 9 } ]

                                    delegate: Rectangle {
                                        required property var modelData
                                        x: 15 - modelData.w
                                        y: modelData.y
                                        width: modelData.w
                                        height: 1.5
                                        radius: 0.75
                                        color: ui.theme.fontPrimaryColor
                                        opacity: 0.7
                                    }
                                }

                                Text {
                                    x: 16
                                    y: 29 - baselineOffset
                                    text: root.eighthNote
                                    color: ui.theme.fontPrimaryColor
                                    font.family: ui.theme.musicalFont.family
                                    font.pixelSize: 26
                                }
                            }

                            // Ölçü çizgisi: the final barline, Tekrar: the start repeat, both of the
                            // music font, centred in the button
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                y: (root.buttonSize + root.barlineGlyphSize) / 2 - baselineOffset
                                visible: toolButton.tool.drawn === "barline" || toolButton.tool.drawn === "repeat"
                                text: toolButton.tool.drawn === "repeat" ? root.repeatLeft : root.finalBarline
                                color: ui.theme.fontPrimaryColor
                                font.family: ui.theme.musicalFont.family
                                font.pixelSize: root.barlineGlyphSize
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
                        // Makam: the THM accidentals (THM donanım) are Bravura glyphs
                        font.family: accidentalButton.modelData && accidentalButton.modelData.bravura
                                     ? toolsModel.thmFontFamily : ui.theme.musicalFont.family
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
