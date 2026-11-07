/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the dizi score, see makamdizi.h. A port of "Diziyi dinle" of the Makam plugin
 * (Makam.qml v0.23.2, Copyright (C) 2026 Barış Karaelma, GPL-3.0).
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
#include "makamdizi.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

#include <QRegularExpression>
#include <QStringList>

#include "makamtuner.h"

#include "../dom/makamdata.h"
#include "../types/symnames.h"

namespace mu::engraving::makam {
namespace {
// One step of a dizi: written letter, equal-tempered cents above the karar, the calibrated perde
// it is (empty: plain ♯/♭/natural), link: the yeden La♯ below or inside the dizi
struct Step {
    char letter = 'C';
    int cents = 0;
    const char* perde = "";
    bool link = false;
};

struct Dizi {
    const char* id = "";
    std::vector<Step> steps;
};

// Sample seyir in SymbTr spelling: "Si4b1:3" = Si 1 koma bemol in octave 4, 3 sixteenths long
struct Seyir {
    const char* id = "";
    const char* source = "";   // UTF-8
    const char* notes = "";
};

const std::vector<Dizi>& diziler()
{
    static const std::vector<Dizi> list {
        { "ussak", { { 'A', 0, "", false }, { 'B', 200, "B-1", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "rast", { { 'G', 0, "", false }, { 'A', 200, "", false }, { 'B', 400, "B-1", false }, { 'C', 500, "", false }, { 'D', 700, "", false }, { 'E', 900, "", false }, { 'F', 1100, "F4", false }, { 'G', 1200, "", false } } },
        { "huzzam", { { 'A', -100, "", true }, { 'B', 0, "B-1", false }, { 'C', 100, "", false }, { 'D', 300, "", false }, { 'E', 500, "E-4", false }, { 'F', 700, "F4", false }, { 'G', 800, "", false }, { 'A', 1000, "", false }, { 'B', 1200, "B-1", false } } },
        { "hicaz", { { 'A', 0, "", false }, { 'B', 200, "B-4", false }, { 'C', 400, "C4", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 900, "F4", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "huseyni", { { 'A', 0, "", false }, { 'B', 200, "B-1", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 900, "F4", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "segah", { { 'B', 0, "B-1", false }, { 'C', 100, "", false }, { 'D', 300, "", false }, { 'E', 500, "E-1", false }, { 'F', 700, "F4", false }, { 'G', 800, "", false }, { 'A', 1000, "", false }, { 'B', 1200, "B-1", false } } },
        { "saba", { { 'A', 0, "", false }, { 'B', 200, "B-1", false }, { 'C', 300, "", false }, { 'D', 500, "D-4", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "nihavend", { { 'G', 0, "", false }, { 'A', 200, "", false }, { 'B', 300, "", false }, { 'C', 500, "", false }, { 'D', 700, "", false }, { 'E', 800, "", false }, { 'F', 1000, "", false }, { 'G', 1200, "", false } } },
        { "kurdi", { { 'A', 0, "", false }, { 'B', 100, "", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "muhayyerkurdi", { { 'A', 0, "", false }, { 'B', 100, "", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "muhayyer", { { 'A', 0, "", false }, { 'B', 200, "B-1", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 900, "F4", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "acemkurdi", { { 'A', 0, "", false }, { 'B', 100, "", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "bayati", { { 'A', 0, "", false }, { 'B', 200, "B-1", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "evic", { { 'F', 0, "F4", false }, { 'G', 100, "", false }, { 'A', 300, "", false }, { 'B', 500, "B-1", false }, { 'C', 600, "", false }, { 'D', 800, "", false }, { 'E', 1000, "", false }, { 'F', 1200, "F4", false } } },
        { "kurdilihicazkar", { { 'G', 0, "", false }, { 'A', 100, "", false }, { 'B', 300, "", false }, { 'C', 500, "", false }, { 'D', 700, "", false }, { 'E', 800, "", false }, { 'F', 1000, "", false }, { 'G', 1200, "", false } } },
        { "hicazuzzal", { { 'A', 0, "", false }, { 'B', 200, "B-4", false }, { 'C', 400, "C4", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 900, "F4", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "zirgulelihicaz", { { 'A', 0, "", false }, { 'B', 200, "B-4", false }, { 'C', 400, "C4", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1100, "G4", false }, { 'A', 1200, "", false } } },
        { "sedaraban", { { 'D', 0, "", false }, { 'E', 200, "E-4", false }, { 'F', 400, "F4", false }, { 'G', 500, "", false }, { 'A', 700, "", false }, { 'B', 900, "B-4", false }, { 'C', 1100, "C4", false }, { 'D', 1200, "", false } } },
        { "buselik", { { 'A', 0, "", false }, { 'B', 200, "", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "", false }, { 'F', 800, "", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "cargah", { { 'C', 0, "", false }, { 'D', 200, "", false }, { 'E', 400, "", false }, { 'F', 500, "", false }, { 'G', 700, "", false }, { 'A', 900, "", false }, { 'B', 1100, "", false }, { 'C', 1200, "", false } } },
        { "mahur", { { 'G', 0, "", false }, { 'A', 200, "", false }, { 'B', 400, "", false }, { 'C', 500, "", false }, { 'D', 700, "", false }, { 'E', 900, "", false }, { 'F', 1000, "F5", false }, { 'G', 1200, "", false } } },
        { "acemasiran", { { 'F', 0, "", false }, { 'G', 200, "", false }, { 'A', 400, "", false }, { 'B', 500, "", false }, { 'C', 700, "", false }, { 'D', 900, "", false }, { 'E', 1100, "", false }, { 'F', 1200, "", false } } },
        { "evcara", { { 'F', 0, "F4", false }, { 'G', 100, "", false }, { 'A', 400, "", true }, { 'B', 500, "B-1", false }, { 'C', 700, "C4", false }, { 'D', 800, "", false }, { 'E', 1100, "E4", false }, { 'F', 1200, "F4", false } } },
        { "suzidil", { { 'E', 0, "", false }, { 'F', 100, "F1", false }, { 'G', 400, "G4", false }, { 'A', 500, "", false }, { 'B', 700, "", false }, { 'C', 800, "", false }, { 'D', 1100, "D4", false }, { 'E', 1200, "", false } } },
        { "karcigar", { { 'A', 0, "", false }, { 'B', 200, "B-1", false }, { 'C', 300, "", false }, { 'D', 500, "", false }, { 'E', 700, "E-4", false }, { 'F', 900, "F4", false }, { 'G', 1000, "", false }, { 'A', 1200, "", false } } },
        { "suzinak", { { 'G', 0, "", false }, { 'A', 200, "", false }, { 'B', 400, "B-1", false }, { 'C', 500, "", false }, { 'D', 700, "", false }, { 'E', 900, "E-4", false }, { 'F', 1100, "F4", false }, { 'G', 1200, "", false } } },
        { "hicazkar", { { 'G', 0, "", false }, { 'A', 200, "A-4", false }, { 'B', 400, "B-1", false }, { 'C', 500, "", false }, { 'D', 700, "", false }, { 'E', 900, "E-4", false }, { 'F', 1100, "F4", false }, { 'G', 1200, "", false } } },
    };
    return list;
}

const std::vector<Seyir>& seyirler()
{
    static const std::vector<Seyir> list {
        { "rast", "seyir, Erol Bingöl",
          "Sol4:4 Sol4:2 La4:2 Si4b1:3 La4:1 Re5:1 Do5:1 Si4b1:1 La4:1 Sol4:8" },
        { "ussak", "örnek, Ruhi Ayangil",
          "Sol4:2 La4:2 Si4b2:2 Do5:2 Si4b2:4 Do5:2 Si4b2:2 La4:8" },
        { "huzzam", "Dil-hûn Olurum, şarkı, Bimen Şen",
          "Do5:8 Re5:4 Re5:8 Mi5b3:4 Fa5#3:8 Mi5b3:2 Re5:2 Re5:8 Do5:2 Si4b1:2 La4#4:2 Si4b1:8" },
        { "hicaz", "Aşkı Seninle, şarkı, Fehmi Tokay",
          "La4:2 Re5:4 Mi5:2 Fa5:4 Mi5:4 Re5:4 Do5#4:4 Re5:8 Re5:2 Do5#4:2 Si4b4:2 La4:8" },
        { "huseyni", "saz semaisi, Neyzen Râşid Efendi",
          "La4:4 Mi5:1 Fa5:1 Mi5:1 Re5:2 Mi5:1 Mi5:1 Re5:1 Do5:2 Re5:2 Mi5:2 Re5:2 Do5:2 Si4b2:2 La4:8" },
        { "segah", "Hasta Kalbimde, şarkı, Kaptanzâde Ali Rıza Bey",
          "Si4b1:2 Do5:4 Re5:4 Re5:2 Do5:2 Si4b1:2 Re5:16 Do5:2 Si4b1:2 La4#4:2 Si4b1:8" },
        { "saba", "peşrev, Hasan Esen",
          "Si4b2:4 Do5:2 Do5:2 Do5:8 Do5:2 Re5b3:2 Do5:2 Si4b2:2 Do5:4 Do5:2 Si4b2:2 La4:2 La4:8" },
        { "nihavend", "Sûziş-i Aşkınla, şarkı, Musa Süreyya Bey",
          "Sol4:3 La4:1 Si4b5:2 Do5:2 Re5:2 Mi5b5:2 Re5:2 Do5:2 Si4b5:2 Re5:12 La4:2 Sol4:2 Fa4#4:2 Sol4:8" },
        { "kurdi", "örnek, Hüseyin Sadettin Arel",
          "La4:6 Si4b5:2 Do5:4 Re5:4 Mi5:2 Fa5:2 Sol5:2 La5:2 Sol5:4 Fa5:2 Mi5:2 Re5:6 Mi5:2 Do5:4 Re5:2 Do5:2 Mi5:2 Do5:2 Si4b5:2 La4:8" },
        { "muhayyerkurdi", "Yalnız Kalan, şarkı, Teoman Alpay",
          "La5:4 Si5b5:4 La5:4 Sol5:4 Fa5:4 Sol5:4 La5:8 Sol5:2 Fa5:2 Mi5:2 Re5:2 Mi5:4 Re5:2 Do5:2 Si4b5:2 La4:8" },
        { "muhayyer", "seyir, Rauf Yekta",
          "Sol5:8 La5:4 La5:8 Si5b2:4 La5:8 Sol5:4 Fa5#3:4 Mi5:8 Re5:4 Do5:2 Si4b2:2 La4:8" },
        { "acemkurdi", "seyir, Erol Bingöl",
          "Fa5:4 Fa5:4 Mi5:4 Sol5:2 Fa5:2 Fa5:8 Mi5:2 Re5:2 Do5:4 Si4b5:4 La4:8" },
        { "bayati", "seyir, Erol Bingöl",
          "Do5:4 Re5:4 Re5:2 Mi5:2 Re5:2 Do5:2 Re5:3 Do5:1 Si4b2:2 Do5:2 Re5:8 Do5:2 Si4b2:2 La4:8" },
        { "evic", "seyir, Rauf Yekta",
          "Fa5#4:8 Fa5#4:4 Fa5#4:8 Sol5:4 Fa5#4:8 Mi5:4 Re5:8 Do5:4 Si4b1:2 La4:2 Sol4:2 Fa4#4:8" },
        { "kurdilihicazkar", "Gittin Bıraktın, şarkı, Emin Ongan",
          "Sol5:6 Sol5:2 Sol5:1 Fa5:1 Mi5b5:1 Sol5:4 Do5:2 Si4b5:2 La4b5:2 Sol4:8" },
        { "karcigar", "seyir, Rauf Yekta",
          "Do5:8 Re5:4 Re5:8 Mi5b3:4 Re5:8 Do5:4 Si4b2:4 La4:8" },
        { "suzinak", "seyir, Erol Bingöl",
          "Do5:4 Re5:4 Re5:2 Mi5b4:1 Fa5#4:1 Mi5b4:2 Re5:2 Re5:3 Do5:1 Si4b1:2 Do5:2 Re5:8 Do5:2 Si4b1:2 La4b4:2 Sol4:8" },
        { "hicazkar", "Birer Birer, şarkı, Ömer Sami Gupgup",
          "Fa5#4:2 Sol5:4 La5b4:2 Sol5:4 Fa5#4:2 Sol5:2 Mi5b4:8 Do5:2 Si4b1:2 La4b4:2 Sol4:8" },
        { "hicazuzzal", "seyir, Erol Bingöl",
          "Re5:4 Re5:4 Do5#4:4 Re5:4 Mi5:8 Mi5:6 Re5:2 Sol5:2 Fa5#3:1 Mi5:1 Mi5:8 Re5:2 Do5#4:2 Si4b4:2 La4:8" },
        { "zirgulelihicaz", "seyir, Erol Bingöl",
          "La4:6 Si4b4:2 Do5#4:4 Re5:4 Mi5:3 Re5:1 Do5#4:2 Re5:2 Mi5:8 Re5:2 Do5#4:2 Si4b4:2 La4:2 Sol4#4:2 Si4b4:2 La4:8" },
        { "sedaraban", "Dinlese Şedd-i Arabân, kâr, Ahmet Avni Konuk",
          "Re5:4 Re5:2 Fa5#4:2 Mi5b4:2 Re5:2 Fa5#4:1 Mi5b4:1 Mi5b4:1 Re5:1 Do5#4:2 Re5:2 Do5#4:2 Si4b4:2 La4:4 Sol4:2 Fa4#4:2 Mi4b4:2 Re4:8" },
        { "buselik", "seyir, Rauf Yekta",
          "Do5:8 Si4:4 Do5:8 Re5:4 Mi5:8 Re5:4 Do5:4 Si4:2 La4:2 Sol4#4:2 La4:8" },
        { "cargah", "Bilmem Şu, türkü (Yeni Çargah)",
          "Do5:2 Do5:1 Re5:1 Mi5:1 Re5:1 Mi5:2 Re5:1 Mi5:1 Re5:2 Mi5:2 Do5:2 Re5:2 Si4:2 Do5:8" },
        { "mahur", "seyir, Şefik Gürmeriç",
          "Sol5:6 Fa5#5:2 Mi5:4 Fa5#5:4 Sol5:8 La5:1 Sol5:1 Fa5#5:1 Mi5:1 Re5:4 Do5:2 Si4:2 La4:2 Sol4:2 Fa4#5:2 Sol4:8" },
        { "acemasiran", "Gördüm Seni, şarkı, İrfan Doğrusöz",
          "Fa5:3 Mi5:1 Fa5:2 La5:2 Sol5:2 Fa5:2 Re5:2 Fa5:2 Mi5:12 Re5:2 Do5:2 Si4b5:2 La4:2 Sol4:2 Fa4:8" },
        { "evcara", "seyir, Rauf Yekta",
          "Fa5#4:8 Fa5#4:4 Fa5#4:8 Mi5#4:4 Re5:8 Mi5#4:4 Fa5#4:8 Mi5#4:2 Re5:2 Do5#4:2 Si4b1:2 La4#4:2 Sol4:2 Fa4#4:8" },
        { "suzidil", "seyir, Erol Bingöl",
          "Re5#4:4 Mi5:4 Mi5:4 Sol5#4:2 Fa5#1:2 Mi5:2 Mi5:3 Re5#4:1 Do5:2 Re5#4:2 Mi5:8 Re5:2 Do5:2 Si4:2 La4:2 Sol4#4:2 Fa4#1:2 Mi4:8" },
    };
    return list;
}

const Dizi* diziOf(const std::string& id)
{
    for (const Dizi& d : diziler()) {
        if (id == d.id) {
            return &d;
        }
    }
    return nullptr;
}

const Seyir* seyirOf(const std::string& id)
{
    for (const Seyir& s : seyirler()) {
        if (id == s.id) {
            return &s;
        }
    }
    return nullptr;
}

int letterPc(char letter)
{
    switch (letter) {
    case 'C': return 0;
    case 'D': return 2;
    case 'E': return 4;
    case 'F': return 5;
    case 'G': return 7;
    case 'A': return 9;
    case 'B': return 11;
    default: break;
    }
    return 0;
}

int letterFifth(char letter)
{
    switch (letter) {
    case 'F': return -1;
    case 'C': return 0;
    case 'G': return 1;
    case 'D': return 2;
    case 'A': return 3;
    case 'E': return 4;
    case 'B': return 5;
    default: break;
    }
    return 0;
}

// Written MIDI pitch of the karar's natural note in the dizi score (Dügah = La4)
int kararBase(char letter)
{
    switch (letter) {
    case 'A': return 69;
    case 'G': return 67;
    case 'B': return 71;
    case 'F': return 65;
    case 'C': return 72;
    case 'D': return 62;
    case 'E': return 64;
    default: break;
    }
    return 69;
}

int komaOf(const std::string& perde)
{
    return perde.size() > 1 ? std::atoi(perde.c_str() + 1) : 0;
}

// The koma shifts that have an accidental; another value is moved to the nearest of them
int snapKoma(int koma)
{
    static const int KOMAS[] = { -8, -5, -4, -1, 0, 1, 4, 5, 8 };
    int best = 0;
    for (int k : KOMAS) {
        if (std::abs(k - koma) < std::abs(best - koma)) {
            best = k;
        }
    }
    return best;
}

// A written note: letter, natural pitch, the ♯/♭ in the pitch, the koma shift it is read with
struct DiziNote {
    char letter = 'C';
    int nat = 60;
    int alter = 0;
    int koma = 0;
    int len = 2;   // sixteenths

    int pitch() const { return nat + alter; }
    int tpc() const { return 14 + letterFifth(letter) + 7 * alter; }
};

DiziNote stepNote(const MakamBasics& makam, const Step& st, int len)
{
    const int karar = kararBase(makam.karar.empty() ? 'A' : makam.karar[0]) + perdeAlter(makam.karar);
    const int pitch = karar + static_cast<int>(std::lround(st.cents / 100.0));
    const int pc = letterPc(st.letter);
    DiziNote n;
    n.letter = st.letter;
    n.nat = static_cast<int>(std::lround((pitch - pc) / 12.0)) * 12 + pc;
    n.alter = pitch - n.nat;
    n.koma = *st.perde ? komaOf(st.perde) : st.link ? 4 : n.alter > 0 ? 4 : n.alter < 0 ? -5 : 0;
    n.len = len;
    return n;
}

// A note of the seyir. A microtonal note takes the perde of the dizi when the dizi has that letter
// with the same kind of accidental (so Uşşak's Si4b2 is its Segah, "B-1"); otherwise its own
// koma shift.
bool seyirNote(const MakamBasics& makam, const Dizi& dizi, const QString& token, DiziNote& result)
{
    static const QRegularExpression re("^(Do|Re|Mi|Fa|Sol|La|Si)(\\d)(?:([b#])(\\d))?:(\\d+)$");
    const QRegularExpressionMatch m = re.match(token);
    if (!m.hasMatch()) {
        return false;
    }
    static const std::map<QString, char> LETTERS {
        { "Do", 'C' }, { "Re", 'D' }, { "Mi", 'E' }, { "Fa", 'F' }, { "Sol", 'G' }, { "La", 'A' }, { "Si", 'B' },
    };
    const char letter = LETTERS.at(m.captured(1));
    int koma = m.captured(3).isEmpty() ? 0 : (m.captured(3) == "b" ? -1 : 1) * m.captured(4).toInt();
    if (koma != 0) {
        for (const Step& st : dizi.steps) {
            if (st.letter != letter) {
                continue;
            }
            const int dk = stepNote(makam, st, 2).koma;
            if (dk != 0 && (dk < 0) == (koma < 0)) {
                koma = dk;
                break;
            }
        }
    }
    koma = snapKoma(koma);

    result.letter = letter;
    result.nat = (m.captured(2).toInt() + 1) * 12 + letterPc(letter);
    result.alter = koma == 4 ? 1 : koma == -5 ? -1 : 0;
    result.koma = koma;
    result.len = m.captured(5).toInt();
    return true;
}

struct DiziPart {
    QString label;
    std::vector<DiziNote> notes;
};

// The dizi up from the karar and down again in eighths, the last note a half (durak); then the seyir
std::vector<DiziPart> diziParts(const MakamBasics& makam, const Dizi& dizi, bool withSeyir)
{
    std::vector<DiziPart> parts;
    const size_t count = dizi.steps.size();
    size_t first = 0;
    while (first + 1 < count && dizi.steps[first].cents < 0) {
        ++first;                                   // Hüzzam's yeden La♯ belongs to the seyir
    }
    DiziPart up { QString::fromUtf8("Dizi: çıkış / iniş"), {} };
    for (size_t i = first; i < count; ++i) {
        up.notes.push_back(stepNote(makam, dizi.steps[i], 2));
    }
    for (size_t j = count - 1; j-- > first;) {
        up.notes.push_back(stepNote(makam, dizi.steps[j], 2));
    }
    if (!up.notes.empty()) {
        up.notes.back().len = 8;
        parts.push_back(up);
    }

    const Seyir* seyir = withSeyir ? seyirOf(makam.id) : nullptr;
    if (seyir) {
        DiziPart ex { QString::fromUtf8("Örnek seyir (SymbTr: %1)").arg(QString::fromUtf8(seyir->source)), {} };
        for (const QString& token : QString::fromLatin1(seyir->notes).split(' ', Qt::SkipEmptyParts)) {
            DiziNote n;
            if (seyirNote(makam, dizi, token, n)) {
                ex.notes.push_back(n);
            }
        }
        if (!ex.notes.empty()) {
            parts.push_back(ex);
        }
    }
    return parts;
}

QString xmlEscape(const QString& s)
{
    return s.toHtmlEscaped();
}

// mscx duration of a length in sixteenths
bool durationOf(int len, const char*& type, int& dots)
{
    switch (len) {
    case 1: type = "16th";
        dots = 0;
        return true;
    case 2: type = "eighth";
        dots = 0;
        return true;
    case 3: type = "eighth";
        dots = 1;
        return true;
    case 4: type = "quarter";
        dots = 0;
        return true;
    case 6: type = "quarter";
        dots = 1;
        return true;
    case 8: type = "half";
        dots = 0;
        return true;
    case 12: type = "half";
        dots = 1;
        return true;
    case 16: type = "whole";
        dots = 0;
        return true;
    default: break;
    }
    return false;
}

QString restsXml(int sixteenths)
{
    static const std::pair<int, const char*> SIZES[] = { { 8, "half" }, { 4, "quarter" }, { 2, "eighth" }, { 1, "16th" } };
    QString x;
    for (const auto& [size, type] : SIZES) {
        while (sixteenths >= size) {
            x += QString("          <Rest>\n            <durationType>%1</durationType>\n            </Rest>\n").arg(QString::fromLatin1(type));
            sixteenths -= size;
        }
    }
    return x;
}

QString accidentalName(int koma)
{
    const SymId sym = koma == 0 ? SymId::accidentalNatural : komaSym(koma);
    return QString::fromLatin1(SymNames::nameForSymId(sym).ascii());
}

// The donanım in written order: the palette's when one has the same accidentals
std::vector<DonanimAcc> donanimOf(const MakamBasics& makam)
{
    std::vector<DonanimAcc> wanted;
    for (const std::string& perde : makam.donanim) {
        if (!perde.empty()) {
            wanted.push_back({ perde[0], komaOf(perde) });
        }
    }
    for (const Donanim& d : donanimlar()) {
        if (d.acc.size() != wanted.size()) {
            continue;
        }
        bool same = true;
        for (const DonanimAcc& a : d.acc) {
            bool found = false;
            for (const DonanimAcc& w : wanted) {
                found = found || (w.letter == a.letter && w.koma == a.koma);
            }
            same = same && found;
        }
        if (same) {
            return d.acc;
        }
    }
    return wanted;
}

QString keySigXml(const std::vector<DonanimAcc>& donanim)
{
    static const std::string LETTERS = "CDEFGAB";
    QString x = "          <KeySig>\n            <concertKey>0</concertKey>\n";
    if (!donanim.empty()) {
        x += "            <custom>1</custom>\n";
        for (const DonanimAcc& a : donanim) {
            const SymId sym = komaSym(a.koma);
            if (sym == SymId::noSym) {
                continue;
            }
            x += QString("            <CustDef>\n              <sym>%1</sym>\n"
                         "              <def degree=\"%2\" xAlt=\"0\" octAlt=\"0\"/>\n              </CustDef>\n")
                 .arg(QString::fromLatin1(SymNames::nameForSymId(sym).ascii()))
                 .arg(LETTERS.find(a.letter));
        }
    }
    return x + "            </KeySig>\n";
}

struct DiziMeasure {
    QString xml;
    QString len;        // irregular length, e.g. "7/8"; empty: 4/4
    bool lineBreak = false;
};

// 4/4 measures. A note that does not fit ends the measure early (an irregular measure, so there
// are no rests inside a phrase); the last measure of a part is filled with rests.
std::vector<DiziMeasure> diziMeasures(const std::vector<DiziPart>& parts, const std::vector<DonanimAcc>& donanim)
{
    std::map<char, int> keyKoma;
    for (const DonanimAcc& a : donanim) {
        keyKoma[a.letter] = a.koma;
    }

    std::vector<DiziMeasure> measures;
    QString cur;
    int used = 0;
    std::map<std::pair<char, int>, int> shown;   // accidentals written in the measure

    auto flush = [&](bool early) {
        DiziMeasure m;
        if (early) {
            const int g = std::gcd(used, 16);
            m.len = QString("%1/%2").arg(used / g).arg(16 / g);
        } else {
            cur += restsXml(16 - used);
        }
        m.xml = cur;
        measures.push_back(m);
        cur.clear();
        used = 0;
        shown.clear();
    };

    for (size_t p = 0; p < parts.size(); ++p) {
        if (used > 0) {
            flush(false);
        }
        if (p > 0 && !measures.empty()) {
            measures.back().lineBreak = true;          // each part starts on a new line
        }
        cur += QString("          <StaffText>\n            <text>%1</text>\n            </StaffText>\n")
               .arg(xmlEscape(parts[p].label));
        for (const DiziNote& n : parts[p].notes) {
            const char* type = "";
            int dots = 0;
            if (!durationOf(n.len, type, dots)) {
                continue;
            }
            if (used + n.len > 16) {
                flush(used < 16);
            }
            const std::pair<char, int> key { n.letter, n.nat };
            const int expected = shown.count(key) ? shown[key] : (keyKoma.count(n.letter) ? keyKoma[n.letter] : 0);
            QString acc;
            if (n.koma != expected) {
                acc = QString("              <Accidental>\n                <subtype>%1</subtype>\n                </Accidental>\n")
                      .arg(accidentalName(n.koma));
                shown[key] = n.koma;
            }
            cur += "          <Chord>\n";
            if (dots) {
                cur += QString("            <dots>%1</dots>\n").arg(dots);
            }
            cur += QString("            <durationType>%1</durationType>\n").arg(QString::fromLatin1(type));
            cur += "            <Note>\n" + acc;
            cur += QString("              <pitch>%1</pitch>\n              <tpc>%2</tpc>\n").arg(n.pitch()).arg(n.tpc());
            cur += "              </Note>\n            </Chord>\n";
            used += n.len;
        }
    }
    if (used > 0) {
        flush(false);
    }
    return measures;
}
}

QString diziTitle(const std::string& makamId)
{
    MakamBasics makam;
    if (!makamBasics(makamId, makam)) {
        return QString();
    }
    return makam.name + " dizisi";
}

QString diziTag(const std::string& makamId, bool withSeyir)
{
    if (!diziOf(makamId)) {
        return QString();
    }
    return QString::fromStdString(makamId) + (withSeyir && seyirOf(makamId) ? " seyir" : "");
}

bool hasSeyir(const std::string& makamId)
{
    return seyirOf(makamId) != nullptr;
}

QByteArray diziMscx(const std::string& makamId, bool withSeyir)
{
    MakamBasics makam;
    const Dizi* dizi = diziOf(makamId);
    if (!dizi || !makamBasics(makamId, makam)) {
        return QByteArray();
    }
    const std::vector<DonanimAcc> donanim = donanimOf(makam);
    const std::vector<DiziMeasure> measures = diziMeasures(diziParts(makam, *dizi, withSeyir), donanim);
    if (measures.empty()) {
        return QByteArray();
    }

    const QString title = diziTitle(makamId);
    const QString subtitle = withSeyir && seyirOf(makamId) ? QString::fromUtf8("Dizi ve örnek seyir") : QString::fromUtf8("Dizi");
    const DefaultTempo& tempo = defaultTempo(4);

    QString x;
    x += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<museScore version=\"4.70\">\n"
         "  <Score>\n"
         "    <Division>480</Division>\n";
    x += QString("    <metaTag name=\"workTitle\">%1</metaTag>\n").arg(xmlEscape(title));
    x += QString("    <metaTag name=\"%1\">%2</metaTag>\n").arg(QString::fromUtf16(DIZI_META_TAG), diziTag(makamId, withSeyir));
    // the "Türk Müziği" instrument of instruments.xml (turk-muzigi)
    x += QString::fromUtf8(
        "    <Part>\n"
        "      <Staff id=\"1\">\n"
        "        <StaffType group=\"pitched\">\n"
        "          <name>stdNormal</name>\n"
        "          </StaffType>\n"
        "        </Staff>\n"
        "      <trackName>Türk Müziği</trackName>\n"
        "      <Instrument id=\"turk-muzigi\">\n"
        "        <longName>Türk Müziği</longName>\n"
        "        <shortName>T.M.</shortName>\n"
        "        <trackName>Türk Müziği</trackName>\n"
        "        <minPitchP>40</minPitchP>\n"
        "        <maxPitchP>93</maxPitchP>\n"
        "        <minPitchA>40</minPitchA>\n"
        "        <maxPitchA>93</maxPitchA>\n"
        "        <instrumentId>pluck.guitar.nylon-string</instrumentId>\n"
        "        <Channel name=\"open\">\n"
        "          <program value=\"24\"/>\n"
        "          <synti>Fluid</synti>\n"
        "          </Channel>\n"
        "        </Instrument>\n"
        "      </Part>\n");
    x += "    <Staff id=\"1\">\n"
         "      <VBox>\n"
         "        <height>10</height>\n"
         "        <Text>\n"
         "          <style>title</style>\n";
    x += QString("          <text>%1</text>\n").arg(xmlEscape(title));
    x += "          </Text>\n"
         "        <Text>\n"
         "          <style>subtitle</style>\n";
    x += QString("          <text>%1</text>\n").arg(xmlEscape(subtitle));
    x += "          </Text>\n"
         "        </VBox>\n";

    for (size_t m = 0; m < measures.size(); ++m) {
        const DiziMeasure& measure = measures[m];
        x += measure.len.isEmpty() ? QString("      <Measure>\n") : QString("      <Measure len=\"%1\">\n").arg(measure.len);
        if (measure.lineBreak) {
            x += "        <LayoutBreak>\n          <subtype>line</subtype>\n          </LayoutBreak>\n";
        }
        x += "        <voice>\n";
        if (m == 0) {
            x += keySigXml(donanim);
            x += "          <TimeSig>\n            <sigN>4</sigN>\n            <sigD>4</sigD>\n            </TimeSig>\n";
            x += QString("          <Tempo>\n            <tempo>%1</tempo>\n            <followText>1</followText>\n"
                         "            <text>%2</text>\n            </Tempo>\n")
                 .arg(tempo.bps, 0, 'f', 6).arg(QString::fromUtf16(tempo.xml));
        }
        x += measure.xml;
        x += "          </voice>\n        </Measure>\n";
    }
    x += "      </Staff>\n"
         "    </Score>\n"
         "  </museScore>\n";
    return x.toUtf8();
}
}
