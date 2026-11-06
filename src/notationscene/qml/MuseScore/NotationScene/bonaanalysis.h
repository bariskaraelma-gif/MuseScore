/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: analysis of a Bona recording (note entry by voice).
 *
 *  - findSyllables(): where each sung syllable starts and ends. A syllable is a stretch of
 *    voiced sound (a vowel); two syllables are told apart by a pause or, when they are sung
 *    legato (la-la, mi-mi), by a dip in loudness between them (the consonant).
 *  - quantize(): the syllables put on the grid of the chosen note value (8th, 16th): a note
 *    lasts until the next one, unless there is a silence of at least one grid step (a rest).
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
#include <vector>

namespace mu::notation::bona {
struct Syllable {
    double onset = 0.0;   // seconds from the first sample: the voice (the vowel) starts
    double end = 0.0;     // the voice stops, or the next syllable starts
    float peakDb = 0.f;   // loudest frame
};

struct AnalysisInfo {
    float noiseDb = 0.f;      // level of the silence
    float peakDb = 0.f;       // level of the loudest syllables
    float thresholdDb = 0.f;  // a frame louder than this (and voiced) is sound
};

std::vector<Syllable> findSyllables(const std::vector<float>& samples, int sampleRate, AnalysisInfo* info = nullptr);

struct TimedSpan {
    double onset = 0.0;   // in ticks from the start of the written passage (may be negative)
    double end = 0.0;
};

struct QuantizedNote {
    int tick = 0;         // from the start of the written passage
    int duration = 0;     // in ticks
};

std::vector<QuantizedNote> quantize(const std::vector<TimedSpan>& spans, int gridTicks);

//! 16-bit mono WAV, for checking a recording afterwards
bool writeWav(const std::string& path, const std::vector<float>& samples, int sampleRate);
}
