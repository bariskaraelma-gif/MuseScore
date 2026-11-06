/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the palette of a tool of the Finale-like tool bar as a column beside the bar, two
 * cells wide. The cells are the user's own palette, as in the Palettes panel (what was taken
 * out of Makam Nota stays out); a click on a cell applies it to the selection, as there.
 * "Daha fazla" shows the full palette of that kind in the same column.
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
import QtQml.Models

import Muse.Ui
import Muse.UiComponents
import MuseScore.Palette

Item {
    id: root

    property string tool: ""
    property string title: ""
    property int columns: 2

    readonly property PaletteProvider paletteProvider: rootModel.paletteProvider
    readonly property var paletteModel: paletteProvider ? paletteProvider.mainPaletteModel : null

    property var paletteIndex: null
    readonly property size cellSize: (paletteModel && paletteIndex) ? paletteModel.data(paletteIndex, PaletteTreeModel.GridSizeRole) : Qt.size(56, 47)
    readonly property bool drawGrid: (paletteModel && paletteIndex) ? paletteModel.data(paletteIndex, PaletteTreeModel.DrawGridRole) : false

    // the full palette of this kind ("Daha fazla"), built when first shown
    property bool showAll: false
    property var poolModel: null
    property var poolRootIndex: null
    property PaletteController poolController: null

    implicitWidth: Math.max(columns * cellSize.width, 2 * 36 + 4)

    function updatePalette() {
        const indices = root.tool !== "" ? rootModel.makamToolPalettes(root.tool) : []
        root.paletteIndex = indices.length > 0 ? indices[0] : null
        root.showAll = false
        root.poolModel = null
        root.poolRootIndex = null
        root.poolController = null
    }

    onToolChanged: updatePalette()
    Component.onCompleted: updatePalette()

    PaletteRootModel {
        id: rootModel
    }

    ItemSelectionModel {
        id: selectionModel
        model: root.paletteModel
    }

    ItemSelectionModel {
        id: poolSelectionModel
        model: root.poolModel
    }

    Column {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 2

        StyledTextLabel {
            width: parent.width
            text: root.title
            font: ui.theme.bodyBoldFont
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }

        FlatButton {
            width: parent.width
            height: 24
            visible: Boolean(root.paletteIndex)
            text: root.showAll ? "Daha az" : "Daha fazla"
            buttonType: FlatButton.TextOnly
            transparent: true

            onClicked: {
                if (!root.poolModel) {
                    root.poolModel = root.paletteProvider.poolPaletteModel(root.paletteIndex)
                    root.poolRootIndex = root.paletteProvider.poolPaletteIndex(root.paletteIndex, root.poolModel)
                    root.poolController = root.paletteProvider.poolPaletteController(root.poolModel, root.paletteIndex)
                }
                root.showAll = !root.showAll
            }
        }
    }

    StyledTextLabel {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.topMargin: 4
        visible: !root.paletteIndex
        text: "Bu aracın paleti palet panelinde gizli ya da kaldırılmış."
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignLeft
    }

    PaletteGridView {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.topMargin: 4
        anchors.bottom: parent.bottom
        visible: Boolean(root.paletteIndex) && !root.showAll

        cellSize: root.cellSize
        drawGrid: root.drawGrid

        paletteModel: root.paletteModel
        paletteRootIndex: root.paletteIndex
        paletteController: root.paletteProvider ? root.paletteProvider.mainPaletteController : null
        selectionModel: selectionModel
    }

    Loader {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.topMargin: 4
        anchors.bottom: parent.bottom
        active: root.showAll && root.poolModel !== null

        sourceComponent: PaletteGridView {
            cellSize: root.cellSize
            drawGrid: root.drawGrid

            paletteModel: root.poolModel
            paletteRootIndex: root.poolRootIndex
            paletteController: root.poolController
            selectionModel: poolSelectionModel
        }
    }
}
