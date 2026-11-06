/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: microphone input of the Bona window (note entry by voice). The default capture device
 * is opened with miniaudio (thirdparty/miniaudio, public domain / MIT-0) as 16 kHz mono float;
 * while the window is open the level is measured, and between beginRecording() and
 * endRecording() the samples are kept.
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mu::notation::bona {
class MicRecorder
{
public:
    static constexpr int SAMPLE_RATE = 16000;

    MicRecorder();
    ~MicRecorder();

    //! opens the default microphone; false and a message if it cannot
    bool open(std::string& error);
    void close();
    bool isOpen() const;
    std::string deviceName() const;

    //! peak level of the last block, 0..1
    float level() const;

    //! time of the microphone: all frames received since open(), in seconds
    double nowSecs() const;

    //! keeps the samples from now on; their first one is at recordingStartSecs()
    void beginRecording();
    //! stops keeping samples and returns them
    std::vector<float> endRecording();
    bool isRecording() const;
    double recordingStartSecs() const;

    struct Impl;   // public for the miniaudio callback

private:
    std::unique_ptr<Impl> m;
};
}
