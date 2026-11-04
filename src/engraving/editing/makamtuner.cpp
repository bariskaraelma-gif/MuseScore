/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: tuning engine, see makamtuner.h. A port of the tuning of the Makam plugin
 * (Makam.qml v0.23.2, Copyright (C) 2026 Barış Karaelma, GPL-3.0):
 *   - Plain notes, ♯ and ♭ are equal-tempered.
 *   - The calibrated microtonal perdes of a makam take their value in cents from the written
 *     equal-tempered note: ascending (çıkış), descending (iniş) or durak (half note or longer).
 *     The direction is where the melody goes next in the same voice; at the end of a voice, the
 *     direction from the previous different pitch.
 *   - Microtonal perdes without a calibration value play at their AEU position (1 koma = 1200/53).
 *   - Yeden: La♯ and the Segah notes in its region play as "yedenli Segah".
 *   - The makam comes from the title and from staff / system texts that start with a makam name
 *     (geçki from that point), else it is guessed from the melody (key signature, start, karar).
 *   - The built-in cent offset of microtonal accidentals (Note::playingTuning) is subtracted.
 * The ahenk is not part of the tuning: it is the transposition of the part (editahenk.h).
 * Calibration values were measured on recordings (OTMM makam recognition + tonic datasets, MTG,
 * CC BY-NC-SA 4.0); seyir centres come from the SymbTr collection (MTG, CC BY-NC-SA 4.0).
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
#include "makamtuner.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <QRegularExpression>
#include <QString>

#include "../dom/accidental.h"
#include "../dom/chord.h"
#include "../dom/key.h"
#include "../dom/masterscore.h"
#include "../dom/measure.h"
#include "../dom/measurebase.h"
#include "../dom/note.h"
#include "../dom/part.h"
#include "../dom/instrument.h"
#include "../dom/segment.h"
#include "../dom/staff.h"
#include "../dom/textbase.h"

namespace mu::engraving::makam {
namespace {
struct Calib {
    double up = 0.0;
    double down = 0.0;
    double stop = 0.0;
};

struct MakamDef {
    const char* id = "";
    std::vector<const char*> keys;                          // names in the score text (normalized)
    std::vector<std::pair<const char*, Calib> > calib;      // perde -> cents from the written ET note
    std::vector<std::vector<const char*> > ksLists;         // donanıms the makam is written with
    const char* karar = "";                                 // final perde
    double seyir = 0.0;                                     // written MIDI pitch where the melody starts
};

const std::vector<MakamDef>& makamlar()
{
    static const std::vector<MakamDef> list {
        { "ussak",
          { "ussak" },
          { { "B-1", { -50.0, -66.67, -50.0 } } },
          { { "B-1" } },
          "A0", 71.9 },
        { "rast",
          { "rast" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "F4", { -16.67, -33.33, -16.67 } } },
          { { "B-1", "F4" } },
          "G0", 70.6 },
        { "huzzam",
          { "huzzam" },
          { { "B-1", { -16.67, -33.33, -16.67 } }, { "E-4", { -50.0, -66.67, -50.0 } }, { "F4", { -33.33, -33.33, -33.33 } } },
          { { "B-1", "E-4", "F4" } },
          "B-1", 74.9 },
        { "hicaz",
          { "hicaz" },
          { { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-4", "C4" }, { "B-4", "C4", "F4" } },
          "A0", 74.1 },
        { "huseyni",
          { "huseyni" },
          { { "B-1", { -33.33, -50.0, -33.33 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-1", "F4" } },
          "A0", 75.5 },
        { "segah",
          { "segah" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "E-1", { 0.0, -16.67, 0.0 } }, { "F4", { -16.67, -50.0, -16.67 } } },
          { { "B-1", "E-1", "F4" } },
          "B-1", 73.3 },
        { "saba",
          { "saba" },
          { { "B-1", { -33.33, -50.0, -33.33 } }, { "D-4", { -66.67, -66.67, -66.67 } } },
          { { "B-1", "D-4" } },
          "A0", 71.9 },
        { "nihavend",
          { "nihavend", "nihavent" },
          {  },
          { { "B-5", "E-5" } },
          "G0", 72.5 },
        { "kurdi",
          { "kurdi" },
          {  },
          { { "B-5" } },
          "A0", 74.0 },
        { "muhayyerkurdi",
          { "muhayyerkurdi", "muhayyer kurdi" },
          {  },
          { { "B-5" } },
          "A0", 78.9 },
        { "muhayyer",
          { "muhayyer" },
          { { "B-1", { -33.33, -50.0, -33.33 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-1", "F4" } },
          "A0", 78.5 },
        { "acemkurdi",
          { "acemkurdi", "acem kurdi" },
          {  },
          { { "B-5" } },
          "A0", 76.5 },
        { "bayati",
          { "bayati", "beyati" },
          { { "B-1", { -50.0, -66.67, -50.0 } } },
          { { "B-1" } },
          "A0", 73.8 },
        { "evic",
          { "evic" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "F4", { -16.67, -33.33, -16.67 } } },
          { { "B-1", "F4" } },
          "F4", 77.2 },
        { "kurdilihicazkar",
          { "kurdilihicazkar", "kurdili hicazkar" },
          {  },
          { { "A-5", "B-5", "E-5" } },
          "G0", 77.0 },
        { "hicazuzzal",
          { "hicaz uzzal", "hicazuzzal", "uzzal hicaz", "uzzal" },
          { { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-4", "C4" } },
          "A0", 75.3 },
        { "zirgulelihicaz",
          { "zirguleli hicaz", "zirgulelihicaz", "hicaz zirgule" },
          { { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "G4", { -16.67, -16.67, -16.67 } } },
          { { "B-4", "C4", "G4" } },
          "A0", 72.6 },
        { "sedaraban",
          { "sedaraban", "sedd-i araban", "seddi araban", "sed araban", "sedd araban" },
          { { "E-4", { -83.33, -83.33, -83.33 } }, { "F4", { -16.67, -16.67, -16.67 } }, { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } } },
          { { "B-4", "E-4", "F4", "C4" } },
          "D0", 74.5 },
        { "buselik",
          { "buselik" },
          {  },
          { {  } },
          "A0", 72.7 },
        { "cargah",
          { "cargah", "yeni cargah" },
          {  },
          { {  } },
          "C0", 73.1 },
        { "mahur",
          { "mahur" },
          { { "F5", { 116.67, 116.67, 116.67 } } },
          { { "F5" } },
          "G0", 78.2 },
        { "acemasiran",
          { "acemasiran", "acem asiran" },
          {  },
          { { "B-5" } },
          "F0", 76.4 },
        { "evcara",
          { "evcara", "evc ara", "evc-ara", "evara" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "F4", { -16.67, -33.33, -16.67 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "E4", { -16.67, -16.67, -16.67 } } },
          { { "B-1", "F4", "C4", "E4", "A4" } },
          "F4", 78.0 },
        { "suzidil",
          { "suzidil", "suzi dil" },
          { { "F1", { 16.67, 16.67, 16.67 } }, { "G4", { -16.67, -16.67, -16.67 } }, { "D4", { -16.67, -16.67, -16.67 } } },
          { { "F1", "G4", "D4" } },
          "E0", 76.0 },
        { "karcigar",
          { "karcigar" },
          { { "B-1", { -33.33, -66.67, -33.33 } }, { "E-4", { -50.0, -66.67, -50.0 } }, { "F4", { -33.33, -33.33, -33.33 } } },
          { { "B-1", "E-4", "F4" } },
          "A0", 74.5 },
        { "suzinak",
          { "suzinak" },
          { { "A-4", { -83.33, -83.33, -83.33 } }, { "B-1", { -16.67, -16.67, -16.67 } }, { "E-4", { -83.33, -83.33, -83.33 } }, { "F4", { -16.67, -16.67, -16.67 } } },
          { { "B-1", "E-4", "F4" } },
          "G0", 73.3 },
        { "hicazkar",
          { "hicazkar" },
          { { "A-4", { -83.33, -83.33, -83.33 } }, { "B-1", { 0.0, -16.67, 0.0 } }, { "E-4", { -83.33, -66.67, -83.33 } }, { "F4", { 0.0, -16.67, 0.0 } } },
          { { "A-4", "B-1", "E-4", "F4" } },
          "G0", 77.5 },
    };
    return list;
}

constexpr double KOMA = 1200.0 / 53.0;
constexpr double YEDEN_SEGAH = -16.67;    // Segah while La♯ (yeden) is active, cents from Si
const char* const FALLBACK_MAKAM = "ussak";

const MakamDef* makamById(const std::string& id)
{
    for (const MakamDef& m : makamlar()) {
        if (id == m.id) {
            return &m;
        }
    }
    return nullptr;
}

const Calib* calibOf(const MakamDef* makam, const std::string& perde)
{
    if (!makam) {
        return nullptr;
    }
    for (const auto& [p, c] : makam->calib) {
        if (perde == p) {
            return &c;
        }
    }
    return nullptr;
}

//---------------------------------------------------------
//   makam names in texts
//---------------------------------------------------------

QString norm(const QString& text)
{
    static const QRegularExpression tags("<[^>]*>");
    QString s = text;
    s.replace(tags, " ");
    s.replace(QChar(u'I'), QChar(u'ı'));
    s.replace(QChar(u'İ'), QChar(u'i'));
    s = s.toLower();
    static const std::vector<std::pair<char16_t, char16_t> > map {
        { u'ı', u'i' }, { u'ş', u's' }, { u'ğ', u'g' }, { u'ü', u'u' }, { u'ö', u'o' },
        { u'ç', u'c' }, { u'â', u'a' }, { u'î', u'i' }, { u'û', u'u' },
    };
    for (const auto& [from, to] : map) {
        s.replace(QChar(from), QChar(to));
    }
    return s.simplified();
}

struct KeyPattern {
    std::string id;
    QRegularExpression anywhere;
    QRegularExpression atStart;
};

// Longest names first, so "hicazkar" is not read as "hicaz"
const std::vector<KeyPattern>& keyPatterns()
{
    static const std::vector<KeyPattern> patterns = [] {
        std::vector<std::pair<std::string, std::string> > keys;
        for (const MakamDef& m : makamlar()) {
            for (const char* k : m.keys) {
                keys.push_back({ k, m.id });
            }
        }
        std::stable_sort(keys.begin(), keys.end(), [](const auto& a, const auto& b) {
            return a.first.size() > b.first.size();
        });
        std::vector<KeyPattern> list;
        for (const auto& [key, id] : keys) {
            const QString k = QRegularExpression::escape(QString::fromStdString(key));
            list.push_back({ id, QRegularExpression("(^|[^a-z])" + k + "(?![a-z])"),
                             QRegularExpression("^" + k + "(?![a-z])") });
        }
        return list;
    }();
    return patterns;
}

// anywhere: the title ("Uşşak Saz Semaisi"); otherwise the text must start with the name
std::string findMakam(const QString& text, bool anywhere)
{
    const QString n = norm(text);
    if (n.isEmpty()) {
        return {};
    }
    for (const KeyPattern& p : keyPatterns()) {
        if ((anywhere ? p.anywhere : p.atStart).match(n).hasMatch()) {
            return p.id;
        }
    }
    return {};
}

//---------------------------------------------------------
//   reading notes in AEU terms
//---------------------------------------------------------

// Koma shift of an accidental (Turkish reading) or nullopt-like {false} for other accidentals
bool accidentalKoma(AccidentalType type, int& koma)
{
    switch (type) {
    case AccidentalType::NATURAL: koma = 0;
        return true;
    case AccidentalType::FLAT: koma = -5;                  // ♭ = küçük mücennep bemol
        return true;
    case AccidentalType::SHARP: koma = 4;                  // ♯ = bakiye diyez
        return true;
    case AccidentalType::MIRRORED_FLAT: koma = -1;         // ters bemol = koma bemol
        return true;
    case AccidentalType::LOWER_ONE_UNDECIMAL_QUARTERTONE: koma = -1;
        return true;
    case AccidentalType::SHARP_SLASH: koma = 1;            // koma diyez
        return true;
    case AccidentalType::FLAT_SLASH: koma = -4;            // bakiye bemol
        return true;
    case AccidentalType::FLAT_SLASH2: koma = -8;           // büyük mücennep bemol
        return true;
    case AccidentalType::SHARP_SLASH3: koma = 5;           // küçük mücennep diyez
        return true;
    case AccidentalType::SHARP_SLASH2: koma = 8;           // büyük mücennep diyez
        return true;
    default:
        break;
    }
    return false;
}

// Koma shift of a key signature glyph
bool symKoma(SymId sym, int& koma)
{
    switch (sym) {
    case SymId::accidentalQuarterToneFlatStein: koma = -1;
        return true;
    case SymId::accidentalLowerOneUndecimalQuartertone: koma = -1;
        return true;
    case SymId::accidentalBakiyeFlat: koma = -4;
        return true;
    case SymId::accidentalFlat: koma = -5;
        return true;
    case SymId::accidentalBuyukMucennebFlat: koma = -8;
        return true;
    case SymId::accidentalQuarterToneSharpStein: koma = 1;
        return true;
    case SymId::accidentalSharp: koma = 4;
        return true;
    case SymId::accidentalKucukMucennebSharp: koma = 5;
        return true;
    case SymId::accidentalBuyukMucennebSharp: koma = 8;
        return true;
    case SymId::accidentalNatural: koma = 0;
        return true;
    default:
        break;
    }
    return false;
}

// Koma shift per letter (index "CDEFGAB") of the written key signature
std::vector<int> keyKomas(const KeySigEvent& ke)
{
    std::vector<int> komas(7, 0);
    if (!ke.isValid()) {
        return komas;
    }
    const int fifths = static_cast<int>(ke.key());
    static const char* const ORDER = "FCGDAEB";
    static const char* const LETTERS = "CDEFGAB";
    for (int i = 0; i < std::abs(fifths) && i < 7; ++i) {
        const char letter = fifths > 0 ? ORDER[i] : ORDER[6 - i];
        komas[std::string(LETTERS).find(letter)] = fifths > 0 ? 4 : -5;
    }
    if (ke.custom()) {
        for (const CustDef& cd : ke.customKeyDefs()) {
            const int degree = ke.degInKey(cd.degree);
            const SymId sym = ke.symInKey(cd.sym, cd.degree);
            int koma = 0;
            if (degree >= 0 && degree < 7 && symKoma(sym, koma)) {
                komas[degree] = koma;
            }
        }
    }
    return komas;
}

struct NoteInfo {
    std::string perde;     // letter + koma shift, e.g. "B-1" = Segah
    double builtin = 0.0;  // MuseScore's cent offset of the note's accidental
    bool known = true;     // the accidental has a Turkish reading
    bool micro = false;    // not an equal-tempered perde
    double aeu = 0.0;      // AEU position, cents from the written equal-tempered note
    double pos = 0.0;      // pitch position for the melodic direction
};

struct ReadState {
    std::map<int, int> carry;               // natural pitch -> koma of an accidental earlier in the measure
    std::map<const Note*, int> tied;        // first note of a tie chain -> koma
};

NoteInfo noteInfo(Note* note, ReadState& state)
{
    const int tpc = note->tpc2();                                  // the written note
    static const char* const FIFTHS = "FCGDAEB";
    const char letter = FIFTHS[((tpc + 1) % 7 + 7) % 7];
    const int alter = (tpc + 1 + 7) / 7 - 3;                      // ♯/♭ already in the pitch
    const int natPitch = note->pitch() - alter;

    NoteInfo info;
    int k = 0;
    const AccidentalType accType = note->accidentalType();
    const Note* first = note->firstTiedNote();
    if (accType != AccidentalType::NONE) {
        if (accidentalKoma(accType, k)) {
            info.builtin = Accidental::subtype2centOffset(accType);
        } else {
            info.known = false;
            k = 0;
        }
        state.carry[natPitch] = k;                                 // until the end of the measure
    } else if (first && first != note && state.tied.count(first)) {
        k = state.tied[first];                                     // tie continuation: same perde
    } else if (state.carry.count(natPitch)) {
        k = state.carry[natPitch];
    } else if (alter != 0) {
        k = alter > 0 ? 4 : -5;
    } else {
        const Staff* staff = note->staff();
        const KeySigEvent ke = staff ? staff->keySigEvent(note->tick()) : KeySigEvent();
        const std::vector<int> komas = keyKomas(ke);
        const int kk = komas[std::string("CDEFGAB").find(letter)];
        k = (kk == 4 || kk == -5) ? 0 : kk;                        // ♯/♭ of the key are in the pitch
    }
    state.tied[note] = k;

    const int etK = alter > 0 ? 4 : alter < 0 ? -5 : 0;
    info.perde = std::string(1, letter) + std::to_string(k);
    info.micro = k != etK;
    info.aeu = k * KOMA - alter * 100.0;
    info.pos = note->pitch() + (k - etK) / 10.0;
    return info;
}

//---------------------------------------------------------
//   voices
//---------------------------------------------------------

struct Event {
    Chord* chord = nullptr;
    Fraction tick;
    int measureIndex = 0;
    const MakamDef* makam = nullptr;
    std::vector<NoteInfo> infos;
    double pos = -1000.0;
};

bool isDurak(const Chord* chord)
{
    if (!chord || chord->noteType() != NoteType::NORMAL || chord->notes().empty()) {
        return false;
    }
    const Note* note = chord->notes().back();
    const Note* first = note->firstTiedNote();
    const Note* last = note->lastTiedNote();
    if (!first || !last) {
        return false;
    }
    const Fraction start = first->chord()->tick();
    const Fraction end = last->chord()->tick() + last->chord()->actualTicks();
    return end - start >= Fraction(1, 2);
}

enum class Dir {
    Up, Down, Stop
};

Dir directionAt(const std::vector<Event>& events, size_t i)
{
    const double p = events[i].pos;
    for (size_t j = i + 1; j < events.size(); ++j) {
        if (std::abs(events[j].pos - p) > 0.01) {
            return events[j].pos > p ? Dir::Up : Dir::Down;
        }
    }
    for (size_t j = i; j-- > 0;) {
        if (std::abs(events[j].pos - p) > 0.01) {
            return p > events[j].pos ? Dir::Up : Dir::Down;
        }
    }
    return Dir::Up;
}

bool hasPerde(const Event& ev, bool (* test)(const std::string&))
{
    for (const NoteInfo& info : ev.infos) {
        if (test(info.perde)) {
            return true;
        }
    }
    return false;
}

// Yeden regions: the Segah notes right before a La♯, and everything after it until La is not ♯
std::vector<bool> yedenRegions(const std::vector<Event>& events)
{
    std::vector<bool> flags(events.size(), false);
    auto isLaSharp = [](const std::string& perde) { return perde == "A4"; };
    auto isOtherLa = [](const std::string& perde) { return !perde.empty() && perde[0] == 'A' && perde != "A4"; };
    auto isSegah = [](const std::string& perde) { return perde == "B-1"; };
    for (size_t j = 0; j < events.size(); ++j) {
        if (!hasPerde(events[j], isLaSharp)) {
            continue;
        }
        size_t start = j;
        while (start > 0 && hasPerde(events[start - 1], isSegah)) {
            --start;
        }
        size_t end = j + 1;
        while (end < events.size() && !hasPerde(events[end], isOtherLa)) {
            ++end;
        }
        for (size_t f = start; f < end; ++f) {
            flags[f] = true;
        }
    }
    return flags;
}

int writtenPitch(const Note* note)
{
    const Part* part = note->part();
    const Instrument* instrument = part ? part->instrument(note->tick()) : nullptr;
    return note->pitch() - (instrument ? instrument->transpose().chromatic : 0);
}

//---------------------------------------------------------
//   makam of the score
//---------------------------------------------------------

struct Marker {
    Fraction tick;
    const MakamDef* makam = nullptr;
};

// The makam from the melody: the key signature narrows the candidates, then the region where the
// melody starts (seyir) and the last note (karar) decide
const MakamDef* guessFromMelody(MasterScore* score, const std::vector<std::vector<Event> >& firstVoice)
{
    if (firstVoice.empty() || firstVoice.front().empty()) {
        return nullptr;
    }
    const std::vector<Event>& events = firstVoice.front();

    // donanım of the first staff at the start
    std::vector<std::string> sig;
    if (score->nstaves() > 0) {
        const std::vector<int> komas = keyKomas(score->staff(0)->keySigEvent(Fraction(0, 1)));
        for (int i = 0; i < 7; ++i) {
            if (komas[i] != 0) {
                sig.push_back(std::string(1, "CDEFGAB"[i]) + std::to_string(komas[i]));
            }
        }
    }

    double mean = 0.0;
    size_t count = 0;
    for (const Event& ev : events) {
        if (count >= 10) {
            break;
        }
        if (ev.chord->noteType() != NoteType::NORMAL || ev.chord->notes().empty()) {
            continue;
        }
        mean += writtenPitch(ev.chord->notes().back());
        ++count;
    }
    if (count == 0) {
        return nullptr;
    }
    mean /= static_cast<double>(count);

    std::string lastPerde;
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
        if (it->chord->noteType() == NoteType::NORMAL && !it->infos.empty()) {
            lastPerde = it->infos.back().perde;
            break;
        }
    }

    const MakamDef* best = nullptr;
    double bestScore = 0.0;
    double bestSig = 0.0;
    for (const MakamDef& m : makamlar()) {
        double sigScore = 0.0;
        for (const std::vector<const char*>& ks : m.ksLists) {
            size_t common = 0;
            for (const std::string& s : sig) {
                if (std::find_if(ks.begin(), ks.end(), [&s](const char* x) { return s == x; }) != ks.end()) {
                    ++common;
                }
            }
            const size_t unionSize = sig.size() + ks.size() - common;
            sigScore = std::max(sigScore, unionSize == 0 ? 1.0 : static_cast<double>(common) / unionSize);
        }
        const bool kararOk = lastPerde == m.karar;
        const double score = sigScore * 10.0 - std::abs(mean - m.seyir) * 0.5 + (kararOk ? 2.0 : 0.0);
        if (!best || score > bestScore) {
            best = &m;
            bestScore = score;
            bestSig = sigScore;
        }
    }
    return bestSig < 0.5 ? nullptr : best;     // the key signature fits no makam
}

std::vector<Marker> findMarkers(MasterScore* score)
{
    std::vector<Marker> markers;

    // title: score properties and the texts of the frames before the music
    std::vector<QString> titles { score->metaTag(u"workTitle").toQString() };
    for (MeasureBase* mb = score->first(); mb && !mb->isMeasure(); mb = mb->next()) {
        for (EngravingItem* e : mb->el()) {
            if (e && e->isTextBase()) {
                titles.push_back(toTextBase(e)->plainText().toQString());
            }
        }
    }
    for (const QString& t : titles) {
        const std::string id = findMakam(t, true);
        if (!id.empty()) {
            markers.push_back({ Fraction(0, 1), makamById(id) });
            break;
        }
    }

    // staff, system texts and rehearsal marks that start with a makam name
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        for (Segment* s = m->first(); s; s = s->next()) {
            for (EngravingItem* e : s->annotations()) {
                if (!e || !(e->isStaffText() || e->isSystemText() || e->isRehearsalMark())) {
                    continue;
                }
                const std::string id = findMakam(toTextBase(e)->plainText().toQString(), false);
                if (!id.empty()) {
                    markers.push_back({ s->tick(), makamById(id) });
                }
            }
        }
    }
    std::stable_sort(markers.begin(), markers.end(), [](const Marker& a, const Marker& b) { return a.tick < b.tick; });
    return markers;
}

const MakamDef* makamAt(const std::vector<Marker>& markers, const Fraction& tick)
{
    const MakamDef* makam = nullptr;
    for (const Marker& m : markers) {
        if (m.tick <= tick) {
            makam = m.makam;
        }
    }
    return makam;
}
}

//---------------------------------------------------------
//   tuneScore
//---------------------------------------------------------

void tuneScore(MasterScore* score)
{
    if (!score) {
        return;
    }

    // the chords of every pitched voice, in order (grace notes before their chord)
    std::map<track_idx_t, std::vector<Event> > voices;
    int measureIndex = -1;
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        ++measureIndex;
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            for (track_idx_t track = 0; track < score->ntracks(); ++track) {
                EngravingItem* e = s->element(track);
                if (!e || !e->isChord()) {
                    continue;
                }
                const Staff* staff = score->staff(track / VOICES);
                if (!staff || !staff->isPitchedStaff(s->tick())) {
                    continue;
                }
                Chord* chord = toChord(e);
                for (Chord* grace : chord->graceNotes()) {
                    voices[track].push_back({ grace, s->tick(), measureIndex });
                }
                voices[track].push_back({ chord, s->tick(), measureIndex });
            }
        }
    }
    if (voices.empty()) {
        return;
    }

    // read the notes
    for (auto& [track, events] : voices) {
        ReadState state;
        int carryMeasure = -1;
        for (Event& ev : events) {
            if (ev.measureIndex != carryMeasure) {
                state.carry.clear();                   // accidentals end at the barline
                carryMeasure = ev.measureIndex;
            }
            for (Note* note : ev.chord->notes()) {
                NoteInfo info = noteInfo(note, state);
                if (info.pos > ev.pos) {
                    ev.pos = info.pos;                 // the top note carries the melody
                }
                ev.infos.push_back(info);
            }
        }
    }

    // the makam of every event
    std::vector<Marker> markers = findMarkers(score);
    if (markers.empty() || markers.front().tick > Fraction(0, 1)) {
        std::vector<std::vector<Event> > firstVoice { voices.begin()->second };
        const MakamDef* guess = guessFromMelody(score, firstVoice);
        markers.insert(markers.begin(), { Fraction(0, 1), guess ? guess : makamById(FALLBACK_MAKAM) });
    }

    // tune
    for (auto& [track, events] : voices) {
        const std::vector<bool> yeden = yedenRegions(events);
        for (size_t i = 0; i < events.size(); ++i) {
            Event& ev = events[i];
            const MakamDef* makam = makamAt(markers, ev.tick);
            const Dir dir = isDurak(ev.chord) ? Dir::Stop : directionAt(events, i);
            const std::vector<Note*>& notes = ev.chord->notes();
            for (size_t n = 0; n < notes.size() && n < ev.infos.size(); ++n) {
                const NoteInfo& info = ev.infos[n];
                double cents = 0.0;
                const Calib* segah = info.known ? calibOf(makam, "B-1") : nullptr;
                const Calib* c = info.known ? calibOf(makam, info.perde) : nullptr;
                if (segah && (info.perde == "A4" || (info.perde == "B-1" && yeden[i]))) {
                    cents = YEDEN_SEGAH - info.builtin;            // yedenli Segah, La♯ 6 mandal below it
                } else if (c) {
                    const double v = dir == Dir::Up ? c->up : dir == Dir::Down ? c->down : c->stop;
                    cents = v - info.builtin;
                } else if (info.known && info.micro) {
                    cents = info.aeu - info.builtin;               // microtonal perde without calibration
                }
                cents = std::round(cents * 10.0) / 10.0;
                Note* note = notes[n];
                if (std::abs(note->tuning() - cents) > 0.05) {
                    note->undoChangeProperty(Pid::TUNING, cents);
                }
            }
        }
    }
}
}
