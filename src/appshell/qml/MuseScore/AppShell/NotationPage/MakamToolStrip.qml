/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the Finale-like tool bar on the left of the score, shown when Preferences >
 * Appearance > Arayüz is "Finale benzeri". Each tool opens its palettes in a popup; the Makam
 * and Usul tools open theirs again on every measure clicked while they are on.
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

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene
import MuseScore.Palette

Rectangle {
    id: root

    visible: toolsModel.finaleMode
    width: visible ? 44 : 0
    color: ui.theme.backgroundPrimaryColor

    readonly property var tools: [
        { id: "selection", title: "Seçim", icon: IconCode.PAN_SCORE,
          description: "Nota girişini kapatır; tıklayarak seçersiniz" },
        { id: "noteinput", title: "Nota girişi", icon: IconCode.NOTE_PLUS,
          description: "Nota girişini açar; kutuda arızalar" },
        { id: "makam", title: "Makam", icon: IconCode.KEY_SIGNATURE,
          description: "Makam donanımları. Bu araç açıkken tıkladığınız ölçüde kutu kendiliğinden açılır" },
        { id: "usul", title: "Usul", icon: IconCode.TIME_SIGNATURE,
          description: "Usuller. Bu araç açıkken tıkladığınız ölçüde kutu kendiliğinden açılır" },
        { id: "measure", title: "Ölçü", icon: IconCode.INSERT_ONE_MEASURE,
          description: "Ölçü çizgileri ve yerleşim (satır sonu, boşluk)" },
        { id: "articulation", title: "Artikülasyon", icon: IconCode.ARTICULATION,
          description: "Artikülasyonlar, süslemeler, nefes ve durak" },
        { id: "dynamics", title: "Nüans", icon: IconCode.DYNAMIC_FORTE,
          description: "Nüanslar, kreşendo ve dekreşendo" },
        { id: "lyrics", title: "Söz", icon: IconCode.LYRICS,
          description: "Seçili notanın altına söz yazmaya başlar" },
        { id: "text", title: "Metin", icon: IconCode.AUTO_TEXT,
          description: "Metinler (Hane, Teslim, Saz…) ve tempo" },
        { id: "repeat", title: "Tekrar", icon: IconCode.REPEAT_START,
          description: "Tekrarlar ve atlamalar" }
    ]

    MakamToolsModel {
        id: toolsModel
    }

    Component.onCompleted: {
        toolsModel.load()
    }

    function toolTitle(toolId) {
        for (let i = 0; i < root.tools.length; ++i) {
            if (root.tools[i].id === toolId) {
                return root.tools[i].title
            }
        }
        return ""
    }

    function toolButton(toolId) {
        for (let i = 0; i < root.tools.length; ++i) {
            if (root.tools[i].id === toolId) {
                return toolRepeater.itemAt(i)
            }
        }
        return null
    }

    Connections {
        target: toolsModel

        function onPopupRequested(tool) {
            const button = root.toolButton(tool)
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
        // the line between the tool bar and the score
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
        spacing: 4

        Repeater {
            id: toolRepeater
            model: root.tools

            delegate: FlatButton {
                required property var modelData

                width: 36
                height: 36

                enabled: toolsModel.hasNotation
                icon: modelData.icon
                toolTipTitle: modelData.title
                toolTipDescription: modelData.description

                accentButton: toolsModel.activeTool === modelData.id
                transparent: !accentButton

                onClicked: {
                    toolsModel.selectTool(modelData.id)
                }
            }
        }

        Item {
            width: 36
            height: 8
        }

        FlatButton {
            width: 36
            height: 36

            icon: IconCode.GRID
            toolTipTitle: "Bütün paletler"
            toolTipDescription: "Palet panelini açar ya da kapatır (kiriş, süsleme notası, çizgiler…)"
            transparent: true

            onClicked: {
                toolsModel.showAllPalettes()
            }
        }
    }
}
