/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: ahenk (pitch level of Turkish music), see editahenk.h
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
#include "editahenk.h"

#include <set>
#include <utility>

#include "editpart.h"
#include "transpose.h"

#include "../dom/chord.h"
#include "../dom/instrument.h"
#include "../dom/interval.h"
#include "../dom/keylist.h"
#include "../dom/masterscore.h"
#include "../dom/note.h"
#include "../dom/part.h"
#include "../dom/segment.h"
#include "../dom/staff.h"



namespace mu::engraving::makam {
const std::vector<Ahenk>& ahenkler()
{
    // The neys and their ahenks, each a semitone below the one before; "ses" is the distance below
    // Bolâhenk (Yerinden), where written La sounds as Mi4 (330 Hz)
    static const std::vector<Ahenk> list {
        { "Yazıldığı gibi (Batı)", 0, 0 },   // La4 (440 Hz), above Bolâhenk
        { "Bolâhenk — Yerinden (La = Mi)",              -3, -5 },
        { "Bolâhenk Mâbeyni — yarım ses (La = Mi♭)",    -3, -6 },
        { "Süpürde — 1 ses (La = Re)",                  -4, -7 },
        { "Müstahsen — 1,5 ses (La = Do♯)",             -5, -8 },
        { "Müstahsen Mâbeyni — 2 ses (La = Do)",        -5, -9 },
        { "Kız — 4 ses (La = Si)",                      -6, -10 },
        { "Kız Mâbeyni — 4,5 ses (La = Si♭)",           -6, -11 },
        { "Mansûr — 5 ses (La = La)",                   -7, -12 },
        { "Mansûr Mâbeyni — 5,5 ses (La = La♭)",        -7, -13 },
        { "Şah — 6 ses (La = Sol)",                     -8, -14 },
        { "Dâvut — 6,5 ses (La = Fa♯)",                 -9, -15 },
        { "Dâvut Mâbeyni — 7 ses (La = Fa)",            -9, -16 },
    };
    return list;
}

bool isAhenkPart(const Part* part)
{
    if (!part || part->nstaves() == 0) {
        return false;
    }
    const Instrument* instrument = part->instrument();
    if (!instrument || instrument->useDrumset()) {
        return false;
    }
    const Staff* staff = part->staff(0);
    return staff && staff->isPitchedStaff(Fraction(0, 1));
}

int ahenkIndex(const Score* score)
{
    if (!score) {
        return -1;
    }
    for (const Part* part : score->masterScore()->parts()) {
        if (!isAhenkPart(part)) {
            continue;
        }
        const Interval v = part->instrument()->transpose();
        const std::vector<Ahenk>& list = ahenkler();
        for (size_t i = 0; i < list.size(); ++i) {
            if (list[i].chromatic == v.chromatic) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    return -1;
}

KeySigEvent writtenKeySigEvent(KeySigEvent ke, const Staff* staff, const Fraction& tick)
{
    if (!staff || ke.isAtonal()) {
        return ke;
    }
    const Interval v = staff->part()->instrument(tick)->transpose();
    if (v.isZero()) {
        return ke;
    }
    const Key written = ke.concertKey();
    ke.setConcertKey(Transpose::transposeKey(written, v, staff->part()->preferSharpFlat()));
    ke.setKey(written);
    return ke;
}

static void makamMoveNote(MasterScore* ms, Note* note, const Interval& newV, int delta)
{
    const int tpc1 = Transpose::transposeTpc(note->tpc2(), newV, true);
    ms->undoChangePitch(note, note->pitch() + delta, tpc1, note->tpc2());
}

void setAhenk(Score* score, const Ahenk& ahenk)
{
    if (!score) {
        return;
    }

    MasterScore* ms = score->masterScore();
    const Interval newV(ahenk.diatonic, ahenk.chromatic);

    for (Part* part : ms->parts()) {
        if (!isAhenkPart(part)) {
            continue;
        }
        const Interval oldV = part->instrument()->transpose();
        if (oldV == newV) {
            continue;
        }

        // the written keys, read before the transposition changes
        std::vector<std::pair<Staff*, std::vector<std::pair<Fraction, KeySigEvent> > > > keys;
        for (Staff* staff : part->staves()) {
            std::vector<std::pair<Fraction, KeySigEvent> > staffKeys;
            for (const auto& [tick, ke] : *staff->keyList()) {
                KeySigEvent written = ke;
                written.setConcertKey(ke.key());   // the written key of the old transposition
                staffKeys.push_back({ Fraction::fromTicks(tick), written });
            }
            if (staffKeys.empty() || !staffKeys.front().first.isZero()) {
                KeySigEvent c;
                c.setConcertKey(Key::C);
                staffKeys.insert(staffKeys.begin(), { Fraction(0, 1), c });
            }
            keys.push_back({ staff, staffKeys });
        }

        // 1. the transposition of the part, in the score and in its excerpts
        std::set<Part*> changedParts;
        for (Staff* linkedStaff : part->staff(0)->staffList()) {
            Part* lp = linkedStaff->part();
            if (!lp || changedParts.count(lp)) {
                continue;
            }
            changedParts.insert(lp);
            Instrument* instrument = new Instrument(*lp->instrument());
            instrument->setTranspose(newV);
            ms->undo(new ChangePart(lp, instrument, lp->partName()));
        }

        // 2. the notes keep their written pitch, the sounding pitch moves
        const int delta = newV.chromatic - oldV.chromatic;
        for (Staff* staff : part->staves()) {
            const track_idx_t startTrack = staff->idx() * VOICES;
            for (Segment* s = ms->firstSegment(SegmentType::ChordRest); s; s = s->next1(SegmentType::ChordRest)) {
                for (track_idx_t track = startTrack; track < startTrack + VOICES; ++track) {
                    EngravingItem* e = s->element(track);
                    if (!e || !e->isChord()) {
                        continue;
                    }
                    Chord* chord = toChord(e);
                    for (Chord* grace : chord->graceNotes()) {
                        for (Note* note : grace->notes()) {
                            makamMoveNote(ms, note, newV, delta);
                        }
                    }
                    for (Note* note : chord->notes()) {
                        makamMoveNote(ms, note, newV, delta);
                    }
                }
            }
        }

        // 3. the key signatures keep their written key, the concert key moves
        for (auto& [staff, staffKeys] : keys) {
            for (auto& [tick, written] : staffKeys) {
                ms->undoChangeKeySig(staff, tick, writtenKeySigEvent(written, staff, tick));
            }
        }
    }
}
}
