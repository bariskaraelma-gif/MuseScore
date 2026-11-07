/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the dizi score ("Diziyi dinle" of the Makam plugin, Makam.qml v0.23.2). A small score
 * with the dizi of a makam, up from the karar and back down, and optionally a sample seyir:
 * the most typical opening phrase in the SymbTr collection (MTG, Universitat Pompeu Fabra;
 * CC BY-NC-SA 4.0) plus the most frequent cadence onto the karar.
 *
 * The score is written in Bolahenk (no transposition) and untuned. Its title names the makam,
 * so the tuning engine (makamtuner.h) tunes it like any other score; the ahenk is set with
 * setAhenk (editahenk.h).
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

#include <string>

#include <QByteArray>
#include <QString>

namespace mu::engraving::makam {
// Meta tag of a dizi score: its makam id, plus " seyir" when it has the sample seyir
static constexpr const char16_t* DIZI_META_TAG = u"makamDizi";

// "Rast dizisi"; empty for an unknown makam
QString diziTitle(const std::string& makamId);

// Value of the DIZI_META_TAG of the dizi score; empty for an unknown makam
QString diziTag(const std::string& makamId, bool withSeyir);

// Whether a sample seyir is known for the makam
bool hasSeyir(const std::string& makamId);

// The dizi score as .mscx text; empty for an unknown makam
QByteArray diziMscx(const std::string& makamId, bool withSeyir);
}
