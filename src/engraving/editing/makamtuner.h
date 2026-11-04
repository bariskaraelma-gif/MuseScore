/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: tuning engine. Gives every note the microtonal tuning of its makam, as the Makam plugin
 * (Makam.qml v0.23.2) did, but inside the editing command: the note-entry preview already plays
 * the tuned note, and Ctrl+Z undoes the note and its tuning in one step.
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

namespace mu::engraving {
class MasterScore;
}

namespace mu::engraving::makam {
// Tunes all pitched notes of the score (Note::tuning, undoable). Call inside a command, before
// it ends (Score::endCmd). Only notes whose tuning changes are written.
void tuneScore(MasterScore* score);
}
