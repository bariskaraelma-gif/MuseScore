/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: analysis of a Bona recording, see bonaanalysis.h
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
#include "bonaanalysis.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>

using namespace mu::notation::bona;

namespace {
constexpr double FRAME_HOP_SECS = 0.010;      // one frame every 10 ms
constexpr int FRAME_WINDOW_HOPS = 3;          // each frame looks at 30 ms
constexpr double MIN_F0 = 70.0;               // voice range for the voicing test
constexpr double MAX_F0 = 800.0;
constexpr float VOICED_CORRELATION = 0.5f;    // periodicity of a voiced frame
constexpr int BRIDGE_FRAMES = 2;              // a gap of up to 20 ms does not split a syllable
constexpr int MIN_SYLLABLE_FRAMES = 6;        // a syllable is at least 60 ms long
constexpr float DIP_DB = 6.f;                 // a legato consonant: loudness drops this much
constexpr int DIP_REACH_FRAMES = 15;          // ...within 150 ms on both sides
constexpr int MIN_DIP_DISTANCE_FRAMES = 8;    // two legato syllables are at least 80 ms apart

float percentile(std::vector<float> values, double p)
{
    if (values.empty()) {
        return 0.f;
    }
    const size_t k = std::min(values.size() - 1, static_cast<size_t>(p * (values.size() - 1)));
    std::nth_element(values.begin(), values.begin() + k, values.end());
    return values[k];
}

//! the highest normalized autocorrelation in the voice range, 0..1
float periodicity(const float* x, int n, int minLag, int maxLag)
{
    std::vector<double> prefix(n + 1, 0.0);   // running sum of squares
    for (int i = 0; i < n; ++i) {
        prefix[i + 1] = prefix[i] + double(x[i]) * x[i];
    }

    float best = 0.f;
    for (int lag = minLag; lag <= maxLag && lag < n - 1; ++lag) {
        double sum = 0.0;
        for (int i = 0; i + lag < n; ++i) {
            sum += double(x[i]) * x[i + lag];
        }
        const double e1 = prefix[n - lag];
        const double e2 = prefix[n] - prefix[lag];
        if (e1 <= 0.0 || e2 <= 0.0) {
            continue;
        }
        best = std::max(best, static_cast<float>(sum / std::sqrt(e1 * e2)));
    }
    return best;
}
}

std::vector<Syllable> mu::notation::bona::findSyllables(const std::vector<float>& samples, int sampleRate, AnalysisInfo* info)
{
    std::vector<Syllable> result;

    const int hop = static_cast<int>(sampleRate * FRAME_HOP_SECS);
    const int window = hop * FRAME_WINDOW_HOPS;
    if (hop <= 0 || static_cast<int>(samples.size()) < window * 2) {
        return result;
    }
    const int frameCount = (static_cast<int>(samples.size()) - window) / hop + 1;

    // Loudness of each frame (DC removed), in dB
    std::vector<float> db(frameCount);
    std::vector<float> frame(window);
    for (int f = 0; f < frameCount; ++f) {
        const float* x = samples.data() + static_cast<size_t>(f) * hop;
        double mean = 0.0;
        for (int i = 0; i < window; ++i) {
            mean += x[i];
        }
        mean /= window;
        double energy = 0.0;
        for (int i = 0; i < window; ++i) {
            const double v = x[i] - mean;
            energy += v * v;
        }
        db[f] = static_cast<float>(10.0 * std::log10(energy / window + 1e-12));
    }

    // Smoothed over three frames
    std::vector<float> smooth(frameCount);
    for (int f = 0; f < frameCount; ++f) {
        const int a = std::max(0, f - 1);
        const int b = std::min(frameCount - 1, f + 1);
        float sum = 0.f;
        for (int i = a; i <= b; ++i) {
            sum += db[i];
        }
        smooth[f] = sum / float(b - a + 1);
    }

    const float noiseDb = percentile(db, 0.10);
    const float peakDb = percentile(db, 0.99);
    const float thresholdDb = std::max(noiseDb + 12.f, peakDb - 30.f);
    if (info) {
        info->noiseDb = noiseDb;
        info->peakDb = peakDb;
        info->thresholdDb = thresholdDb;
    }
    if (peakDb - noiseDb < 15.f) {
        return result;   // only silence (or only noise)
    }

    // Sound: loud enough and voiced (a click or a hiss is not)
    const int minLag = std::max(2, static_cast<int>(sampleRate / MAX_F0));
    const int maxLag = static_cast<int>(sampleRate / MIN_F0);
    std::vector<char> sound(frameCount, 0);
    for (int f = 0; f < frameCount; ++f) {
        if (smooth[f] < thresholdDb) {
            continue;
        }
        const float* x = samples.data() + static_cast<size_t>(f) * hop;
        double mean = 0.0;
        for (int i = 0; i < window; ++i) {
            mean += x[i];
        }
        mean /= window;
        for (int i = 0; i < window; ++i) {
            frame[i] = static_cast<float>(x[i] - mean);
        }
        sound[f] = periodicity(frame.data(), window, minLag, maxLag) >= VOICED_CORRELATION;
    }

    // Short gaps do not split a syllable
    for (int f = 0; f < frameCount;) {
        if (sound[f]) {
            ++f;
            continue;
        }
        int g = f;
        while (g < frameCount && !sound[g]) {
            ++g;
        }
        if (f > 0 && g < frameCount && g - f <= BRIDGE_FRAMES) {
            std::fill(sound.begin() + f, sound.begin() + g, 1);
        }
        f = g;
    }

    auto frameTime = [&](int f) {
        return (static_cast<double>(f) * hop + window / 2.0) / sampleRate;
    };

    // Runs of sound, each split at its loudness dips (legato syllables)
    for (int f = 0; f < frameCount;) {
        if (!sound[f]) {
            ++f;
            continue;
        }
        const int start = f;
        while (f < frameCount && sound[f]) {
            ++f;
        }
        const int end = f;   // one past the last sound frame
        if (end - start < MIN_SYLLABLE_FRAMES) {
            continue;
        }

        std::vector<int> onsets { start };
        for (int i = start + MIN_DIP_DISTANCE_FRAMES; i < end - MIN_DIP_DISTANCE_FRAMES; ++i) {
            if (smooth[i] > smooth[i - 1] || smooth[i] > smooth[i + 1]) {
                continue;   // not a local minimum
            }
            float leftMax = smooth[i];
            for (int j = std::max(start, i - DIP_REACH_FRAMES); j < i; ++j) {
                leftMax = std::max(leftMax, smooth[j]);
            }
            float rightMax = smooth[i];
            for (int j = i + 1; j <= std::min(end - 1, i + DIP_REACH_FRAMES); ++j) {
                rightMax = std::max(rightMax, smooth[j]);
            }
            if (leftMax - smooth[i] < DIP_DB || rightMax - smooth[i] < DIP_DB) {
                continue;
            }
            if (i - onsets.back() < MIN_DIP_DISTANCE_FRAMES) {
                continue;
            }
            onsets.push_back(i);
        }

        for (size_t k = 0; k < onsets.size(); ++k) {
            const int a = onsets[k];
            const int b = k + 1 < onsets.size() ? onsets[k + 1] : end;
            Syllable s;
            s.onset = frameTime(a);
            s.end = k + 1 < onsets.size() ? frameTime(b) : frameTime(end - 1);
            s.peakDb = *std::max_element(db.begin() + a, db.begin() + b);
            result.push_back(s);
        }
    }

    return result;
}

std::vector<QuantizedNote> mu::notation::bona::quantize(const std::vector<TimedSpan>& spans, int gridTicks)
{
    std::vector<QuantizedNote> notes;
    if (gridTicks <= 0) {
        return notes;
    }

    std::vector<int> ends;
    for (const TimedSpan& span : spans) {
        const int tick = static_cast<int>(std::lround(span.onset / gridTicks)) * gridTicks;
        if (tick < 0) {
            continue;   // sung before the passage (during the count-in)
        }
        // the end leans to the longer value: a held syllable often fades a little early
        int end = static_cast<int>(std::lround(span.end / gridTicks + 0.25)) * gridTicks;
        end = std::max(end, tick + gridTicks);

        if (!notes.empty() && tick <= notes.back().tick) {
            ends.back() = std::max(ends.back(), end);   // on the same grid point: one note
            continue;
        }
        notes.push_back({ tick, 0 });
        ends.push_back(end);
    }

    for (size_t i = 0; i < notes.size(); ++i) {
        int end = ends[i];
        if (i + 1 < notes.size()) {
            const int next = notes[i + 1].tick;
            if (next - end < gridTicks) {
                end = next;   // legato up to the next note; a shorter gap is no rest
            }
        }
        notes[i].duration = end - notes[i].tick;
    }

    return notes;
}

bool mu::notation::bona::writeWav(const std::string& path, const std::vector<float>& samples, int sampleRate)
{
    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        return false;
    }

    auto put32 = [file](uint32_t v) { std::fwrite(&v, 4, 1, file); };
    auto put16 = [file](uint16_t v) { std::fwrite(&v, 2, 1, file); };

    const uint32_t dataBytes = static_cast<uint32_t>(samples.size() * 2);
    std::fwrite("RIFF", 1, 4, file);
    put32(36 + dataBytes);
    std::fwrite("WAVEfmt ", 1, 8, file);
    put32(16);
    put16(1);                       // PCM
    put16(1);                       // mono
    put32(static_cast<uint32_t>(sampleRate));
    put32(static_cast<uint32_t>(sampleRate * 2));
    put16(2);
    put16(16);
    std::fwrite("data", 1, 4, file);
    put32(dataBytes);

    std::vector<int16_t> pcm(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        pcm[i] = static_cast<int16_t>(std::clamp(samples[i], -1.f, 1.f) * 32767.f);
    }
    std::fwrite(pcm.data(), 2, pcm.size(), file);

    const bool ok = std::ferror(file) == 0;
    std::fclose(file);
    return ok;
}
