/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: data of Turkish makam music used by the palettes and by the time signature command.
 * Taken from the Makam plugin (Makam.qml: usuls, donanimPalette, komaSyms).
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

#include <optional>
#include <string>
#include <vector>

#include "../types/fraction.h"
#include "../types/groupnode.h"
#include "../types/symid.h"
#include "groups.h"
#include "key.h"

namespace mu::engraving::makam {
//---------------------------------------------------------
//   Usuls
//---------------------------------------------------------

// One stroke: düm (lower line, stem down) or tek (upper line, stem up), length in 1/d of the usul
struct Darb {
    bool dum = true;
    int len = 1;
};

struct Usul {
    int code = 0;              // stable number, stored in the time signature (see usulGroups)
    std::string name;          // UTF-8
    int n = 4;
    int d = 4;
    std::vector<Darb> darb;
    std::vector<int> beams;    // beam groups in 1/d; empty: MuseScore's default grouping
};

// Pitches and drum lines of the usul staff (instrument "usul" in instruments.xml)
static constexpr int USUL_DUM_PITCH = 36;   // Bass Drum 1
static constexpr int USUL_TEK_PITCH = 43;   // High Floor Tom
static constexpr const char16_t* USUL_INSTRUMENT_ID = u"usul";

// The time signature of an usul carries one extra beam group node far beyond the end of any
// measure, with no action: pos = USUL_MARKER_BASE + code. It does not change the beaming
// and tells which usul the time signature is (several usuls share 9/8, 7/8, 8/8).
static constexpr int USUL_MARKER_BASE = 10000;

inline const std::vector<Usul>& usuller()
{
    static const std::vector<Darb> aksakSemai { { true, 2 }, { true, 1 }, { false, 2 }, { true, 2 }, { false, 2 }, { false, 1 } };
    static const std::vector<int> aksakSemaiBeams { 2, 1, 2, 2, 2, 1 };

    static const std::vector<Usul> list {
        { 1,  "Nim Sofyan",   2,  4,  { { true, 1 }, { false, 1 } }, {} },
        { 2,  "Semai",        3,  4,  { { true, 1 }, { false, 1 }, { false, 1 } }, {} },
        { 3,  "Sofyan",       4,  4,  { { true, 2 }, { true, 1 }, { false, 1 } }, {} },
        { 4,  "Türk Aksağı",  5,  8,  { { true, 2 }, { false, 2 }, { false, 1 } }, {} },
        { 5,  "Yürük Semai",  6,  8,  { { true, 1 }, { false, 1 }, { false, 1 }, { true, 1 }, { false, 2 } }, {} },
        { 6,  "Devr-i Hindi", 7,  8,  { { true, 1 }, { false, 1 }, { false, 1 }, { true, 2 }, { false, 2 } }, {} },
        { 7,  "Devr-i Turan", 7,  8,  { { true, 2 }, { false, 2 }, { false, 3 } }, {} },
        { 8,  "Düyek",        8,  8,  { { true, 1 }, { false, 2 }, { false, 1 }, { true, 2 }, { false, 2 } }, {} },
        { 9,  "Müsemmen",     8,  8,  { { true, 3 }, { false, 2 }, { false, 3 } }, {} },
        { 10, "Aksak",        9,  8,  { { true, 2 }, { true, 1 }, { false, 1 }, { true, 2 }, { false, 2 }, { false, 1 } }, {} },
        { 11, "Evfer",        9,  8,  { { true, 2 }, { true, 1 }, { false, 1 }, { true, 2 }, { false, 1 }, { false, 2 } }, {} },
        { 12, "Raks Aksağı",  9,  8,  { { true, 2 }, { false, 3 }, { true, 2 }, { false, 2 } }, {} },
        { 13, "Oynak",        9,  8,  { { true, 1 }, { false, 1 }, { false, 1 }, { true, 2 }, { false, 2 }, { false, 2 } }, {} },
        { 14, "Aksak Semai",  10, 8,  aksakSemai, aksakSemaiBeams },
        { 15, "Curcuna",      10, 16, aksakSemai, aksakSemaiBeams },
        { 16, "Curcuna",      10, 8,  aksakSemai, aksakSemaiBeams },
        { 17, "Cengi Harbi",  10, 8,  { { true, 1 }, { false, 1 }, { true, 1 }, { false, 1 }, { true, 1 }, { false, 1 },
                                        { false, 1 }, { true, 1 }, { false, 1 }, { false, 1 } }, {} },
    };
    return list;
}

// An usul the user created ("Oluştur" in the Usul tool) has the marker code USUL_CUSTOM_CODE and
// carries itself in more nodes after the marker, also far beyond any measure (Groups::beamMode
// stops at the first node past the note, so they never change the beaming):
//   pos = USUL_DARB_BASE + i, action = len * 2 + (düm ? 1 : 0)   the strokes, in order
//   pos = USUL_NAME_BASE + i, action = byte                        the name, UTF-8
// So the score file, the palette cell and a copy on another computer all know the usul.
static constexpr int USUL_CUSTOM_CODE = 1000;
static constexpr int USUL_DARB_BASE = 20000;
static constexpr int USUL_NAME_BASE = 30000;
static constexpr int USUL_MAX_ITEMS = 9999;

// Beam groups of an usul's time signature, with the usul marker after the beam nodes
inline Groups usulGroups(const Usul& usul)
{
    GroupNodes nodes;
    if (usul.beams.empty()) {
        nodes = Groups::endings(Fraction(usul.n, usul.d)).nodes();
    } else {
        int pos = 0;
        for (size_t i = 0; i + 1 < usul.beams.size(); ++i) {
            pos += usul.beams[i] * 32 / usul.d;
            nodes.push_back(GroupNode { pos, 0x111 });   // break the 8th, 16th and 32nd beams
        }
    }
    nodes.push_back(GroupNode { USUL_MARKER_BASE + usul.code, 0 });

    if (usul.code == USUL_CUSTOM_CODE) {
        for (size_t i = 0; i < usul.darb.size() && i < USUL_MAX_ITEMS; ++i) {
            nodes.push_back(GroupNode { USUL_DARB_BASE + int(i), usul.darb[i].len * 2 + (usul.darb[i].dum ? 1 : 0) });
        }
        for (size_t i = 0; i < usul.name.size() && i < USUL_MAX_ITEMS; ++i) {
            nodes.push_back(GroupNode { USUL_NAME_BASE + int(i), int(static_cast<unsigned char>(usul.name[i])) });
        }
    }
    return Groups(nodes);
}

// The usul of a time signature, from its marker node; nothing if it is not an usul
inline std::optional<Usul> usulOfGroups(const Groups& groups, const Fraction& sig)
{
    const GroupNode* marker = nullptr;
    for (const GroupNode& node : groups.nodes()) {
        if (node.pos >= USUL_MARKER_BASE && node.pos < USUL_DARB_BASE) {
            marker = &node;
            break;
        }
    }
    if (!marker) {
        return std::nullopt;
    }

    const int code = marker->pos - USUL_MARKER_BASE;
    if (code != USUL_CUSTOM_CODE) {
        for (const Usul& usul : usuller()) {
            if (usul.code == code && Fraction(usul.n, usul.d).identical(sig)) {
                return usul;
            }
        }
        return std::nullopt;
    }

    Usul usul;
    usul.code = USUL_CUSTOM_CODE;
    usul.n = sig.numerator();
    usul.d = sig.denominator();
    int total = 0;
    for (const GroupNode& node : groups.nodes()) {
        if (node.pos >= USUL_DARB_BASE && node.pos < USUL_DARB_BASE + USUL_MAX_ITEMS) {
            const Darb darb { (node.action & 1) != 0, node.action / 2 };
            if (darb.len <= 0) {
                return std::nullopt;
            }
            usul.darb.push_back(darb);
            total += darb.len;
        } else if (node.pos >= USUL_NAME_BASE && node.pos < USUL_NAME_BASE + USUL_MAX_ITEMS) {
            usul.name.push_back(char(node.action & 0xff));
        }
    }
    if (usul.darb.empty() || total != usul.n) {
        return std::nullopt;
    }
    return usul;
}

// The tempo marking at the start of a new score: ♩ = 80, or ♪ = 90 once its first usul counts in
// eighths. Dropping an usul on the first measure swaps one default marking for the other; a
// marking the user changed stays.
struct DefaultTempo {
    const char16_t* xml = u"";
    double bps = 0.0;   // quarter notes per second
};

inline const DefaultTempo& defaultTempo(int usulDenominator)
{
    static const DefaultTempo quarter { u"<sym>metNoteQuarterUp</sym> = 80", 80.0 / 60.0 };
    static const DefaultTempo eighth { u"<sym>metNote8thUp</sym> = 90", 90.0 / 120.0 };
    return usulDenominator == 8 ? eighth : quarter;
}

//---------------------------------------------------------
//   Donanıms (key signatures) of Arel-Ezgi-Uzdilek
//---------------------------------------------------------

// One accidental of a donanım: note letter and koma shift (−1 koma bemol, −4 bakiye bemol,
// −5 küçük mücennep bemol (♭), −8 büyük mücennep bemol, 1 koma diyez, 4 bakiye diyez (♯),
// 5 küçük mücennep diyez, 8 büyük mücennep diyez)
struct DonanimAcc {
    char letter = 'C';
    int koma = 0;
};

struct Donanim {
    std::vector<DonanimAcc> acc;    // in the order they are written
    const char* makamlar = "";      // makams written with this donanım
    const char* text = "";          // the accidentals in words
};

inline const std::vector<Donanim>& donanimlar()
{
    static const std::vector<Donanim> list {
        { {}, "Buselik, Çargah", "donanımsız" },
        { { { 'B', -5 } }, "Kürdi, Muhayyerkürdi, Acemkürdi, Acemaşiran", "Si bemol" },
        { { { 'B', -5 }, { 'E', -5 } }, "Nihavend", "Si bemol, Mi bemol" },
        { { { 'B', -5 }, { 'E', -5 }, { 'A', -5 } }, "Kürdilihicazkâr", "Si bemol, Mi bemol, La bemol" },
        { { { 'B', -4 }, { 'C', 4 } }, "Hicaz, Hicaz Uzzal, Sultaniyegâh", "Si bakiye bemol, Do diyez" },
        { { { 'B', -4 }, { 'C', 4 }, { 'F', 4 } }, "Hicaz", "Si bakiye bemol, Do diyez, Fa diyez" },
        { { { 'B', -4 }, { 'C', 4 }, { 'G', 4 } }, "Zirgüleli Hicaz", "Si bakiye bemol, Do diyez, Sol diyez" },
        { { { 'B', -1 } }, "Uşşak, Bayati", "Si koma bemol" },
        { { { 'B', -1 }, { 'D', -4 } }, "Saba", "Si koma bemol, Re bakiye bemol" },
        { { { 'B', -1 }, { 'F', 4 } }, "Rast, Hüseyni, Muhayyer, Eviç", "Si koma bemol, Fa diyez" },
        { { { 'B', -1 }, { 'E', -1 }, { 'F', 4 } }, "Segah", "Si koma bemol, Mi koma bemol, Fa diyez" },
        { { { 'B', -1 }, { 'E', -4 }, { 'F', 4 } }, "Hüzzam, Karcığar, Suzinak", "Si koma bemol, Mi bakiye bemol, Fa diyez" },
        { { { 'B', -1 }, { 'E', -4 }, { 'A', -4 }, { 'F', 4 } }, "Hicazkâr",
          "Si koma bemol, Mi bakiye bemol, La bakiye bemol, Fa diyez" },
        { { { 'B', -4 }, { 'E', -4 }, { 'F', 4 }, { 'C', 4 } }, "Şedaraban", "Si bakiye bemol, Mi bakiye bemol, Fa diyez, Do diyez" },
        { { { 'F', 5 } }, "Mahur", "Fa küçük mücennep diyez" },
        { { { 'F', 1 }, { 'G', 4 }, { 'D', 4 } }, "Suzidil", "Fa koma diyez, Sol diyez, Re diyez" },
        { { { 'B', -1 }, { 'F', 4 }, { 'C', 4 }, { 'E', 4 }, { 'A', 4 } }, "Evcârâ",
          "Si koma bemol, Fa diyez, Do diyez, Mi diyez, La diyez" },
    };
    return list;
}

// Glyph of a koma shift (the user's key signature glyphs, makamlar.mscz)
inline SymId komaSym(int koma)
{
    switch (koma) {
    case -1: return SymId::accidentalQuarterToneFlatStein;   // koma bemol (ters bemol)
    case -4: return SymId::accidentalBakiyeFlat;
    case -5: return SymId::accidentalFlat;                   // küçük mücennep bemol
    case -8: return SymId::accidentalBuyukMucennebFlat;
    case 1:  return SymId::accidentalQuarterToneSharpStein;  // koma diyez
    case 4:  return SymId::accidentalSharp;                  // bakiye diyez
    case 5:  return SymId::accidentalKucukMucennebSharp;
    case 8:  return SymId::accidentalBuyukMucennebSharp;
    default: break;
    }
    return SymId::noSym;
}

// The key signature event of a donanım: a custom key signature in C, as the Makam plugin writes it
inline KeySigEvent donanimKeySigEvent(const Donanim& donanim)
{
    KeySigEvent ke;
    ke.setConcertKey(Key::C);
    if (donanim.acc.empty()) {
        return ke;
    }
    ke.setCustom(true);
    for (const DonanimAcc& a : donanim.acc) {
        CustDef cd;
        switch (a.letter) {
        case 'C': cd.degree = 0;
            break;
        case 'D': cd.degree = 1;
            break;
        case 'E': cd.degree = 2;
            break;
        case 'F': cd.degree = 3;
            break;
        case 'G': cd.degree = 4;
            break;
        case 'A': cd.degree = 5;
            break;
        case 'B': cd.degree = 6;
            break;
        default: break;
        }
        cd.sym = komaSym(a.koma);
        ke.customKeyDefs().push_back(cd);
    }
    return ke;
}
}
