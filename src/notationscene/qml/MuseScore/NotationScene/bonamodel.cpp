/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the "Bona" window, see bonamodel.h
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
#include "bonamodel.h"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

#include "settings.h"
#include "types/translatablestring.h"

#include "engraving/dom/chordrest.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/utils.h"
#include "engraving/types/constants.h"

using namespace mu::notation;

namespace {
const muse::Settings::Key BONA_GRID_KEY("makam", "makam/bonaGrid");
const muse::Settings::Key BONA_LATENCY_KEY("makam", "makam/bonaLatencyMs");

constexpr int LA_STEP = 5 * 7 + 5;   // La of the middle octave (A4), the one pitch of stage 1

//! a note (step >= 0) or a rest (step < 0) at tick, lasting duration; ties over the barlines
bool putNoteRest(mu::engraving::Score* score, mu::engraving::track_idx_t track, const mu::engraving::Fraction& tick,
                 const mu::engraving::Fraction& duration, int step)
{
    using namespace mu::engraving;

    Segment* segment = score->tick2segment(tick, true, SegmentType::ChordRest);
    if (!segment || segment->tick() != tick || !segment->element(track)) {
        return false;
    }

    NoteVal nval;   // a rest
    if (step >= 0) {
        Position pos;
        pos.segment = segment;
        pos.staffIdx = track2staff(track);
        pos.line = relStep(step, score->staff(pos.staffIdx)->clef(tick));
        bool error = false;
        nval = score->noteValForPosition(pos, AccidentalType::NONE, error);
        if (error) {
            return false;
        }
    }

    score->setNoteRest(segment, track, nval, duration, DirectionV::AUTO, false, {}, true /*rhythmic*/);
    return true;
}

QString durationName(int ticks)
{
    using mu::engraving::Constants;
    const int q = Constants::DIVISION;
    switch (ticks) {
    case q * 4: return "1";
    case q * 3: return "2.";
    case q * 2: return "2";
    case q * 3 / 2: return "4.";
    case q: return "4";
    case q * 3 / 4: return "8.";
    case q / 2: return "8";
    case q / 4: return "16";
    default: break;
    }
    return QString("%1/16").arg(ticks / (q / 4));
}

QString tempPath(const QString& name)
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(name);
}
}

BonaModel::BonaModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &BonaModel::onTimer);
}

BonaModel::~BonaModel()
{
    close();
}

void BonaModel::close()
{
    m_timer.stop();
    if (m_state != "idle") {
        m_mic.endRecording();
        setState("idle");
        stopPlaybackAndRestore();
    }
    m_mic.close();
}

void BonaModel::load()
{
    const muse::Val grid = muse::settings()->value(BONA_GRID_KEY);
    if (!grid.isNull() && (grid.toInt() == 8 || grid.toInt() == 16)) {
        m_grid = grid.toInt();
    }
    const muse::Val latency = muse::settings()->value(BONA_LATENCY_KEY);
    if (!latency.isNull()) {
        m_latencyMs = std::clamp(latency.toInt(), -500, 500);
    }
    emit gridChanged();
    emit latencyMsChanged();

    std::string error;
    if (m_mic.open(error)) {
        emit micNameChanged();
        setStatusText("Yazmaya başlanacak ölçüyü seçin, sonra Kaydet'e basın. "
                      "Bir ölçü sayılır, sonra metronomla birlikte okuyun.");
    } else {
        setStatusText(QString::fromStdString(error));
    }

    playbackController()->currentPlaybackPositionChanged().onReceive(this, [this](muse::audio::secs_t pos, muse::midi::tick_t tick) {
        onPlaybackPosition(pos.to_double(), static_cast<int>(tick));
    });
    playbackController()->isPlayingChanged().onNotify(this, [this]() {
        // the playback stopped by itself (end of the score) or from the main window
        if (m_state == "recording" && !playbackController()->isPlaying()) {
            finish();
        }
    });

    m_timer.start();
}

QString BonaModel::state() const
{
    return m_state;
}

double BonaModel::level() const
{
    return m_level;
}

QString BonaModel::micName() const
{
    return QString::fromStdString(m_mic.deviceName());
}

QString BonaModel::statusText() const
{
    return m_statusText;
}

QString BonaModel::countText() const
{
    return m_countText;
}

QString BonaModel::resultText() const
{
    return m_resultText;
}

int BonaModel::grid() const
{
    return m_grid;
}

int BonaModel::latencyMs() const
{
    return m_latencyMs;
}

bool BonaModel::canReanalyze() const
{
    return m_state == "idle" && !m_samples.empty() && !m_offsets.empty();
}

void BonaModel::setGrid(int grid)
{
    if (grid != 8 && grid != 16) {
        return;
    }
    if (m_grid != grid) {
        m_grid = grid;
        muse::settings()->setSharedValue(BONA_GRID_KEY, muse::Val(grid));
        emit gridChanged();
    }
}

void BonaModel::setLatencyMs(int ms)
{
    ms = std::clamp(ms, -500, 500);
    if (m_latencyMs != ms) {
        m_latencyMs = ms;
        muse::settings()->setSharedValue(BONA_LATENCY_KEY, muse::Val(ms));
        emit latencyMsChanged();
    }
}

void BonaModel::toggleRecording()
{
    if (m_state == "idle") {
        start();
    } else {
        finish();
    }
}

void BonaModel::reanalyze()
{
    if (canReanalyze()) {
        analyzeAndWrite();
    }
}

void BonaModel::start()
{
    using namespace mu::engraving;

    INotationPtr notation = globalContext()->currentNotation();
    Score* score = notation ? notation->elements()->msScore() : nullptr;
    if (!score) {
        setStatusText("Açık bir eser yok.");
        return;
    }
    if (!m_mic.isOpen()) {
        std::string error;
        if (!m_mic.open(error)) {
            setStatusText(QString::fromStdString(error));
            return;
        }
        emit micNameChanged();
    }
    if (!playbackController()->isPlayAllowed()) {
        setStatusText("Çalma şu an kullanılamıyor (sesler yükleniyor olabilir). Biraz sonra yeniden deneyin.");
        return;
    }

    // The measure and the staff of the selection
    const Selection& selection = score->selection();
    Fraction tick;
    staff_idx_t staffIdx = 0;
    if (selection.isRange()) {
        tick = selection.tickStart();
        staffIdx = selection.staffStart();
    } else if (EngravingItem* item = selection.element()) {
        tick = item->tick();
        staffIdx = item->staffIdx();
    } else {
        setStatusText("Önce yazmaya başlanacak ölçüyü seçin (ölçünün boş bir yerine tıklayın).");
        return;
    }
    Measure* measure = score->tick2measure(tick);
    Staff* staff = score->staff(staffIdx);
    if (!measure || !staff) {
        setStatusText("Seçimin ölçüsü bulunamadı.");
        return;
    }
    if (staff->isDrumStaff(measure->tick())) {
        setStatusText("Usul dizeğine nota yazılmaz. Çalgı dizeğinde bir ölçü seçin.");
        return;
    }

    m_startTick = measure->tick().ticks();
    m_staffIdx = staffIdx;
    m_writtenEndTick = m_startTick;

    const mu::notation::Tempo tempo = globalContext()->currentMasterNotation()->playback()->multipliedTempo(m_startTick);
    m_beatSecs = tempo.valueBpm > 0 ? 60.0 / tempo.valueBpm : 0.75;

    // Metronome and count-in on for the recording, as they were afterwards
    m_restoreMetronome = !playbackController()->actionChecked("metronome");
    if (m_restoreMetronome) {
        dispatcher()->dispatch("metronome");
    }
    m_restoreCountIn = !playbackController()->actionChecked("countin");
    if (m_restoreCountIn) {
        dispatcher()->dispatch("countin");
    }

    if (playbackController()->isPlaying()) {
        dispatcher()->dispatch("stop");
    }

    // Play from the start of the measure
    Segment* first = measure->first(SegmentType::ChordRest);
    EngravingItem* seekTo = first ? first->element(staff2track(staffIdx)) : nullptr;
    playbackController()->seekElement(seekTo ? seekTo : static_cast<EngravingItem*>(measure));

    m_offsets.clear();
    m_samples.clear();
    m_firstPosition = -1.0;
    m_mic.beginRecording();
    m_timer.start();

    setState("countin");
    setCountText("");
    setStatusText(QString("Sayım… %1. ölçüden yazılacak. Bitirmek için Dur'a basın.").arg(measure->no() + 1));
    m_countClock.start();

    dispatcher()->dispatch("play");
}

void BonaModel::onPlaybackPosition(double pos, int tick)
{
    if (m_state != "countin" && m_state != "recording") {
        return;
    }

    // During the count-in the position does not move; the first move starts the passage
    if (m_firstPosition < 0.0) {
        m_firstPosition = pos;
        return;
    }
    if (pos <= m_firstPosition + 0.0005) {
        return;
    }

    m_offsets.push_back(m_mic.nowSecs() - pos);

    if (m_state == "countin") {
        setState("recording");
    }

    const MeasureBeat beat = globalContext()->currentMasterNotation()->playback()->beat(tick);
    setCountText(QString("%1. ölçü · %2").arg(beat.measureIndex + 1).arg(static_cast<int>(beat.beat) + 1));
}

void BonaModel::onTimer()
{
    const double level = m_mic.isOpen() ? std::min(1.0, static_cast<double>(m_mic.level()) * 1.5) : 0.0;
    if (std::abs(level - m_level) > 0.005) {
        m_level = level;
        emit levelChanged();
    }

    if (m_state == "countin" && m_beatSecs > 0.0) {
        const int count = static_cast<int>(m_countClock.elapsed() / 1000.0 / m_beatSecs) + 1;
        setCountText(QString::number(count));
    }
}

void BonaModel::finish()
{
    if (m_state == "idle") {
        return;
    }

    m_samples = m_mic.endRecording();
    m_recordingStartSecs = m_mic.recordingStartSecs();
    setState("idle");   // before stopping: the stop notifies isPlayingChanged
    setCountText("");
    stopPlaybackAndRestore();

    if (m_offsets.empty()) {
        m_samples.clear();
        setStatusText("Çalma başlamadan kayıt durdu; bir şey yazılmadı.");
        m_resultText.clear();
        emit resultTextChanged();
        return;
    }

    m_writtenEndTick = m_startTick;
    analyzeAndWrite();
}

void BonaModel::stopPlaybackAndRestore()
{
    if (playbackController()->isPlaying()) {
        dispatcher()->dispatch("stop");
    }
    if (m_restoreMetronome) {
        dispatcher()->dispatch("metronome");
        m_restoreMetronome = false;
    }
    if (m_restoreCountIn) {
        dispatcher()->dispatch("countin");
        m_restoreCountIn = false;
    }
}

void BonaModel::analyzeAndWrite()
{
    using namespace mu::engraving;

    INotationPtr notation = globalContext()->currentNotation();
    IMasterNotationPtr master = globalContext()->currentMasterNotation();
    Score* score = notation ? notation->elements()->msScore() : nullptr;
    if (!score || !master) {
        setStatusText("Açık bir eser yok.");
        return;
    }

    // Microphone time -> playback time: the median of the offsets
    std::vector<double> offsets = m_offsets;
    std::nth_element(offsets.begin(), offsets.begin() + offsets.size() / 2, offsets.end());
    const double offset = offsets[offsets.size() / 2];
    const double latency = m_latencyMs / 1000.0;

    bona::AnalysisInfo info;
    const std::vector<bona::Syllable> syllables = bona::findSyllables(m_samples, bona::MicRecorder::SAMPLE_RATE, &info);

    // Before the start of the score (a syllable sung a little early on the first beat of the
    // first measure) the time goes on at the tempo of the start
    INotationPlaybackPtr playback = master->playback();
    const double ticksPerSec = (static_cast<double>(playback->secToTick(0.5)) - playback->secToTick(0.0)) / 0.5;

    std::vector<bona::TimedSpan> spans;
    auto toTicks = [&](double micSecs) {
        const double playSecs = micSecs - offset - latency;
        const double tick = playSecs >= 0.0
                            ? static_cast<double>(playback->secToTick(playSecs))
                            : static_cast<double>(playback->secToTick(0.0)) + playSecs * ticksPerSec;
        return tick - m_startTick;
    };
    for (const bona::Syllable& s : syllables) {
        spans.push_back({ toTicks(m_recordingStartSecs + s.onset), toTicks(m_recordingStartSecs + s.end) });
    }

    const int gridTicks = Constants::DIVISION * 4 / m_grid;
    const std::vector<bona::QuantizedNote> notes = bona::quantize(spans, gridTicks);

    // The analysis, for checking afterwards
    {
        bona::writeWav(tempPath("makam-bona-son.wav").toStdString(), m_samples, bona::MicRecorder::SAMPLE_RATE);
        QFile file(tempPath("makam-bona-son.txt"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "mikrofon: " << micName() << "\n";
            out << "ses (dB): sessizlik " << info.noiseDb << ", en yüksek " << info.peakDb << ", eşik " << info.thresholdDb << "\n";
            out << "kayit basi (mikrofon sn): " << m_recordingStartSecs << ", fark (medyan): " << offset
                << ", fark sayisi: " << m_offsets.size() << ", gecikme ms: " << m_latencyMs << "\n";
            out << "baslangic tick: " << m_startTick << ", birim: " << m_grid << "'lik (" << gridTicks << " tick)\n";
            out << "heceler (kayit sn -> tick):\n";
            for (size_t i = 0; i < syllables.size(); ++i) {
                out << "  " << syllables[i].onset << " - " << syllables[i].end << "  ->  " << spans[i].onset << " - " << spans[i].end
                    << "  (" << syllables[i].peakDb << " dB)\n";
            }
            out << "notalar (tick, sure):\n";
            for (const bona::QuantizedNote& n : notes) {
                out << "  " << n.tick << "  " << n.duration << "\n";
            }
        }
    }

    if (notes.empty()) {
        setStatusText(syllables.empty()
                      ? "Ses bulunamadı. Mikrofonun seviyesi oynuyor mu? Biraz daha yüksek sesle okuyun."
                      : "Heceler ölçünün başından önce kaldı; yazılacak nota yok. Sayımdan sonra okuyun.");
        m_resultText.clear();
        emit resultTextChanged();
        return;
    }

    // Writing: from the start of the measure, rests in the gaps, and what an earlier analysis of
    // the same recording wrote further is cleared
    const track_idx_t track = staff2track(m_staffIdx);
    const Fraction start = Fraction::fromTicks(m_startTick);
    Fraction cur = start;
    int written = 0;

    INotationUndoStackPtr undoStack = notation->undoStack();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Bona"));
    for (const bona::QuantizedNote& n : notes) {
        const Fraction tick = start + Fraction::fromTicks(n.tick);
        if (tick > cur) {
            if (!putNoteRest(score, track, cur, tick - cur, -1)) {
                break;
            }
            cur = tick;
        }
        const Fraction duration = Fraction::fromTicks(n.duration);
        if (!putNoteRest(score, track, cur, duration, LA_STEP)) {
            break;
        }
        cur += duration;
        ++written;
    }
    const Fraction end = cur;
    if (cur.ticks() < m_writtenEndTick) {
        putNoteRest(score, track, cur, Fraction::fromTicks(m_writtenEndTick) - cur, -1);
    }
    m_writtenEndTick = end.ticks();

    // The written passage stays selected
    ChordRest* firstCr = score->findCR(start, track);
    ChordRest* lastCr = end > start ? score->findCR(end - Fraction::fromTicks(1), track) : nullptr;
    if (firstCr) {
        score->select(firstCr, SelectType::SINGLE, m_staffIdx);
        if (lastCr && lastCr != firstCr) {
            score->select(lastCr, SelectType::RANGE, m_staffIdx);
        }
    }
    undoStack->commitChanges();

    // The rhythm as text: 4 4 8 8 | ...
    QStringList parts;
    for (const bona::QuantizedNote& n : notes) {
        parts << durationName(n.duration);
    }
    m_resultText = QString("%1 nota yazıldı (%2'lik birimle): %3").arg(written).arg(m_grid).arg(parts.join("  "));
    emit resultTextChanged();

    const Measure* lastMeasure = score->tick2measure(end - Fraction::fromTicks(1));
    setStatusText(QString("%1.–%2. ölçü yazıldı. Ctrl+Z bir kerede geri alır. "
                          "Notalar geç ya da erken çıkıyorsa Gecikme'yi değiştirip Yeniden çözümle'ye basın.")
                  .arg(score->tick2measure(start)->no() + 1)
                  .arg(lastMeasure ? lastMeasure->no() + 1 : score->tick2measure(start)->no() + 1));
}

void BonaModel::setState(const QString& state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged();
        emit resultTextChanged();   // canReanalyze
    }
}

void BonaModel::setStatusText(const QString& text)
{
    if (m_statusText != text) {
        m_statusText = text;
        emit statusTextChanged();
    }
}

void BonaModel::setCountText(const QString& text)
{
    if (m_countText != text) {
        m_countText = text;
        emit countTextChanged();
    }
}
