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

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QString>

#include "settings.h"

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
#include "../dom/factory.h"
#include "../dom/interval.h"
#include "../dom/select.h"
#include "../dom/tie.h"

#include "transpose.h"

namespace mu::engraving::makam {
namespace {
struct Calib {
    double up = 0.0;
    double down = 0.0;
    double stop = 0.0;
};

struct MakamDef {
    const char* id = "";
    const char* name = "";                                  // UTF-8
    std::vector<const char*> keys;                         // names in the score text (normalized)
    std::vector<std::pair<const char*, Calib> > calib;      // perde -> cents from the written ET note
    std::vector<std::vector<const char*> > ksLists;         // donanıms the makam is written with
    const char* karar = "";                                 // final perde
    double seyir = 0.0;                                     // written MIDI pitch where the melody starts
};

const std::vector<MakamDef>& makamlar()
{
    static const std::vector<MakamDef> list {
        { "ussak", "Uşşak",
          { "ussak" },
          { { "B-1", { -50.0, -66.67, -50.0 } } },
          { { "B-1" } },
          "A0", 71.9 },
        { "rast", "Rast",
          { "rast" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "F4", { -16.67, -33.33, -16.67 } } },
          { { "B-1", "F4" } },
          "G0", 70.6 },
        { "huzzam", "Hüzzam",
          { "huzzam" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "E-4", { -50.0, -66.67, -16.67 } }, { "F4", { -33.33, -33.33, -16.67 } }, { "E4", { -33.33, -33.33, -33.33 } }, { "E-1", { -16.67, -16.67, -16.67 } } },
          { { "B-1", "E-4", "F4" } },
          "B-1", 74.9 },
        { "hicaz", "Hicaz",
          { "hicaz" },
          { { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-4", "C4" }, { "B-4", "C4", "F4" } },
          "A0", 74.1 },
        { "huseyni", "Hüseyni",
          { "huseyni" },
          { { "B-1", { -33.33, -50.0, -33.33 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-1", "F4" } },
          "A0", 75.5 },
        { "segah", "Segah",
          { "segah" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "E-1", { 0.0, -16.67, 0.0 } }, { "F4", { -16.67, -50.0, -16.67 } }, { "E4", { -33.33, -33.33, -33.33 } } },
          { { "B-1", "E-1", "F4" } },
          "B-1", 73.3 },
        { "saba", "Saba",
          { "saba" },
          { { "B-1", { -33.33, -50.0, -33.33 } }, { "D-4", { -66.67, -66.67, -66.67 } } },
          { { "B-1", "D-4" } },
          "A0", 71.9 },
        { "nihavend", "Nihavend",
          { "nihavend", "nihavent" },
          {  },
          { { "B-5", "E-5" } },
          "G0", 72.5 },
        { "kurdi", "Kürdi",
          { "kurdi" },
          {  },
          { { "B-5" } },
          "A0", 74.0 },
        { "muhayyerkurdi", "Muhayyerkürdi",
          { "muhayyerkurdi", "muhayyer kurdi" },
          {  },
          { { "B-5" } },
          "A0", 78.9 },
        { "muhayyer", "Muhayyer",
          { "muhayyer" },
          { { "B-1", { -33.33, -50.0, -33.33 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-1", "F4" } },
          "A0", 78.5 },
        { "acemkurdi", "Acemkürdi",
          { "acemkurdi", "acem kurdi" },
          {  },
          { { "B-5" } },
          "A0", 76.5 },
        { "bayati", "Bayati",
          { "bayati", "beyati" },
          { { "B-1", { -50.0, -66.67, -50.0 } } },
          { { "B-1" } },
          "A0", 73.8 },
        { "evic", "Eviç",
          { "evic" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "F4", { -16.67, -33.33, -16.67 } } },
          { { "B-1", "F4" } },
          "F4", 77.2 },
        { "kurdilihicazkar", "Kürdilihicazkâr",
          { "kurdilihicazkar", "kurdili hicazkar" },
          {  },
          { { "A-5", "B-5", "E-5" } },
          "G0", 77.0 },
        { "hicazuzzal", "Hicaz Uzzal",
          { "hicaz uzzal", "hicazuzzal", "uzzal hicaz", "uzzal" },
          { { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "F4", { -33.33, -50.0, -33.33 } } },
          { { "B-4", "C4" } },
          "A0", 75.3 },
        { "zirgulelihicaz", "Zirgüleli Hicaz",
          { "zirguleli hicaz", "zirgulelihicaz", "hicaz zirgule" },
          { { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "G4", { -16.67, -16.67, -16.67 } } },
          { { "B-4", "C4", "G4" } },
          "A0", 72.6 },
        { "sedaraban", "Şedaraban",
          { "sedaraban", "sedd-i araban", "seddi araban", "sed araban", "sedd araban" },
          { { "E-4", { -83.33, -83.33, -83.33 } }, { "F4", { -16.67, -16.67, -16.67 } }, { "B-4", { -83.33, -83.33, -83.33 } }, { "C4", { -16.67, -16.67, -16.67 } } },
          { { "B-4", "E-4", "F4", "C4" } },
          "D0", 74.5 },
        { "buselik", "Buselik",
          { "buselik" },
          {  },
          { {  } },
          "A0", 72.7 },
        { "cargah", "Çargah",
          { "cargah", "yeni cargah" },
          {  },
          { {  } },
          "C0", 73.1 },
        { "mahur", "Mahur",
          { "mahur" },
          { { "F5", { 116.67, 116.67, 116.67 } } },
          { { "F5" } },
          "G0", 78.2 },
        { "acemasiran", "Acemaşiran",
          { "acemasiran", "acem asiran" },
          {  },
          { { "B-5" } },
          "F0", 76.4 },
        { "evcara", "Evcârâ",
          { "evcara", "evc ara", "evc-ara", "evara" },
          { { "B-1", { -16.67, -16.67, -16.67 } }, { "F4", { -16.67, -33.33, -16.67 } }, { "C4", { -16.67, -16.67, -16.67 } }, { "E4", { -16.67, -16.67, -16.67 } } },
          { { "B-1", "F4", "C4", "E4", "A4" } },
          "F4", 78.0 },
        { "suzidil", "Suzidil",
          { "suzidil", "suzi dil" },
          { { "F1", { 16.67, 16.67, 16.67 } }, { "G4", { -16.67, -16.67, -16.67 } }, { "D4", { -16.67, -16.67, -16.67 } } },
          { { "F1", "G4", "D4" } },
          "E0", 76.0 },
        { "karcigar", "Karcığar",
          { "karcigar" },
          { { "B-1", { -33.33, -66.67, -33.33 } }, { "E-4", { -50.0, -66.67, -50.0 } }, { "F4", { -33.33, -33.33, -33.33 } } },
          { { "B-1", "E-4", "F4" } },
          "A0", 74.5 },
        { "suzinak", "Suzinak",
          { "suzinak" },
          { { "A-4", { -83.33, -83.33, -83.33 } }, { "B-1", { -16.67, -16.67, -16.67 } }, { "E-4", { -83.33, -83.33, -83.33 } }, { "F4", { -16.67, -16.67, -16.67 } } },
          { { "B-1", "E-4", "F4" } },
          "G0", 73.3 },
        { "hicazkar", "Hicazkâr",
          { "hicazkar" },
          { { "A-4", { -83.33, -83.33, -83.33 } }, { "B-1", { 0.0, -16.67, 0.0 } }, { "E-4", { -83.33, -66.67, -83.33 } }, { "F4", { 0.0, -16.67, 0.0 } } },
          { { "A-4", "B-1", "E-4", "F4" } },
          "G0", 77.5 },
    };
    return list;
}

constexpr double KOMA = 1200.0 / 53.0;
constexpr double DEFAULT_YEDEN_SEGAH = -16.67;    // Segah while La♯ (yeden) is active, cents from Si
const char* const FALLBACK_MAKAM = "ussak";

const muse::Settings::Key CALIBRATION_KEY("makam", "makam/calibration");
const muse::Settings::Key YEDEN_KEY("makam", "makam/yedenSegah");

const MakamDef* makamById(const std::string& id)
{
    for (const MakamDef& m : makamlar()) {
        if (id == m.id) {
            return &m;
        }
    }
    return nullptr;
}

//---------------------------------------------------------
//   user calibration (app settings), same JSON as the Makam plugin:
//   { makamId: { perde: { "up": cents, "down": cents, "stop": cents } } }
//---------------------------------------------------------

using UserCalib = std::map<std::string, std::map<std::string, Calib> >;

QString savedCalibrationJson()
{
    return muse::settings()->value(CALIBRATION_KEY).toQString();
}

const UserCalib& userCalib()
{
    static QString cachedJson;
    static UserCalib cached;
    static bool loaded = false;
    const QString json = savedCalibrationJson();
    if (loaded && json == cachedJson) {
        return cached;
    }
    loaded = true;
    cachedJson = json;
    cached.clear();
    const QJsonObject root = QJsonDocument::fromJson(json.toUtf8()).object();
    for (auto mit = root.begin(); mit != root.end(); ++mit) {
        const QJsonObject perdes = mit.value().toObject();
        for (auto pit = perdes.begin(); pit != perdes.end(); ++pit) {
            const QJsonObject v = pit.value().toObject();
            Calib c;
            c.up = v.value("up").toDouble();
            c.down = v.value("down").toDouble();
            c.stop = v.value("stop").toDouble(c.up);
            cached[mit.key().toStdString()][pit.key().toStdString()] = c;
        }
    }
    return cached;
}

void saveUserCalib(const UserCalib& calib)
{
    QJsonObject root;
    for (const auto& [makamId, perdes] : calib) {
        QJsonObject obj;
        for (const auto& [perde, c] : perdes) {
            QJsonObject v;
            v.insert("up", c.up);
            v.insert("down", c.down);
            v.insert("stop", c.stop);
            obj.insert(QString::fromStdString(perde), v);
        }
        if (!obj.isEmpty()) {
            root.insert(QString::fromStdString(makamId), obj);
        }
    }
    const QString json = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    muse::settings()->setSharedValue(CALIBRATION_KEY, muse::Val(json));
}

double currentYedenSegah()
{
    const muse::Val v = muse::settings()->value(YEDEN_KEY);
    return v.isNull() ? DEFAULT_YEDEN_SEGAH : v.toDouble();
}

// The value of a calibrated perde: the user's value, else the default; nullptr when not calibrated
bool calibOf(const MakamDef* makam, const std::string& perde, Calib& result)
{
    if (!makam) {
        return false;
    }
    bool found = false;
    for (const auto& [p, c] : makam->calib) {
        if (perde == p) {
            result = c;
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }
    const UserCalib& user = userCalib();
    auto mit = user.find(makam->id);
    if (mit != user.end()) {
        auto pit = mit->second.find(perde);
        if (pit != mit->second.end()) {
            result = pit->second;
        }
    }
    return true;
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
    QString source;
};

// The makam from the melody: the key signature narrows the candidates, then the region where the
// melody starts (seyir) and the last note (karar) decide
const MakamDef* guessFromMelody(MasterScore* score, const std::vector<Event>& events)
{
    if (events.empty()) {
        return nullptr;
    }

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
            markers.push_back({ Fraction(0, 1), makamById(id), QString::fromUtf8("başlık") });
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
                    const char* source = e->isStaffText() ? "dizek metni" : e->isSystemText() ? "sistem metni" : "prova işareti";
                    markers.push_back({ s->tick(), makamById(id), QString::fromUtf8(source) });
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

using Voices = std::map<track_idx_t, std::vector<Event> >;

// The chords of every pitched voice in order (grace notes before their chord), read in AEU terms
Voices readVoices(MasterScore* score)
{
    Voices voices;
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
    return voices;
}

// The makam markers, with a guessed (or the fallback) makam at the start when no text names one
std::vector<Marker> scoreMarkers(MasterScore* score, const Voices& voices)
{
    std::vector<Marker> markers = findMarkers(score);
    if (markers.empty() || markers.front().tick > Fraction(0, 1)) {
        const MakamDef* guess = voices.empty() ? nullptr : guessFromMelody(score, voices.begin()->second);
        if (guess) {
            markers.insert(markers.begin(), { Fraction(0, 1), guess, QString::fromUtf8("ezgiden tahmin") });
        } else {
            markers.insert(markers.begin(), { Fraction(0, 1), makamById(FALLBACK_MAKAM), QString::fromUtf8("varsayılan") });
        }
    }
    return markers;
}

const char* const LETTER_NAMES[] = { "Do", "Re", "Mi", "Fa", "Sol", "La", "Si" };
}

//---------------------------------------------------------
//   tuneScore
//---------------------------------------------------------

void tuneScore(MasterScore* score)
{
    if (!score) {
        return;
    }

    Voices voices = readVoices(score);
    if (voices.empty()) {
        return;
    }
    const std::vector<Marker> markers = scoreMarkers(score, voices);
    const double yedenSegah = currentYedenSegah();

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
                Calib segah;
                Calib c;
                const bool hasSegah = info.known && calibOf(makam, "B-1", segah);
                const bool hasCalib = info.known && calibOf(makam, info.perde, c);
                if (hasSegah && (info.perde == "A4" || (info.perde == "B-1" && yeden[i]))) {
                    cents = yedenSegah - info.builtin;             // yedenli Segah, La♯ 6 mandal below it
                } else if (hasCalib) {
                    const double v = dir == Dir::Up ? c.up : dir == Dir::Down ? c.down : c.stop;
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

//---------------------------------------------------------
//   calibration (for the Makam window)
//---------------------------------------------------------

std::vector<MakamName> allMakams()
{
    std::vector<MakamName> list;
    for (const MakamDef& m : makamlar()) {
        list.push_back({ m.id, QString::fromUtf8(m.name) });
    }
    return list;
}

bool makamBasics(const std::string& makamId, MakamBasics& result)
{
    const MakamDef* makam = makamById(makamId);
    if (!makam) {
        return false;
    }
    result.id = makam->id;
    result.name = QString::fromUtf8(makam->name);
    result.karar = makam->karar;
    result.donanim.clear();
    if (!makam->ksLists.empty()) {
        for (const char* perde : makam->ksLists.front()) {
            result.donanim.push_back(perde);
        }
    }
    return true;
}

std::vector<PerdeValues> calibration(const std::string& makamId, bool defaults)
{
    std::vector<PerdeValues> list;
    const MakamDef* makam = makamById(makamId);
    if (!makam) {
        return list;
    }
    for (const auto& [perde, def] : makam->calib) {
        Calib c = def;
        if (!defaults) {
            calibOf(makam, perde, c);
        }
        list.push_back({ perde, c.up, c.down, c.stop });
    }
    return list;
}

void setCalibration(const std::string& makamId, const PerdeValues& values)
{
    UserCalib user = userCalib();
    user[makamId][values.perde] = Calib { values.up, values.down, values.stop };
    saveUserCalib(user);
}

void resetCalibration(const std::string& makamId)
{
    UserCalib user = userCalib();
    if (makamId.empty()) {
        user.clear();
    } else {
        user.erase(makamId);
    }
    saveUserCalib(user);
}

double yedenSegah()
{
    return currentYedenSegah();
}

void setYedenSegah(double cents)
{
    muse::settings()->setSharedValue(YEDEN_KEY, muse::Val(cents));
}

double defaultYedenSegah()
{
    return DEFAULT_YEDEN_SEGAH;
}

QString perdeLabel(const std::string& perde)
{
    if (perde.size() < 2) {
        return QString::fromStdString(perde);
    }
    static const std::string LETTERS = "CDEFGAB";
    const size_t degree = LETTERS.find(perde[0]);
    const int koma = std::atoi(perde.c_str() + 1);
    const char* word = "";
    switch (koma) {
    case -1: word = "koma bemol";
        break;
    case -4: word = "bakiye bemol";
        break;
    case -5: word = "bemol";
        break;
    case -8: word = "büyük mücennep bemol";
        break;
    case 1: word = "koma diyez";
        break;
    case 4: word = "diyez";
        break;
    case 5: word = "küçük mücennep diyez";
        break;
    case 8: word = "büyük mücennep diyez";
        break;
    default: break;
    }
    const QString name = degree == std::string::npos ? QString::fromLatin1(perde.c_str(), 1) : QString::fromUtf8(LETTER_NAMES[degree]);
    return *word ? name + " " + QString::fromUtf8(word) : name;
}

int perdeAlter(const std::string& perde)
{
    const int koma = perde.size() > 1 ? std::atoi(perde.c_str() + 1) : 0;
    return koma == 4 ? 1 : koma == -5 ? -1 : 0;
}

std::vector<ScoreMakam> scoreMakams(MasterScore* score)
{
    std::vector<ScoreMakam> list;
    if (!score) {
        return list;
    }
    const Voices voices = readVoices(score);
    for (const Marker& m : scoreMarkers(score, voices)) {
        if (!m.makam) {
            continue;
        }
        const Measure* measure = score->tick2measure(m.tick);
        list.push_back({ m.makam->id, QString::fromUtf8(m.makam->name), measure ? measure->no() + 1 : 1, m.source });
    }
    return list;
}
}

//---------------------------------------------------------
//   Türk arıza ↑/↓: the Turkish (AEU) accidental ladder of the TurkAriza plugins (v0.7)
//     −3 küçük mücennep bemol (♭)  −2 bakiye bemol  −1 koma bemol  0 naturel
//     +1 koma diyez  +2 bakiye diyez (♯)  +3 küçük mücennep diyez
//   Above the top step the next note's naturel follows (Fa +3 → Sol). Mi–Fa and Si–Do are only
//   4 komas apart: Mi ↑ Mi♯ ↑ Fa, Si ↑ Do. Koma diyez only on Fa. Going down: Fa ↓ Mi♯, Do ↓ Si.
//   An accidental is written only where the step differs from the key signature or from an
//   earlier accidental on the same note in the measure.
//---------------------------------------------------------

namespace mu::engraving::makam {
namespace {
constexpr int NO_STEP = 99;

const std::string LADDER_LETTERS = "CDEFGAB";
constexpr int NAT_TPC[7] = { 14, 16, 18, 13, 15, 17, 19 };   // C D E F G A B
constexpr int SEMIS_UP[7] = { 2, 2, 1, 2, 2, 2, 1 };          // to the next letter

const std::vector<int>& ladder(char letter)
{
    static const std::vector<int> C { 0, 2, 3 };              // no bemol steps: Do ↓ Si
    static const std::vector<int> E { -3, -2, -1, 0, 2 };     // only ♯ (4 komas): Mi ↑ Mi♯ ↑ Fa
    static const std::vector<int> F { 0, 1, 2, 3 };           // no bemol steps: Fa ↓ Mi♯
    static const std::vector<int> B { -3, -2, -1, 0 };        // no diyez steps: Si ↑ Do
    static const std::vector<int> other { -3, -2, -1, 0, 2, 3 };
    switch (letter) {
    case 'C': return C;
    case 'E': return E;
    case 'F': return F;
    case 'B': return B;
    default: break;
    }
    return other;
}

int nextLadderStep(char letter, int step, int direction)
{
    const std::vector<int>& list = ladder(letter);
    if (direction > 0) {
        for (int s : list) {
            if (s > step) {
                return s;
            }
        }
    } else {
        for (auto it = list.rbegin(); it != list.rend(); ++it) {
            if (*it < step) {
                return *it;
            }
        }
    }
    return NO_STEP;
}

int stepOfAccidental(AccidentalType type)
{
    switch (type) {
    case AccidentalType::FLAT: return -3;
    case AccidentalType::FLAT_SLASH: return -2;
    case AccidentalType::MIRRORED_FLAT: return -1;
    case AccidentalType::LOWER_ONE_UNDECIMAL_QUARTERTONE: return -1;
    case AccidentalType::NATURAL: return 0;
    case AccidentalType::SHARP_SLASH: return 1;
    case AccidentalType::SHARP: return 2;
    case AccidentalType::SHARP_SLASH3: return 3;
    default: break;
    }
    return NO_STEP;
}

AccidentalType accidentalOfStep(int step)
{
    switch (step) {
    case -3: return AccidentalType::FLAT;
    case -2: return AccidentalType::FLAT_SLASH;        // bakiye bemol
    case -1: return AccidentalType::MIRRORED_FLAT;     // koma bemol (ters bemol)
    case 0: return AccidentalType::NATURAL;
    case 1: return AccidentalType::SHARP_SLASH;        // koma diyez
    case 2: return AccidentalType::SHARP;
    case 3: return AccidentalType::SHARP_SLASH3;       // küçük mücennep diyez
    default: break;
    }
    return AccidentalType::NONE;
}

int stepOfKoma(int koma)
{
    switch (koma) {
    case -5: return -3;
    case -4: return -2;
    case -1: return -1;
    case 1: return 1;
    case 4: return 2;
    case 5: return 3;
    default: break;
    }
    return 0;
}

// The written ♯/♭ (equal-tempered) of a step
int alterOfStep(int step)
{
    return step == -3 ? -1 : step == 2 ? 1 : 0;
}

void letterOfTpc(int tpc, char& letter, int& alter)
{
    static const char* const FIFTHS = "FCGDAEB";
    letter = FIFTHS[((tpc + 1) % 7 + 7) % 7];
    alter = (tpc + 1 + 7) / 7 - 3;
}

// Step of the last accidental on the same note (letter and written natural pitch) earlier in the
// measure, on the same staff in any voice; NO_STEP when there is none
int measureStep(const Note* note, char letter, int natWritten)
{
    const Chord* chord = note->chord();
    const Segment* segment = chord ? chord->segment() : nullptr;
    const Measure* measure = segment ? segment->measure() : nullptr;
    if (!measure) {
        return NO_STEP;
    }
    const track_idx_t firstTrack = note->staffIdx() * VOICES;
    int found = NO_STEP;
    for (Segment* s = measure->first(SegmentType::ChordRest); s && s->tick() < segment->tick(); s = s->next(SegmentType::ChordRest)) {
        for (track_idx_t track = firstTrack; track < firstTrack + VOICES; ++track) {
            EngravingItem* e = s->element(track);
            if (!e || !e->isChord()) {
                continue;
            }
            for (const Note* n : toChord(e)->notes()) {
                if (n->accidentalType() == AccidentalType::NONE) {
                    continue;
                }
                char l = 'C';
                int a = 0;
                letterOfTpc(n->tpc2(), l, a);
                if (l != letter || writtenPitch(n) - a != natWritten) {
                    continue;
                }
                const int st = stepOfAccidental(n->accidentalType());
                found = st != NO_STEP ? st : (a > 0 ? 2 : a < 0 ? -3 : 0);
            }
        }
    }
    return found;
}

// The step a note without its own accidental is on: an earlier accidental in the measure, else
// the key signature
int currentStep(const Note* note, char letter, int natWritten)
{
    const int ms = measureStep(note, letter, natWritten);
    if (ms != NO_STEP) {
        return ms;
    }
    const Staff* staff = note->staff();
    if (!staff) {
        return 0;
    }
    const std::vector<int> komas = keyKomas(staff->keySigEvent(note->tick()));
    return stepOfKoma(komas[LADDER_LETTERS.find(letter)]);
}

void removeAccidental(Note* note)
{
    for (EngravingObject* obj : note->linkList()) {
        Note* ln = toNote(obj);
        if (Accidental* acc = ln->accidental()) {
            ln->score()->undoRemoveElement(acc);
        }
    }
}

// Writes the note (and its tie chain) as letter + step
void setNoteStep(Score* score, Note* note, char letter, int natWritten, int step)
{
    const int ks = currentStep(note, letter, natWritten);
    const int alter = alterOfStep(step);
    const int written = natWritten + alter;
    const int tpc2 = NAT_TPC[LADDER_LETTERS.find(letter)] + 7 * alter;
    const Part* part = note->part();
    const Interval v = part ? part->instrument(note->tick())->transpose() : Interval();
    const int pitch = written + v.chromatic;
    const int tpc1 = v.isZero() ? tpc2 : Transpose::transposeTpc(tpc2, v, true);

    removeAccidental(note);
    score->undoChangePitch(note, pitch, tpc1, tpc2);
    if (ks != step) {
        for (EngravingObject* obj : note->linkList()) {
            Note* ln = toNote(obj);
            Accidental* acc = Factory::createAccidental(ln);
            acc->setParent(ln);
            acc->setAccidentalType(accidentalOfStep(step));
            acc->setRole(AccidentalRole::USER);
            ln->score()->undoAddElement(acc);
        }
    }

    // the tied notes follow
    for (Note* n = note; n->tieFor() && n->tieFor()->endNote();) {
        n = n->tieFor()->endNote();
        removeAccidental(n);
        score->undoChangePitch(n, pitch, tpc1, tpc2);
    }
}
}

bool turkishStep(Score* score, int direction)
{
    if (!score) {
        return false;
    }
    std::vector<Note*> notes;
    for (Note* n : score->selection().noteList()) {
        const Staff* staff = n->staff();
        if (!staff || !staff->isPitchedStaff(n->tick())) {
            continue;
        }
        if (n->tieBack()) {
            continue;                                  // the tie chain follows its first note
        }
        notes.push_back(n);
    }
    if (notes.empty()) {
        return false;
    }

    for (Note* note : notes) {
        char letter = 'C';
        int alter = 0;
        letterOfTpc(note->tpc2(), letter, alter);
        if (alter < -1 || alter > 1) {
            continue;                                  // double sharps / flats: not in the ladder
        }
        int natWritten = writtenPitch(note) - alter;
        int step = note->accidentalType() != AccidentalType::NONE ? stepOfAccidental(note->accidentalType()) : NO_STEP;
        if (step == NO_STEP) {
            step = currentStep(note, letter, natWritten);
        }

        int next = nextLadderStep(letter, step, direction);
        if (next == NO_STEP) {
            size_t index = LADDER_LETTERS.find(letter);
            if (direction > 0) {
                natWritten += SEMIS_UP[index];
                index = (index + 1) % 7;
                next = 0;
            } else {
                index = (index + 6) % 7;
                natWritten -= SEMIS_UP[index];
                next = LADDER_LETTERS[index] == 'E' ? 2 : 0;   // Fa ↓ Mi♯
            }
            letter = LADDER_LETTERS[index];
        }
        setNoteStep(score, note, letter, natWritten, next);
    }

    score->setPlayNote(true);
    score->setSelectionChanged(true);
    return true;
}
}
