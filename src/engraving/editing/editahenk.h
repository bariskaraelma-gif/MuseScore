/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: ahenk (pitch level of Turkish music). Notes are written in Bolahenk and sound lower;
 * the ahenk is stored as the transposition of the pitched parts, so the written notes and
 * donanıms stay as they are and only the sounding pitch changes.
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

#include <vector>

#include "../dom/key.h"
#include "../types/fraction.h"

namespace mu::engraving {
class Part;
class Score;
class Staff;
}

namespace mu::engraving::makam {
struct Ahenk {
    const char* name = "";   // shown in the status bar
    int diatonic = 0;        // transposition of the written notes (negative: sounds lower)
    int chromatic = 0;
};

// Yerinden, 1 ses, 4 ses, 5 ses (as in the Makam plugin), and written pitch (Western)
const std::vector<Ahenk>& ahenkler();

// Index of the ahenk of new scores: 4 ses
static constexpr int DEFAULT_AHENK = 2;

// Parts that follow the ahenk: pitched parts (not drum sets, not tablature)
bool isAhenkPart(const Part* part);

// Index in ahenkler() of the score's ahenk (from its first pitched part), -1 if it has
// no pitched part or another transposition
int ahenkIndex(const Score* score);

// Sets the ahenk of every pitched part of the score and its excerpts. The written notes and
// key signatures stay; the sounding pitch moves. Undoable inside a command; outside a command
// (new score) the changes are applied directly.
void setAhenk(Score* score, const Ahenk& ahenk);

// A key signature from the palette is a written key (donanım in Bolahenk). On a transposed
// staff, its concert key is moved by the transposition, so that Score::undoChangeKeySig
// (which takes the concert key) gives back the same written key.
KeySigEvent writtenKeySigEvent(KeySigEvent ke, const Staff* staff, const Fraction& tick);
}
