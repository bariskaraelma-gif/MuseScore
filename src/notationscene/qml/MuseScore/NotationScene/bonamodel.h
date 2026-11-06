/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the "Bona" window, note entry by voice. The user selects a measure, presses
 * Record and, after the count-in, sings with the metronome; on Stop the syllables are found,
 * put on the grid (8th or 16th) and written from the selected measure on, in one undo step.
 *
 * Stage 1: the rhythm only, every note is written as La. The time of the microphone is matched
 * to the time of the playback by the playback position updates (the median of their offsets);
 * "Gecikme" moves the notes earlier by the latency of the sound card.
 *
 * The last recording and its analysis are kept in the temp folder (makam-bona-son.wav/.txt).
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

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"
#include "playback/iplaybackcontroller.h"

#include "bonaanalysis.h"
#include "bonarecorder.h"

namespace mu::notation {
class BonaModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(double level READ level NOTIFY levelChanged)
    Q_PROPERTY(QString micName READ micName NOTIFY micNameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString countText READ countText NOTIFY countTextChanged)
    Q_PROPERTY(QString resultText READ resultText NOTIFY resultTextChanged)
    Q_PROPERTY(int grid READ grid WRITE setGrid NOTIFY gridChanged)
    Q_PROPERTY(int latencyMs READ latencyMs WRITE setLatencyMs NOTIFY latencyMsChanged)
    Q_PROPERTY(bool canReanalyze READ canReanalyze NOTIFY resultTextChanged)

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };

public:
    explicit BonaModel(QObject* parent = nullptr);
    ~BonaModel() override;

    QString state() const;   // "idle", "countin", "recording"
    double level() const;
    QString micName() const;
    QString statusText() const;
    QString countText() const;
    QString resultText() const;
    int grid() const;        // 8 or 16
    int latencyMs() const;
    bool canReanalyze() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void toggleRecording();
    Q_INVOKABLE void reanalyze();
    Q_INVOKABLE void close();   // the window closes: stop a recording, release the microphone

public slots:
    void setGrid(int grid);
    void setLatencyMs(int ms);

signals:
    void stateChanged();
    void levelChanged();
    void micNameChanged();
    void statusTextChanged();
    void countTextChanged();
    void resultTextChanged();
    void gridChanged();
    void latencyMsChanged();

private:
    void start();
    void finish();
    void stopPlaybackAndRestore();
    void analyzeAndWrite();
    void onTimer();
    void onPlaybackPosition(double pos, int tick);

    void setState(const QString& state);
    void setStatusText(const QString& text);
    void setCountText(const QString& text);

    bona::MicRecorder m_mic;
    QTimer m_timer;
    QElapsedTimer m_countClock;

    QString m_state = "idle";
    QString m_statusText;
    QString m_countText;
    QString m_resultText;
    int m_grid = 16;
    int m_latencyMs = 190;   // measured on a laptop (Realtek, headphones)
    double m_level = 0.0;

    // the passage being recorded
    int m_startTick = 0;
    size_t m_staffIdx = 0;
    int m_writtenEndTick = 0;       // end of what was written last (cleared on a new analysis)
    double m_beatSecs = 0.75;       // for the count shown during the count-in
    bool m_restoreMetronome = false;
    bool m_restoreCountIn = false;

    // the recording and its time
    std::vector<float> m_samples;
    double m_recordingStartSecs = 0.0;   // microphone time of the first sample
    double m_firstPosition = -1.0;       // playback position when it started (count-in: no move)
    std::vector<double> m_offsets;       // microphone time - playback time
};
}
