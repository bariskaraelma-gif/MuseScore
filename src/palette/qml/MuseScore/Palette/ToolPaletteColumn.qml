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

    // Makam: "Oluştur" (a new donanım or usul) for the Makam and Usul tools
    readonly property bool canCreate: tool === "makam" || tool === "usul"
    property string newName: ""
    property var newAccs: []      // [{ letter, koma }] (makamdata.h)
    property var newDarbs: []     // [{ dum, len }]
    property int newUnit: 8
    property string createError: ""

    readonly property var letterOptions: [
        { text: "Do", value: "C" }, { text: "Re", value: "D" }, { text: "Mi", value: "E" },
        { text: "Fa", value: "F" }, { text: "Sol", value: "G" }, { text: "La", value: "A" },
        { text: "Si", value: "B" }
    ]
    readonly property var komaOptions: [
        { text: "Koma bemol", value: -1 },
        { text: "Bakiye bemol", value: -4 },
        { text: "Küçük mücennep bemol (♭)", value: -5 },
        { text: "Büyük mücennep bemol", value: -8 },
        { text: "Koma diyez", value: 1 },
        { text: "Bakiye diyez (♯)", value: 4 },
        { text: "Küçük mücennep diyez", value: 5 },
        { text: "Büyük mücennep diyez", value: 8 }
    ]

    function resetCreateForm() {
        newName = ""
        createError = ""
        newAccs = [{ letter: "B", koma: -1 }]
        newDarbs = [{ dum: true, len: 2 }, { dum: false, len: 1 }]
        newUnit = 8
    }

    // the next accidental: the first note not used yet, in the order flats are written
    function addAcc() {
        const order = ["B", "E", "A", "D", "G", "C", "F"]
        const used = newAccs.map(a => a.letter)
        const letter = order.find(l => !used.includes(l))
        if (letter === undefined) {
            return
        }
        let list = newAccs.slice()
        list.push({ letter: letter, koma: -1 })
        newAccs = list
    }

    function setAcc(i, key, value) {
        let list = newAccs.slice()
        let acc = Object.assign({}, list[i])
        acc[key] = value
        list[i] = acc
        newAccs = list
    }

    function setDarb(i, key, value) {
        let list = newDarbs.slice()
        let darb = Object.assign({}, list[i])
        darb[key] = value
        list[i] = darb
        newDarbs = list
    }

    function darbTotal() {
        let total = 0
        for (const d of newDarbs) {
            total += d.len
        }
        return total
    }

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

        // Makam: a new donanım or usul of the user's own, added to the end of this palette
        FlatButton {
            id: createButton

            width: parent.width
            height: 24
            visible: Boolean(root.paletteIndex) && root.canCreate
            text: "Oluştur…"
            buttonType: FlatButton.TextOnly
            transparent: true

            onClicked: {
                if (!createPopup.isOpened) {
                    root.resetCreateForm()
                }
                createPopup.toggleOpened()
            }

            StyledPopupView {
                id: createPopup

                contentWidth: 320
                contentHeight: createForm.implicitHeight
                placementPolicies: PopupView.PreferRight

                Column {
                    id: createForm

                    width: 320
                    spacing: 8

                    StyledTextLabel {
                        width: parent.width
                        text: root.tool === "makam" ? "Yeni donanım" : "Yeni usul"
                        font: ui.theme.bodyBoldFont
                        horizontalAlignment: Text.AlignLeft
                    }

                    StyledTextLabel {
                        width: parent.width
                        text: root.tool === "makam" ? "Ad (hücrede görünür, ör. makamın adı)" : "Usulün adı (ölçünün üstüne yazılır)"
                        horizontalAlignment: Text.AlignLeft
                    }

                    TextInputField {
                        id: nameField

                        width: parent.width
                        currentText: root.newName
                        onTextChanged: function(newTextValue) {
                            root.newName = newTextValue
                        }
                    }

                    // the donanım: one row per accidental, in the order they are written
                    Column {
                        width: parent.width
                        spacing: 6
                        visible: root.tool === "makam"

                        StyledTextLabel {
                            width: parent.width
                            text: "Arızalar (yazılış sırasıyla)"
                            horizontalAlignment: Text.AlignLeft
                        }

                        Repeater {
                            model: root.newAccs

                            delegate: Row {
                                id: accRow

                                required property var modelData
                                required property int index

                                spacing: 6

                                StyledDropdown {
                                    width: 80
                                    model: root.letterOptions
                                    currentIndex: indexOfValue(accRow.modelData.letter)
                                    onActivated: function(index, value) {
                                        root.setAcc(accRow.index, "letter", value)
                                    }
                                }

                                StyledDropdown {
                                    width: 190
                                    model: root.komaOptions
                                    currentIndex: indexOfValue(accRow.modelData.koma)
                                    onActivated: function(index, value) {
                                        root.setAcc(accRow.index, "koma", value)
                                    }
                                }

                                FlatButton {
                                    width: 30
                                    height: 30
                                    icon: IconCode.CLOSE_X_ROUNDED
                                    transparent: true
                                    toolTipTitle: "Bu arızayı sil"
                                    onClicked: {
                                        let list = root.newAccs.slice()
                                        list.splice(accRow.index, 1)
                                        root.newAccs = list
                                    }
                                }
                            }
                        }

                        FlatButton {
                            icon: IconCode.PLUS
                            text: "Arıza ekle"
                            orientation: Qt.Horizontal
                            enabled: root.newAccs.length < 7
                            onClicked: {
                                root.addAcc()
                            }
                        }
                    }

                    // the usul: the unit, then one row per stroke (düm / tek and its length)
                    Column {
                        width: parent.width
                        spacing: 6
                        visible: root.tool === "usul"

                        Row {
                            spacing: 8

                            StyledTextLabel {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Birim:"
                            }

                            StyledDropdown {
                                width: 120
                                model: [
                                    { text: "2'lik", value: 2 },
                                    { text: "4'lük", value: 4 },
                                    { text: "8'lik", value: 8 },
                                    { text: "16'lık", value: 16 }
                                ]
                                currentIndex: indexOfValue(root.newUnit)
                                onActivated: function(index, value) {
                                    root.newUnit = value
                                }
                            }

                            StyledTextLabel {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Ölçü: " + root.darbTotal() + "/" + root.newUnit
                                font: ui.theme.bodyBoldFont
                            }
                        }

                        StyledTextLabel {
                            width: parent.width
                            text: "Darblar (süre, birim cinsinden)"
                            horizontalAlignment: Text.AlignLeft
                        }

                        Repeater {
                            model: root.newDarbs

                            delegate: Row {
                                id: darbRow

                                required property var modelData
                                required property int index

                                spacing: 6

                                StyledDropdown {
                                    width: 100
                                    model: [
                                        { text: "Düm", value: true },
                                        { text: "Tek", value: false }
                                    ]
                                    currentIndex: darbRow.modelData.dum ? 0 : 1
                                    onActivated: function(index, value) {
                                        root.setDarb(darbRow.index, "dum", value)
                                    }
                                }

                                IncrementalPropertyControl {
                                    width: 100
                                    currentValue: darbRow.modelData.len
                                    step: 1
                                    decimals: 0
                                    minValue: 1
                                    maxValue: 16
                                    onValueEdited: function(newValue) {
                                        root.setDarb(darbRow.index, "len", newValue)
                                    }
                                }

                                FlatButton {
                                    width: 30
                                    height: 30
                                    icon: IconCode.CLOSE_X_ROUNDED
                                    transparent: true
                                    toolTipTitle: "Bu darbı sil"
                                    onClicked: {
                                        let list = root.newDarbs.slice()
                                        list.splice(darbRow.index, 1)
                                        root.newDarbs = list
                                    }
                                }
                            }
                        }

                        FlatButton {
                            icon: IconCode.PLUS
                            text: "Darb ekle"
                            orientation: Qt.Horizontal
                            enabled: root.newDarbs.length < 32
                            onClicked: {
                                let list = root.newDarbs.slice()
                                list.push({ dum: false, len: 1 })
                                root.newDarbs = list
                            }
                        }
                    }

                    StyledTextLabel {
                        width: parent.width
                        visible: root.createError !== ""
                        text: root.createError
                        color: "#d9534f"
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignLeft
                    }

                    StyledTextLabel {
                        width: parent.width
                        text: "Yeni hücre paletin sonuna eklenir; ölçü seçiliyse hemen uygulanır. Silmek için hücreye sağ tıklayın (palet panelinde)."
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignLeft
                        opacity: 0.7
                    }

                    Row {
                        anchors.right: parent.right
                        spacing: 8

                        FlatButton {
                            text: "Vazgeç"
                            onClicked: {
                                createPopup.close()
                            }
                        }

                        FlatButton {
                            text: "Oluştur"
                            accentButton: true
                            onClicked: {
                                root.createError = root.tool === "makam"
                                        ? rootModel.makamCreateDonanim(root.newName, root.newAccs)
                                        : rootModel.makamCreateUsul(root.newName, root.darbTotal(), root.newUnit, root.newDarbs)
                                if (root.createError === "") {
                                    root.showAll = false
                                    createPopup.close()
                                }
                            }
                        }
                    }
                }
            }
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
