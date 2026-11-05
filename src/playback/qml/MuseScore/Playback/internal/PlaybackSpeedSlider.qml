/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

import Muse.UiComponents
import Muse.Ui

import MuseScore.Playback

RowLayout {
    id: root

    property PlaybackToolBarModel playbackModel: null

    property NavigationPanel navigationPanel: null
    property int navigationOrderStart: 0

    spacing: 12

    StyledTextLabel {
        Layout.fillWidth: true
        Layout.fillHeight: true

        text: qsTrc("playback", "Speed")
        font: ui.theme.largeBodyFont
        horizontalAlignment: Text.AlignLeft
    }

    IncrementalPropertyControl {
        id: speedControl

        Layout.preferredWidth: 76
        // Makam: the tempo itself (90 plays at 90), not a percentage
        currentValue: root.playbackModel.tempoBpm

        maxValue: 999
        minValue: 1
        step: 1
        decimals: 0

        navigation.panel: root.navigationPanel
        navigation.order: root.navigationOrderStart
        navigation.accessible.name: qsTrc("playback", "Speed")

        onValueEdited: function(newValue) {
            root.playbackModel.tempoBpm = newValue
        }
    }

    StyledSlider {
        id: slider

        Layout.preferredWidth: root.width / 2

        value: root.playbackModel.tempoMultiplier
        from: 0.1
        to: 3.0
        stepSize: 0.05

        fillBackground: false

        navigation.panel: root.navigationPanel
        navigation.order: speedControl.navigation.order + 1

        onMoved: {
            root.playbackModel.tempoMultiplier = value
        }
    }
}
