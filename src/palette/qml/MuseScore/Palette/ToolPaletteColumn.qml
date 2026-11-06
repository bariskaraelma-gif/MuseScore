/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: a palette of a tool of the Finale-like tool bar as a column beside the bar, two cells
 * wide (the Makam and Usul tools). The cells are the user's own palette, as in the Palettes
 * panel; a click on a cell applies it to the selection, as there.
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
    property int columns: 2

    readonly property PaletteProvider paletteProvider: rootModel.paletteProvider
    readonly property var paletteModel: paletteProvider ? paletteProvider.mainPaletteModel : null

    property var paletteIndex: null
    readonly property size cellSize: (paletteModel && paletteIndex) ? paletteModel.data(paletteIndex, PaletteTreeModel.GridSizeRole) : Qt.size(56, 47)

    implicitWidth: columns * cellSize.width

    function updatePalette() {
        const indices = root.tool !== "" ? rootModel.makamToolPalettes(root.tool) : []
        root.paletteIndex = indices.length > 0 ? indices[0] : null
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

    StyledTextLabel {
        anchors.fill: parent
        anchors.margins: 4
        visible: !root.paletteIndex
        text: "Bu aracın paleti palet panelinde gizli ya da kaldırılmış."
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignTop
    }

    PaletteGridView {
        anchors.fill: parent
        visible: Boolean(root.paletteIndex)

        cellSize: root.cellSize
        drawGrid: (root.paletteModel && root.paletteIndex) ? root.paletteModel.data(root.paletteIndex, PaletteTreeModel.DrawGridRole) : false

        paletteModel: root.paletteModel
        paletteRootIndex: root.paletteIndex
        paletteController: root.paletteProvider ? root.paletteProvider.mainPaletteController : null
        selectionModel: selectionModel
    }
}
