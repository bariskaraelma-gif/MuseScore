/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * TurkScore (Makam Nota): the training tab of the Learn page. One block per section
 * (a folder in <app data>/makam-egitim), one card per lesson; a click opens the lesson score
 * in the Score tab, where it is played with the program's own player.
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

Item {
    id: root

    property var sections: []
    property color backgroundColor: ui.theme.backgroundSecondaryColor
    property real sideMargin: 46

    signal lessonClicked(string path, string title)

    StyledFlickable {
        id: flickable

        anchors.fill: parent
        contentWidth: width
        contentHeight: column.implicitHeight + root.sideMargin

        Column {
            id: column

            anchors.left: parent.left
            anchors.leftMargin: root.sideMargin
            anchors.right: parent.right
            anchors.rightMargin: root.sideMargin

            spacing: 32

            StyledTextLabel {
                width: parent.width
                visible: root.sections.length === 0
                text: "Ders bulunamadı. Dersler program klasöründeki makam-egitim klasöründe durur."
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: root.sections

                Column {
                    id: sectionColumn

                    required property var modelData

                    width: column.width
                    spacing: 14

                    StyledTextLabel {
                        text: sectionColumn.modelData.title
                        font: ui.theme.largeBodyBoldFont
                        horizontalAlignment: Text.AlignLeft
                    }

                    Flow {
                        width: parent.width
                        spacing: 16

                        Repeater {
                            model: sectionColumn.modelData.lessons

                            Rectangle {
                                id: card

                                required property var modelData

                                width: 300
                                height: 120
                                radius: 6
                                color: mouseArea.containsMouse ? ui.theme.buttonColor : ui.theme.backgroundPrimaryColor
                                border.width: 1
                                border.color: mouseArea.containsMouse ? ui.theme.accentColor : ui.theme.strokeColor

                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 6

                                    StyledTextLabel {
                                        width: parent.width
                                        text: card.modelData.title
                                        font: ui.theme.bodyBoldFont
                                        horizontalAlignment: Text.AlignLeft
                                    }

                                    StyledTextLabel {
                                        width: parent.width
                                        visible: Boolean(card.modelData.subtitle)
                                        text: card.modelData.subtitle || ""
                                        color: ui.theme.accentColor
                                        horizontalAlignment: Text.AlignLeft
                                    }

                                    StyledTextLabel {
                                        width: parent.width
                                        visible: Boolean(card.modelData.notes)
                                        text: card.modelData.notes || ""
                                        opacity: 0.75
                                        horizontalAlignment: Text.AlignLeft
                                        verticalAlignment: Text.AlignTop
                                        wrapMode: Text.WordWrap
                                        maximumLineCount: 3
                                    }
                                }

                                MouseArea {
                                    id: mouseArea

                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor

                                    onClicked: {
                                        ui.tooltip.hide(card, true)
                                        root.lessonClicked(card.modelData.path, card.modelData.title)
                                    }

                                    onContainsMouseChanged: {
                                        if (containsMouse && Boolean(card.modelData.notes)) {
                                            ui.tooltip.show(card, card.modelData.title, card.modelData.notes)
                                        } else {
                                            ui.tooltip.hide(card)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
