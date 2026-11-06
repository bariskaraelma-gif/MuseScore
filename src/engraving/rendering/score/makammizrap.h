/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the mızrap (plectrum) arrows. They are the standard guitarStrumDown/Up articulations
 * (so the file stays a normal MuseScore file), but drawn here with a long thin shaft and a small
 * open head, as in Turkish notation, instead of the short SMuFL glyph. Two arrows on one chord
 * stand side by side in the order they were put (ChordLayout::layoutArticulations2).
 *
 * Only for tlayout.cpp, tdraw.cpp and chordlayout.cpp.
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

#include "draw/types/geometry.h"
#include "types/symid.h"

namespace mu::engraving::makam {
// in spaces
constexpr double MIZRAP_HEIGHT = 3.0;        // the whole arrow
constexpr double MIZRAP_HEAD_WIDTH = 0.6;
constexpr double MIZRAP_HEAD_LENGTH = 0.55;
constexpr double MIZRAP_LINE_WIDTH = 0.12;
constexpr double MIZRAP_PAIR_GAP = 0.35;     // between two arrows on one chord

inline bool isMizrap(SymId id)
{
    return id == SymId::guitarStrumDown || id == SymId::guitarStrumUp;
}

//! as a glyph: from the baseline (y = 0) upwards
inline muse::RectF mizrapBbox(double sp)
{
    return muse::RectF(0.0, -MIZRAP_HEIGHT * sp, MIZRAP_HEAD_WIDTH * sp, MIZRAP_HEIGHT * sp);
}
}
