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

#include <map>
#include <string>
#include <vector>

#include <QString>

namespace mu::engraving {
enum class AccidentalType : unsigned char;
class KeySigEvent;
class MasterScore;
class Note;
class Score;
}

namespace mu::engraving::makam {
// Tunes all pitched notes of the score (Note::tuning, undoable). Call inside a command, before
// it ends (Score::endCmd). Only notes whose tuning changes are written.
void tuneScore(MasterScore* score);

//---------------------------------------------------------
//   calibration (Makam window). Values are cents from the written equal-tempered note; the
//   user's values are kept in the app settings and override the defaults.
//---------------------------------------------------------

struct MakamName {
    std::string id;
    QString name;
};

struct PerdeValues {
    std::string perde;   // letter + koma shift, e.g. "B-1" = Si koma bemol (Segah)
    double up = 0.0;     // çıkış
    double down = 0.0;   // iniş
    double stop = 0.0;   // durak
};

struct ScoreMakam {
    std::string id;
    QString name;
    int measureNo = 1;
    QString source;      // başlık, dizek metni, ezgiden tahmin…
};

std::vector<MakamName> allMakams();

// Name, karar and donanım of a makam (for the dizi score, makamdizi.h)
struct MakamBasics {
    std::string id;
    QString name;
    std::string karar;                  // final perde, e.g. "A0", "B-1" (Segah), "F4" (Irak)
    std::vector<std::string> donanim;   // perdes of the donanım it is written with, e.g. { "B-1", "F4" }
};

// false for an unknown makam id
bool makamBasics(const std::string& makamId, MakamBasics& result);
std::vector<PerdeValues> calibration(const std::string& makamId, bool defaults = false);
void setCalibration(const std::string& makamId, const PerdeValues& values);
void resetCalibration(const std::string& makamId);   // empty id: all makams

// Segah while La♯ (yeden) is active, cents from Si
double yedenSegah();
void setYedenSegah(double cents);
double defaultYedenSegah();

QString perdeLabel(const std::string& perde);   // "Si koma bemol"
int perdeAlter(const std::string& perde);       // the ♯ (1) / ♭ (-1) contained in the written note

// The makams of a score, in order, with where each was found
std::vector<ScoreMakam> scoreMakams(MasterScore* score);

// Türk arıza ↑/↓: moves the selected notes one step in the AEU accidental ladder (inside a
// command). Returns false when no pitched note is selected (MuseScore's own pitch-up/down then).
bool turkishStep(Score* score, int direction);
//---------------------------------------------------------
//   Turkish reading of the written notes (used by the SymbTr import, makamsymbtr.h)
//---------------------------------------------------------

// Koma shift of an accidental in AEU (or THM) terms; false for an accidental with no Turkish reading
bool accidentalKomaShift(AccidentalType type, int& koma);
// The AEU accidental of a koma shift (0 natural, ±1 koma, 4 bakiye ♯, -4 bakiye ♭, ±5, ±8); NONE for others
AccidentalType accidentalOfKoma(int koma);
// Koma shift per letter (index "CDEFGAB") of a key signature
std::vector<int> keySigKomas(const KeySigEvent& ke);
// The koma shift the tuner reads for every pitched note: its accidental, a tie, an accidental
// earlier in the measure and voice, the ♯/♭ of its pitch, the donanım
std::map<const Note*, int> readKomas(MasterScore* score);
}
