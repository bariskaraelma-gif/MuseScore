/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: "Hızlı giriş" (speedy entry, as in Finale). In note input the cursor is the shadow note
 * at the input position, on a line or space of the staff: ↑/↓ move it a step (Ctrl: an octave),
 * the number keys choose the note value (the shadow note shows it), Enter writes the note there
 * with the accidentals of the donanım, * or + raise and - lower the last note on the Turkish
 * accidental ladder. S (command makam-speedy) or the tool bar button turns it on and off.
 *
 * Header only (no new .cpp: unity build, see CLAUDE.md). The state is one for the program.
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
#pragma once

#include <algorithm>

#include "async/notification.h"

#include "engraving/dom/segment.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/utils.h"

#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"

namespace mu::notation::makamspeedy {
static constexpr const char* ACTION_CODE = "makam-speedy";

// line of the staff, as Position::line: 0 the top line, 8 the bottom line of five, a step each
static constexpr int MIDDLE_LINE = 4;
static constexpr int MIN_LINE = -14;   // about four ledger lines above, more than enough
static constexpr int MAX_LINE = 22;

struct State {
    bool on = false;
    int line = MIDDLE_LINE;
    muse::async::Notification changed;   // on/off
};

inline State& state()
{
    static State s;
    return s;
}

// the staff and position where the next note goes, if speedy entry can work there (a pitched
// staff: the usul staff and tablature keep MuseScore's own input)
inline const mu::engraving::Staff* inputStaff(const INotationInteractionPtr& interaction)
{
    if (!interaction || !interaction->noteInput() || !interaction->noteInput()->isNoteInputMode()) {
        return nullptr;
    }
    const NoteInputState& is = interaction->noteInput()->state();
    const mu::engraving::Segment* segment = is.segment();
    const mu::engraving::Staff* staff = is.staff();
    if (!segment || !staff) {
        return nullptr;
    }
    const mu::engraving::Fraction tick = segment->tick();
    if (staff->isDrumStaff(tick) || staff->isTabStaff(tick)) {
        return nullptr;
    }
    return staff;
}

inline bool active(const INotationInteractionPtr& interaction)
{
    return state().on && inputStaff(interaction) != nullptr;
}

// the absolute step (as NoteInputParams::step) of the cursor line, under the clef in effect
inline int cursorStep(const INotationInteractionPtr& interaction)
{
    const mu::engraving::Staff* staff = inputStaff(interaction);
    const mu::engraving::Fraction tick = interaction->noteInput()->state().segment()->tick();
    return mu::engraving::absStep(state().line, staff->clef(tick));
}

// the cursor: the shadow note on the cursor line at the input position, in the note value chosen
inline void showCursor(const INotationInteractionPtr& interaction)
{
    if (!active(interaction)) {
        return;
    }
    // a white-key pitch whose note sits on that line (showShadowNoteForMidiPitch takes the line
    // back from it: octave * 7 + pitch2step)
    const int step = std::max(0, cursorStep(interaction));
    const int pitch = (step / 7) * 12 + mu::engraving::step2pitch(step % 7);
    if (pitch < 0 || pitch > 127) {
        return;
    }
    interaction->showShadowNoteForMidiPitch(static_cast<uint8_t>(pitch));
}

inline void moveCursor(const INotationInteractionPtr& interaction, int steps)
{
    state().line = std::clamp(state().line + steps, MIN_LINE, MAX_LINE);
    showCursor(interaction);
}

// Enter: the note at the cursor line, in the input note value; the input moves on
inline void writeNote(const INotationInteractionPtr& interaction)
{
    if (!active(interaction)) {
        return;
    }
    NoteInputParams params;
    params.step = cursorStep(interaction);
    interaction->noteInput()->addNote(params, NoteAddingMode::NextChord);
    showCursor(interaction);
}

inline void setOn(const INotationInteractionPtr& interaction, bool on)
{
    if (state().on == on) {
        if (on) {
            showCursor(interaction);
        }
        return;
    }
    state().on = on;
    if (on) {
        showCursor(interaction);
    } else if (interaction) {
        interaction->hideShadowNote();
    }
    state().changed.notify();
}

// note input started, ended or moved: the cursor follows; when note input ends speedy entry ends
inline void onNoteInputChanged(const INotationInteractionPtr& interaction)
{
    if (!state().on) {
        return;
    }
    if (!interaction || !interaction->noteInput() || !interaction->noteInput()->isNoteInputMode()) {
        state().on = false;
        state().changed.notify();
        return;
    }
    showCursor(interaction);
}
}
