/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the MusicXML files of the SymbTr corpus opened as Turkish makam scores (makamsymbtr.h).
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
#include "makamsymbtr.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include "editpart.h"
#include "makamtuner.h"

#include "../dom/accidental.h"
#include "../dom/box.h"
#include "../dom/chord.h"
#include "../dom/factory.h"
#include "../dom/instrtemplate.h"
#include "../dom/instrument.h"
#include "../dom/key.h"
#include "../dom/makamdata.h"
#include "../dom/masterscore.h"
#include "../dom/measure.h"
#include "../dom/measurebase.h"
#include "../dom/note.h"
#include "../dom/part.h"
#include "../dom/segment.h"
#include "../dom/lyrics.h"
#include "../dom/staff.h"
#include "../dom/stafftext.h"
#include "../dom/text.h"
#include "../dom/textbase.h"
#include "../dom/timesig.h"

#include "log.h"

namespace mu::engraving::makam {
namespace {
// "Makam: Hicaz, Form: Şarkı, Usul: Ağır Aksaksemâî", before the metronome mark of the same text
struct Header {
    TextBase* text = nullptr;
    QString xml;
    qsizetype start = 0;
    qsizetype length = 0;
    QString makam;
    QString form;
    QString usul;
};

const QRegularExpression& headerPattern()
{
    static const QRegularExpression re(QStringLiteral(
                                           "Makam:\\s*([^,<]+?)\\s*,\\s*Form:\\s*([^,<]*?)\\s*,\\s*Usul:\\s*([^<]*?)\\s*(?=<|$)"));
    return re;
}

std::optional<Header> findHeader(MasterScore* score)
{
    Measure* measure = score ? score->firstMeasure() : nullptr;
    if (!measure) {
        return std::nullopt;
    }
    for (Segment* s = measure->first(); s; s = s->next()) {
        for (EngravingItem* e : s->annotations()) {
            if (!e || !e->isTextBase()) {
                continue;
            }
            TextBase* text = toTextBase(e);
            const QString xml = text->xmlText().toQString();
            const QRegularExpressionMatch match = headerPattern().match(xml);
            if (!match.hasMatch()) {
                continue;
            }
            Header h;
            h.text = text;
            h.xml = xml;
            h.start = match.capturedStart();
            h.length = match.capturedLength();
            h.makam = match.captured(1).trimmed();
            h.form = match.captured(2).trimmed();
            h.usul = match.captured(3).trimmed();
            return h;
        }
    }
    return std::nullopt;
}

// Lower case, Turkish letters as ASCII, letters only: "Ağır Aksaksemâî" -> "agiraksaksemai"
QString letters(const QString& text)
{
    QString s = text;
    s.replace(QChar(u'I'), QChar(u'ı'));
    s.replace(QChar(u'İ'), QChar(u'i'));
    s = s.toLower();
    static const std::vector<std::pair<char16_t, char16_t> > map {
        { u'ı', u'i' }, { u'ş', u's' }, { u'ğ', u'g' }, { u'ü', u'u' }, { u'ö', u'o' },
        { u'ç', u'c' }, { u'â', u'a' }, { u'î', u'i' }, { u'û', u'u' },
    };
    for (const auto& [from, to] : map) {
        s.replace(QChar(from), QChar(to));
    }
    QString out;
    for (const QChar c : s) {
        if (c >= u'a' && c <= u'z') {
            out += c;
        }
    }
    return out;
}

// The header text keeps only the metronome mark; the makam, form and usul go to the subtitle
void writeHeader(MasterScore* score, const Header& h)
{
    QString rest = h.xml;
    rest.remove(h.start, h.length);
    rest = rest.trimmed();
    if (rest.isEmpty()) {
        score->undoRemoveElement(h.text);
    } else {
        h.text->undoChangeProperty(Pid::TEXT, String::fromQString(rest));
    }

    QStringList parts;
    for (const QString& part : { h.makam, h.form, h.usul }) {
        if (!part.isEmpty() && part != u"?") {
            parts << part;
        }
    }
    if (parts.isEmpty()) {
        return;
    }
    // SymbTr's titles are in title case, which makes the izafet "-i" a capital: "Bîgâne-I Hâb"
    static const QRegularExpression izafet(QStringLiteral("-[Iİ](?=\\s|$)"));
    const QString workTitle = score->metaTag(u"workTitle").toQString();
    if (workTitle.contains(izafet)) {
        score->setMetaTag(u"workTitle", String::fromQString(QString(workTitle).replace(izafet, QStringLiteral("-i"))));
    }

    MeasureBase* first = score->first();
    if (first && first->isVBox()) {
        const ElementList texts = first->el();
        for (EngravingItem* e : texts) {
            if (!e || !e->isText()) {
                continue;
            }
            Text* text = toText(e);
            if (text->plainText().trimmed() == u"?") {
                score->undoRemoveElement(e);   // SymbTr writes "?" for an unknown poet or composer
                continue;
            }
            const QString xml = text->xmlText().toQString();
            if (xml.contains(izafet)) {
                text->undoChangeProperty(Pid::TEXT, String::fromQString(QString(xml).replace(izafet, QStringLiteral("-i"))));
            }
        }
    }
    if (!first || !first->isVBox()) {
        first = score->insertBox(ElementType::VBOX, first);
    }
    if (!first) {
        return;
    }
    Text* subtitle = Factory::createText(first, TextStyleType::SUBTITLE);
    subtitle->setParent(first);
    subtitle->setXmlText(String::fromQString(parts.join(QStringLiteral(" · ")).toHtmlEscaped()));
    score->undoAddElement(subtitle);
}

// Beam groups of an x/8 usul as Turkish music writes them: the eighths of an odd number in twos
// with a three at the end (Aksak 9/8 = 2+2+2+3, 7/8 = 2+2+3, 5/8 = 2+3), the 16ths in twos of
// eighths (9/8 = 2+2+2+2+1); Düyek 8/8 all in twos. Empty for the others (MuseScore's grouping,
// or the usul's own beams). Positions in 1/32, an eighth is 4.
GroupNodes aksakNodes(const Fraction& sig)
{
    GroupNodes nodes;
    const int n = sig.numerator();
    if (sig.denominator() != 8 || n < 5 || (n % 2 == 0 && n != 8)) {
        return nodes;
    }
    const int lastEighthBreak = n % 2 ? n - 3 : n - 2;   // the eighths' last group is 3 (odd) or 2
    for (int e = 2; e < n; e += 2) {
        const bool eighthBreak = e <= lastEighthBreak;
        nodes.push_back(GroupNode { e * 4, eighthBreak ? 0x111 : 0x110 });   // 0x110: 16ths and 32nds only
    }
    return nodes;
}

// The time signatures of the usul (same name and same n/d) get the usul marker
void applyUsul(MasterScore* score, const QString& usulName)
{
    QString key = letters(usulName);
    if (key == u"cenkiharbi") {
        key = QStringLiteral("cengiharbi");
    }
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::TimeSig); s; s = s->next(SegmentType::TimeSig)) {
            for (track_idx_t track = 0; track < score->ntracks(); track += VOICES) {
                EngravingItem* e = s->element(track);
                if (!e || !e->isTimeSig()) {
                    continue;
                }
                TimeSig* ts = toTimeSig(e);
                const GroupNodes aksak = aksakNodes(ts->sig());
                bool isUsul = false;
                for (const Usul& usul : usuller()) {
                    if (!key.isEmpty() && letters(QString::fromStdString(usul.name)) == key && ts->sig().identical(Fraction(usul.n, usul.d))) {
                        GroupNodes nodes = usulGroups(usul).nodes();
                        if (usul.beams.empty() && !aksak.empty()) {
                            nodes = aksak;
                            nodes.push_back(GroupNode { USUL_MARKER_BASE + usul.code, 0 });
                        }
                        ts->undoChangeProperty(Pid::GROUP_NODES, nodes);
                        isUsul = true;
                        break;
                    }
                }
                if (!isUsul && ts->groups().empty() && !aksak.empty()) {
                    ts->undoChangeProperty(Pid::GROUP_NODES, aksak);
                }
                // Ağır Aksak 9/4, Ağır Aksak Semâî 10/4, Fahte 20/4…: beams end on every quarter.
                // (MuseScore's default grouping of a time signature it has no table for comes out
                // empty, Groups::endings: the whole measure would be one beam.)
                if (!isUsul && ts->groups().empty() && ts->sig().denominator() == 4) {
                    GroupNodes nodes;
                    for (int i = 1; i < ts->sig().numerator(); ++i) {
                        nodes.push_back(GroupNode { 8 * i, 0x111 });
                    }
                    ts->undoChangeProperty(Pid::GROUP_NODES, nodes);
                }
            }
        }
    }
}

// SymbTr writes "." under every note of an instrumental passage, and the name of a section
// ("ARANAĞME", "SAZ") as a lyric: the dots go, the names become staff texts ("Aranağme")
void cleanLyrics(MasterScore* score)
{
    static const QRegularExpression sectionName(QStringLiteral("^[A-ZÇĞİÖŞÜÂÎÛ][A-ZÇĞİÖŞÜÂÎÛ ]{2,}\\.?$"));
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            for (track_idx_t track = 0; track < score->ntracks(); ++track) {
                EngravingItem* e = s->element(track);
                if (!e || !e->isChordRest()) {
                    continue;
                }
                const std::vector<Lyrics*> lyrics = toChordRest(e)->lyrics();
                for (Lyrics* l : lyrics) {
                    const QString text = l->plainText().toQString().trimmed();
                    if (text == u".") {
                        score->undoRemoveElement(l);
                    } else if (sectionName.match(text).hasMatch()) {
                        QString name = text;
                        if (name.endsWith(u'.')) {
                            name.chop(1);
                        }
                        name = name.left(1) + name.mid(1).replace(u'I', u'ı').replace(u'İ', u'i').toLower();
                        StaffText* st = Factory::createStaffText(s);
                        st->setParent(s);
                        st->setTrack(track);
                        st->setXmlText(String::fromQString(name.toHtmlEscaped()));
                        score->undoAddElement(st);
                        score->undoRemoveElement(l);
                    }
                }
            }
        }
    }
}

// SymbTr's beams run over whole half measures; the time signature's grouping (the usul's) instead
void resetBeams(MasterScore* score)
{
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            for (track_idx_t track = 0; track < score->ntracks(); ++track) {
                EngravingItem* e = s->element(track);
                if (!e || !e->isChordRest()) {
                    continue;
                }
                ChordRest* cr = toChordRest(e);
                if (cr->beamMode() != BeamMode::AUTO) {
                    cr->undoChangeProperty(Pid::BEAM_MODE, BeamMode::AUTO);
                }
                if (cr->isChord()) {
                    for (Chord* grace : toChord(cr)->graceNotes()) {
                        if (grace->beamMode() != BeamMode::AUTO) {
                            grace->undoChangeProperty(Pid::BEAM_MODE, BeamMode::AUTO);
                        }
                    }
                }
            }
        }
    }
}

// SymbTr's one part is called "Music": TurkScore's Türk Müziği instrument (name, sound)
void useTurkishInstrument(MasterScore* score)
{
    const InstrumentTemplate* templ = searchTemplate(u"turk-muzigi");
    if (!templ) {
        return;
    }
    const std::vector<Part*> parts = score->parts();
    for (Part* part : parts) {
        if (part->partName() == u"Music" || part->longName() == u"Music") {
            EditPart::replacePartInstrument(score, part, Instrument::fromTemplate(templ));
        }
    }
}

// The ♯ (1) / ♭ (-1) in the written pitch, as the tuner reads it
int writtenAlter(const Note* note)
{
    return (note->tpc2() + 1 + 7) / 7 - 3;
}

int letterIndex(const Note* note)
{
    static const char* const FIFTHS = "FCGDAEB";
    const char letter = FIFTHS[((note->tpc2() + 1) % 7 + 7) % 7];
    return static_cast<int>(std::string("CDEFGAB").find(letter));
}

// Every note of the pitched staves, in score order (grace notes before their chord)
std::vector<std::vector<Note*> > notesByMeasureAndTrack(MasterScore* score, std::vector<track_idx_t>& tracks)
{
    std::vector<std::vector<Note*> > result;
    for (Measure* m = score->firstMeasure(); m; m = m->nextMeasure()) {
        std::map<track_idx_t, std::vector<Note*> > byTrack;
        for (Segment* s = m->first(SegmentType::ChordRest); s; s = s->next(SegmentType::ChordRest)) {
            for (track_idx_t track = 0; track < score->ntracks(); ++track) {
                EngravingItem* e = s->element(track);
                if (!e || !e->isChord()) {
                    continue;
                }
                const Staff* staff = score->staff(track / VOICES);
                if (!staff || !staff->isPitchedStaff(s->tick())) {
                    continue;
                }
                Chord* chord = toChord(e);
                for (Chord* grace : chord->graceNotes()) {
                    for (Note* note : grace->notes()) {
                        byTrack[track].push_back(note);
                    }
                }
                for (Note* note : chord->notes()) {
                    byTrack[track].push_back(note);
                }
            }
        }
        for (auto& [track, notes] : byTrack) {
            tracks.push_back(track);
            result.push_back(std::move(notes));
        }
    }
    return result;
}

bool isTieContinuation(const Note* note)
{
    const Note* first = note->firstTiedNote();
    return first && first != note;
}
}

bool isSymbTrScore(MasterScore* score)
{
    return findHeader(score).has_value();
}

void fixSymbTrScore(MasterScore* score)
{
    const std::optional<Header> header = findHeader(score);
    if (!header) {
        return;
    }
    const QString usulName = header->usul;
    writeHeader(score, *header);
    applyUsul(score, usulName);
    resetBeams(score);
    cleanLyrics(score);
    useTurkishInstrument(score);

    // No instrument name and no indent on the first system: the melody gets the whole width
    score->style().set(Sid::firstSystemInstNameVisibility, static_cast<int>(InstrumentLabelVisibility::HIDE));
    score->style().set(Sid::subsSystemInstNameVisibility, static_cast<int>(InstrumentLabelVisibility::HIDE));
    score->style().set(Sid::enableIndentationOnFirstSystem, false);

    std::vector<track_idx_t> tracks;
    const std::vector<std::vector<Note*> > groups = notesByMeasureAndTrack(score, tracks);

    // 1. The perde SymbTr gives each note: its accidental, else the ♯/♭ of its pitch, else natural.
    //    A grace note with neither is the exception: SymbTr's converter drops them there, and the
    //    grace note has the perde of the same note earlier in the measure, else of the donanım.
    std::map<const Note*, int> want;
    for (const std::vector<Note*>& notes : groups) {
        std::map<int, int> earlier;   // natural pitch -> perde, in this measure and voice
        for (Note* note : notes) {
            int koma = 0;
            const Accidental* acc = note->accidental();
            const int alter = writtenAlter(note);
            const int natPitch = note->pitch() - alter;
            if (acc && (acc->role() == AccidentalRole::USER || Accidental::isMicrotonal(acc->accidentalType()))
                && accidentalKomaShift(acc->accidentalType(), koma)) {
                // the written accidental
            } else if (alter != 0) {
                koma = alter > 0 ? 4 : -5;
            } else if (note->chord()->isGrace()) {
                if (earlier.count(natPitch)) {
                    koma = earlier[natPitch];
                } else {
                    const Staff* staff = note->staff();
                    koma = keySigKomas(staff ? staff->keySigEvent(note->tick()) : KeySigEvent())[letterIndex(note)];
                }
            } else if (acc && accidentalKomaShift(acc->accidentalType(), koma)) {
                // an accidental of the layout (♮ after a ♯/♭ of the measure)
            }
            want[note] = koma;
            earlier[natPitch] = koma;
        }
    }
    for (const std::vector<Note*>& notes : groups) {
        for (Note* note : notes) {
            if (isTieContinuation(note) && want.count(note->firstTiedNote())) {
                want[note] = want[note->firstTiedNote()];
            }
        }
    }

    // 2. Remove the written accidentals; the layout then gives back the ♯/♭/♮ of the pitches
    for (const std::vector<Note*>& notes : groups) {
        for (Note* note : notes) {
            Accidental* acc = note->accidental();
            if (acc && (acc->role() == AccidentalRole::USER || Accidental::isMicrotonal(acc->accidentalType()))) {
                score->undoRemoveElement(acc);
            }
        }
    }
    score->doLayout();

    // 3. Read each measure and voice as the tuner does; write an accidental where the reading
    //    (donanım, an accidental earlier in the measure, the ♯/♭ of the pitch) is not the perde
    for (size_t g = 0; g < groups.size(); ++g) {
        std::map<int, int> carry;   // natural pitch -> koma, until the end of the measure
        for (Note* note : groups[g]) {
            if (isTieContinuation(note)) {
                continue;
            }
            const int alter = writtenAlter(note);
            const int natPitch = note->pitch() - alter;
            const Accidental* acc = note->accidental();
            int read = 0;
            if (acc) {
                if (!accidentalKomaShift(acc->accidentalType(), read)) {
                    read = 0;
                }
            } else if (carry.count(natPitch)) {
                read = carry[natPitch];
            } else if (alter != 0) {
                read = alter > 0 ? 4 : -5;
            } else {
                const Staff* staff = note->staff();
                const std::vector<int> komas = keySigKomas(staff ? staff->keySigEvent(note->tick()) : KeySigEvent());
                const int kk = komas[letterIndex(note)];
                read = (kk == 4 || kk == -5) ? 0 : kk;
            }
            const int target = want[note];
            if (read != target) {
                const AccidentalType type = accidentalOfKoma(target);
                if (type != AccidentalType::NONE) {
                    score->changeAccidental(note, type);
                    carry[natPitch] = target;
                    continue;
                }
            }
            if (acc) {
                carry[natPitch] = read;
            }
        }
    }
    score->doLayout();

    // 4. Check with the tuner's own reading
    const std::map<const Note*, int> read = readKomas(score);
    int wrong = 0;
    for (const auto& [note, koma] : want) {
        const auto it = read.find(note);
        if (it != read.end() && it->second != koma) {
            ++wrong;
        }
    }
    if (wrong > 0) {
        LOGW() << "SymbTr: " << wrong << " of " << want.size() << " notes are not read with their SymbTr perde";
    }

    tuneScore(score);
}
}
