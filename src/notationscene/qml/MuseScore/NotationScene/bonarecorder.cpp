/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: microphone input of the Bona window, see bonarecorder.h
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
#include "bonarecorder.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX   // windows.h, included by miniaudio, must not define min/max
#endif

// Only the capture device is needed: no decoding, encoding, mixing or sound engine
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

using namespace mu::notation::bona;

struct MicRecorder::Impl {
    ma_device device;
    bool opened = false;
    std::string name;

    std::atomic<uint64_t> frames { 0 };     // all frames received since open
    std::atomic<float> level { 0.f };
    std::atomic<bool> recording { false };

    std::mutex mutex;                       // guards the fields below
    bool started = false;                   // the first recorded block has come
    uint64_t startFrame = 0;
    std::vector<float> samples;

    void onData(const float* input, ma_uint32 count)
    {
        float peak = 0.f;
        if (input) {
            for (ma_uint32 i = 0; i < count; ++i) {
                peak = std::max(peak, std::fabs(input[i]));
            }
        }
        level.store(peak);

        const uint64_t before = frames.load();
        if (recording.load()) {
            std::lock_guard<std::mutex> lock(mutex);
            if (!started) {
                started = true;
                startFrame = before;
            }
            if (input) {
                samples.insert(samples.end(), input, input + count);
            } else {
                samples.insert(samples.end(), count, 0.f);
            }
        }
        frames.store(before + count);
    }
};

static void dataCallback(ma_device* device, void* /*output*/, const void* input, ma_uint32 frameCount)
{
    static_cast<MicRecorder::Impl*>(device->pUserData)->onData(static_cast<const float*>(input), frameCount);
}

MicRecorder::MicRecorder()
    : m(std::make_unique<Impl>())
{
}

MicRecorder::~MicRecorder()
{
    close();
}

// miniaudio initializes COM on the thread that opens and closes the device, and on some paths
// uninitializes it unconditionally; the main thread's COM (Qt: drag and drop, file dialogs) must
// stay as it is, so the device is opened and closed on a thread of its own.
template<typename Func>
static void onOwnThread(Func f)
{
    std::thread thread(f);
    thread.join();
}

bool MicRecorder::open(std::string& error)
{
    if (m->opened) {
        return true;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.format = ma_format_f32;
    config.capture.channels = 1;
    config.sampleRate = SAMPLE_RATE;
    config.periodSizeInMilliseconds = 10;
    config.dataCallback = dataCallback;
    config.pUserData = m.get();

    m->frames.store(0);
    onOwnThread([this, &config, &error]() {
        if (ma_device_init(nullptr, &config, &m->device) != MA_SUCCESS) {
            error = "Mikrofon açılamadı. Bilgisayarda bir mikrofon takılı mı, Windows'un gizlilik ayarlarında "
                    "masaüstü uygulamalarının mikrofona erişimi açık mı?";
            return;
        }

        char name[256] = {};
        if (ma_device_get_name(&m->device, ma_device_type_capture, name, sizeof(name), nullptr) == MA_SUCCESS) {
            m->name = name;
        }

        if (ma_device_start(&m->device) != MA_SUCCESS) {
            ma_device_uninit(&m->device);
            error = "Mikrofon başlatılamadı.";
            return;
        }

        m->opened = true;
    });
    return m->opened;
}

void MicRecorder::close()
{
    if (!m->opened) {
        return;
    }
    m->recording.store(false);
    onOwnThread([this]() { ma_device_uninit(&m->device); });
    m->opened = false;
    m->level.store(0.f);
}

bool MicRecorder::isOpen() const
{
    return m->opened;
}

std::string MicRecorder::deviceName() const
{
    return m->name;
}

float MicRecorder::level() const
{
    return m->level.load();
}

double MicRecorder::nowSecs() const
{
    return static_cast<double>(m->frames.load()) / SAMPLE_RATE;
}

void MicRecorder::beginRecording()
{
    {
        std::lock_guard<std::mutex> lock(m->mutex);
        m->samples.clear();
        m->samples.reserve(SAMPLE_RATE * 120);
        m->started = false;
        m->startFrame = m->frames.load();
    }
    m->recording.store(true);
}

std::vector<float> MicRecorder::endRecording()
{
    m->recording.store(false);
    std::lock_guard<std::mutex> lock(m->mutex);
    std::vector<float> result;
    result.swap(m->samples);
    return result;
}

bool MicRecorder::isRecording() const
{
    return m->recording.load();
}

double MicRecorder::recordingStartSecs() const
{
    std::lock_guard<std::mutex> lock(m->mutex);
    return static_cast<double>(m->startFrame) / SAMPLE_RATE;
}
