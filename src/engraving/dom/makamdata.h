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
    const char* name = "";
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

// Beam groups of an usul's time signature, with the usul marker as the last node
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
    return Groups(nodes);
}

// The usul of a time signature, from its marker node; nullptr if it is not an usul
inline const Usul* usulOfGroups(const Groups& groups, const Fraction& sig)
{
    if (groups.empty()) {
        return nullptr;
    }
    const int code = groups.nodes().back().pos - USUL_MARKER_BASE;
    for (const Usul& usul : usuller()) {
        if (usul.code == code && Fraction(usul.n, usul.d).identical(sig)) {
            return &usul;
        }
    }
    return nullptr;
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
        { { { 'B', -4 }, { 'C', 4 } }, "Hicaz, Hicaz Uzzal", "Si bakiye bemol, Do diyez" },
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
