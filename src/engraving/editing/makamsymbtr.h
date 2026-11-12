/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the MusicXML files of the SymbTr corpus (MTG, github.com/MTG/SymbTr) opened as Turkish
 * makam scores.
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
// A score imported from a SymbTr MusicXML file: its first measure has the text
// "Makam: Hicaz, Form: Şarkı, Usul: Ağır Aksaksemâî" (before the metronome mark).
bool isSymbTrScore(MasterScore* score);

// After the MusicXML import of a SymbTr file, outside a command (not in the undo history):
// - the makam, form and usul go to the subtitle (the tuner finds the makam there);
// - the time signatures of a known usul get its usul marker (usul staff, usullü metronom);
// - the accidentals are rewritten for the Turkish reading. SymbTr gives every note its own
//   pitch (a note with no accidental and no <alter> is natural, even where the donanım or an
//   earlier note of the measure has a koma accidental); TurkScore reads the donanım and the
//   accidentals of the measure as in written music. So redundant accidentals are removed and
//   naturals are added where the Turkish reading needs them;
// - the notes are tuned.
void fixSymbTrScore(MasterScore* score);
}
