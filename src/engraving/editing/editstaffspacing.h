/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: spacing by hand, as in Finale.
 * - The gap above a staff (to the staff above it, or for the first staff of a system, to the last
 *   staff of the system above) is dragged with the mouse from the handle left of the staff, or
 *   moved with the keyboard. It changes for every system (distance above the staff, or distance
 *   between systems), or only for this system (a fixed spacer).
 * - The width of a line is dragged from the handle right of its last measure (see setLineEnd).
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

#include "../types/types.h"

#include "draw/types/geometry.h"

namespace mu::engraving {
class Measure;
class Score;
}

namespace mu::engraving::makam {
// The gap above a staff. Measures, not systems, are kept: systems may be made again by a layout.
struct StaffGap {
    Measure* measure = nullptr;          // first measure of the system of the lower staff
    staff_idx_t staffIdx = muse::nidx;   // the lower staff
    Measure* aboveMeasure = nullptr;     // first measure of the system of the upper staff
    staff_idx_t aboveStaffIdx = muse::nidx;
    double gap = 0.0;                    // spatium, from the bottom of the upper staff to the top of the lower one

    bool isValid() const { return measure && aboveMeasure; }
    bool betweenSystems() const { return measure != aboveMeasure; }
};

// The gap whose handle is at pos (canvas coordinates): left of the staff, in the page margin.
// Not valid for the first staff of a page.
StaffGap staffGapAt(Score* score, const muse::PointF& pos);

// The gap above the staff of the selection, in the system of its first measure
StaffGap staffGapOfSelection(Score* score);

// Sets the gap to gap.gap + delta spatium, inside a command. For every system, the distance above
// the staff (or between systems) is changed; for this system only, a fixed spacer is set on the
// upper staff. Vertical justification is turned off, so that the gap stays as it is set.
void setStaffGap(Score* score, const StaffGap& gap, double delta, bool thisSystemOnly);

//---------------------------------------------------------
//   Line end: a line (system) that is not stretched to the page width. The line ends with a
//   horizontal frame and a line break after its last measure; the frame takes the rest of the
//   width, so the measures keep their natural width. Both are plain MuseScore elements, so the
//   line looks the same in MuseScore. The handle right of the last measure drags the frame width.
//---------------------------------------------------------

// The line end whose handle is at pos (canvas coordinates): right of the last measure of a system.
// Returns its last measure, nullptr if none.
Measure* lineEndAt(Score* score, const muse::PointF& pos);

// Whether a frame follows the measure, and its width in spatium (0 if there is none)
bool hasLineEndFrame(const Measure* measure);
double lineEndFrameWidth(const Measure* measure);

// Width in spatium of the frame that leaves the line ending with the measure at its natural
// width. The spacing of the measure's system is computed again for it, then put back as it was.
double naturalLineEndFrameWidth(Measure* measure);

// The right end of the measure's line (page width), in canvas coordinates
double lineEndRight(const Measure* measure);

// Ends the line after the measure with a frame of the given width (spatium) and a line break.
// Inside a command.
void setLineEnd(Score* score, Measure* measure, double frameWidth);

// Removes the frame after the measure; the line break goes back to the measure. Inside a command.
void removeLineEnd(Score* score, Measure* measure);

// The line end palette cell dropped at pos (canvas coordinates): the line ends after the measure
// there, at its natural width. Inside a command. False if there is no measure at pos.
bool dropLineEnd(Score* score, const muse::PointF& pos);
}
