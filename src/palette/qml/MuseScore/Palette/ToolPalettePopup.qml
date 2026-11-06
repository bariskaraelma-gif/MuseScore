/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the popup of a tool of the Finale-like tool bar. It shows the palettes of the tool
 * (the user's own palettes, as in the Palettes panel: what was taken out of Makam Nota stays
 * out); a click on a cell applies it to the selection. "Daha fazla" shows the full palette of
 * that kind in the same popup.
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

StyledPopupView {
    id: root

    property string tool: ""
    property string title: ""

    property var paletteIndices: []

    readonly property PaletteProvider paletteProvider: rootModel.paletteProvider
    readonly property var paletteModel: paletteProvider ? paletteProvider.mainPaletteModel : null

    contentWidth: 340
    contentHeight: Math.min(column.implicitHeight, 600)

    placementPolicies: PopupView.PreferRight

    function openFor(tool, title, anchor) {
        if (root.isOpened) {
            root.close()
        }
        root.tool = tool
        root.title = title
        root.paletteIndices = rootModel.makamToolPalettes(tool)
        root.parent = anchor
        root.open()
    }

    PaletteRootModel {
        id: rootModel
    }

    ItemSelectionModel {
        id: selectionModel
        model: root.paletteModel
    }

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "ToolPalettePopup"
        section: root.navigationSection
        order: 1
        direction: NavigationPanel.Both
    }

    StyledFlickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight

        Column {
            id: column
            width: parent.width
            spacing: 8

            StyledTextLabel {
                width: parent.width
                text: root.title
                font: ui.theme.bodyBoldFont
                horizontalAlignment: Text.AlignLeft
            }

            StyledTextLabel {
                width: parent.width
                visible: root.paletteIndices.length === 0
                text: "Bu aracın paleti palet panelinde gizli ya da kaldırılmış."
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: root.paletteIndices

                delegate: Column {
                    id: section

                    required property var modelData
                    required property int index

                    readonly property size gridSize: root.paletteModel ? root.paletteModel.data(modelData, PaletteTreeModel.GridSizeRole) : Qt.size(40, 40)
                    readonly property bool drawGrid: root.paletteModel ? root.paletteModel.data(modelData, PaletteTreeModel.DrawGridRole) : false

                    property bool showAll: false

                    // the full palette of this kind, built when first shown
                    property var poolModel: null
                    property var poolRootIndex: null
                    property PaletteController poolController: null

                    width: column.width
                    spacing: 4

                    Item {
                        width: parent.width
                        height: Math.max(sectionLabel.implicitHeight, moreButton.height)

                        StyledTextLabel {
                            id: sectionLabel
                            anchors.left: parent.left
                            anchors.right: moreButton.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.paletteModel ? root.paletteModel.data(section.modelData, Qt.DisplayRole) : ""
                            horizontalAlignment: Text.AlignLeft
                            opacity: 0.8
                        }

                        FlatButton {
                            id: moreButton
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: section.showAll ? "Daha az" : "Daha fazla"
                            buttonType: FlatButton.TextOnly
                            transparent: true

                            navigation.panel: root.navigationPanel
                            navigation.row: section.index * 3
                            navigation.column: 0

                            onClicked: {
                                if (!section.poolModel) {
                                    section.poolModel = root.paletteProvider.poolPaletteModel(section.modelData)
                                    section.poolRootIndex = root.paletteProvider.poolPaletteIndex(section.modelData, section.poolModel)
                                    section.poolController = root.paletteProvider.poolPaletteController(section.poolModel, section.modelData)
                                }
                                section.showAll = !section.showAll
                            }
                        }
                    }

                    PaletteGridView {
                        width: parent.width
                        height: implicitHeight
                        visible: !section.showAll

                        cellSize: section.gridSize
                        drawGrid: section.drawGrid

                        paletteModel: root.paletteModel
                        paletteRootIndex: section.modelData
                        paletteController: root.paletteProvider ? root.paletteProvider.mainPaletteController : null
                        selectionModel: selectionModel

                        navigationPanel: root.navigationPanel
                        navigationRow: section.index * 3 + 1
                    }

                    ItemSelectionModel {
                        id: poolSelectionModel
                        model: section.poolModel
                    }

                    Loader {
                        width: parent.width
                        active: section.showAll && section.poolModel !== null
                        visible: active

                        sourceComponent: PaletteGridView {
                            width: parent ? parent.width : 0
                            height: implicitHeight

                            cellSize: section.gridSize
                            drawGrid: section.drawGrid

                            paletteModel: section.poolModel
                            paletteRootIndex: section.poolRootIndex
                            paletteController: section.poolController
                            selectionModel: poolSelectionModel

                            navigationPanel: root.navigationPanel
                            navigationRow: section.index * 3 + 2
                        }
                    }
                }
            }
        }
    }
}
