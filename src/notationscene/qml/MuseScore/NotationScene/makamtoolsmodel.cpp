/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: the Finale-like tool bar, see makamtoolsmodel.h
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
#include "makamtoolsmodel.h"

#include <iterator>

#include "settings.h"

#include "engraving/dom/accidental.h"
#include "engraving/dom/makamdata.h"
#include "engraving/dom/score.h"
#include "engraving/dom/select.h"
#include "engraving/dom/staff.h"

#include "notation/inotation.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationelements.h"

using namespace mu::notation;

namespace {
const muse::Settings::Key INTERFACE_KEY("makam", "makam/interface");   // "musescore" or "finale"
const char* FINALE_INTERFACE = "finale";

const char* SELECTION_TOOL = "selection";
const char* NOTE_INPUT_TOOL = "noteinput";

bool isFinaleInterface(const muse::Val& value)
{
    return value.toString() == FINALE_INTERFACE;
}

//! the AEU accidentals as in the Accidentals palette (palettecreator.cpp); the glyphs are those
//! of the accidental types in accidental.cpp (SMuFL code points)
struct MakamAccidental {
    const char* id;
    mu::engraving::AccidentalType type;
    const char* title;
    char16_t glyph;
    const char* column;
    bool bravura = false;   // a THM glyph, drawn with Bravura (thmFontFamily)
};

const MakamAccidental MAKAM_ACCIDENTALS[] = {
    { "natural", mu::engraving::AccidentalType::NATURAL, "Naturel", 0xE261, "natural" },
    { "koma-sharp", mu::engraving::AccidentalType::SHARP_SLASH, "Koma diyez", 0xE282, "sharp" },
    { "bakiye-sharp", mu::engraving::AccidentalType::SHARP, "Bakiye diyez", 0xE262, "sharp" },
    { "kucuk-sharp", mu::engraving::AccidentalType::SHARP_SLASH3, "Küçük mücennep diyez", 0xE446, "sharp" },
    { "buyuk-sharp", mu::engraving::AccidentalType::SHARP_SLASH2, "Büyük mücennep diyez", 0xE447, "sharp" },
    { "koma-flat", mu::engraving::AccidentalType::MIRRORED_FLAT, "Koma bemol", 0xE280, "flat" },
    { "bakiye-flat", mu::engraving::AccidentalType::FLAT_SLASH, "Bakiye bemol", 0xE442, "flat" },
    { "kucuk-flat", mu::engraving::AccidentalType::FLAT, "Küçük mücennep bemol", 0xE260, "flat" },
    { "buyuk-flat", mu::engraving::AccidentalType::FLAT_SLASH2, "Büyük mücennep bemol", 0xE440, "flat" },
};

//! Makam: the THM (TRT) accidentals, offered where the donanım is a THM one (makam::isThmKeySig):
//! sharp/flat with the number of komas; the sharp alone is 4 komas, the flat alone 5 (Bravura E450-E457)
const MakamAccidental THM_ACCIDENTALS[] = {
    { "natural", mu::engraving::AccidentalType::NATURAL, "Naturel", 0xE261, "natural" },
    { "thm-sharp1", mu::engraving::AccidentalType::ONE_COMMA_SHARP, "Diyez 1 (1 koma)", 0xE450, "sharp", true },
    { "thm-sharp2", mu::engraving::AccidentalType::TWO_COMMA_SHARP, "Diyez 2 (2 koma)", 0xE451, "sharp", true },
    { "thm-sharp3", mu::engraving::AccidentalType::THREE_COMMA_SHARP, "Diyez 3 (3 koma)", 0xE452, "sharp", true },
    { "thm-sharp4", mu::engraving::AccidentalType::SHARP, "Diyez (4 koma)", 0xE262, "sharp" },
    { "thm-sharp5", mu::engraving::AccidentalType::FIVE_COMMA_SHARP, "Diyez 5 (5 koma)", 0xE453, "sharp", true },
    { "thm-flat1", mu::engraving::AccidentalType::ONE_COMMA_FLAT, "Bemol 1 (1 koma)", 0xE454, "flat", true },
    { "thm-flat2", mu::engraving::AccidentalType::TWO_COMMA_FLAT, "Bemol 2 (2 koma)", 0xE455, "flat", true },
    { "thm-flat3", mu::engraving::AccidentalType::THREE_COMMA_FLAT, "Bemol 3 (3 koma)", 0xE456, "flat", true },
    { "thm-flat4", mu::engraving::AccidentalType::FOUR_COMMA_FLAT, "Bemol 4 (4 koma)", 0xE457, "flat", true },
    { "thm-flat5", mu::engraving::AccidentalType::FLAT, "Bemol (5 koma)", 0xE260, "flat" },
};

struct AccidentalRange {
    const MakamAccidental* first;
    const MakamAccidental* last;
    const MakamAccidental* begin() const { return first; }
    const MakamAccidental* end() const { return last; }
};

AccidentalRange accidentalsOf(bool thm)
{
    if (thm) {
        return { std::begin(THM_ACCIDENTALS), std::end(THM_ACCIDENTALS) };
    }
    return { std::begin(MAKAM_ACCIDENTALS), std::end(MAKAM_ACCIDENTALS) };
}
}

MakamToolsModel::MakamToolsModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

bool MakamToolsModel::finaleMode() const
{
    return m_finaleMode;
}

QString MakamToolsModel::activeTool() const
{
    return m_activeTool;
}

bool MakamToolsModel::hasNotation() const
{
    return globalContext()->currentNotation() != nullptr;
}

QString MakamToolsModel::thmFontFamily() const
{
    const engraving::IEngravingFontPtr font = engravingFonts() ? engravingFonts()->fallbackFont() : nullptr;
    return font ? QString::fromStdString(font->family()) : QString("Bravura");
}

void MakamToolsModel::load()
{
    m_finaleMode = isFinaleInterface(muse::settings()->value(INTERFACE_KEY));
    emit finaleModeChanged();

    muse::settings()->valueChanged(INTERFACE_KEY).onReceive(this, [this](const muse::Val& value) {
        const bool finale = isFinaleInterface(value);
        if (finale != m_finaleMode) {
            m_finaleMode = finale;
            emit finaleModeChanged();
        }
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        onNotationChanged();
    });
    onNotationChanged();
}

QVariantList MakamToolsModel::accidentals() const
{
    QVariantList result;
    for (const MakamAccidental& a : accidentalsOf(m_thm)) {
        result << QVariantMap {
            { "id", QString::fromLatin1(a.id) },
            { "title", QString::fromUtf8(a.title) },
            { "glyph", QString(QChar(a.glyph)) },
            { "column", QString::fromLatin1(a.column) },
            { "bravura", a.bravura },
        };
    }
    return result;
}

bool MakamToolsModel::thm() const
{
    return m_thm;
}

// Makam: THM accidentals where the donanım is a THM one: at the note input position, else at the
// start of the selection, else at the start of the score
void MakamToolsModel::updateThm()
{
    bool thm = false;
    INotationPtr notation = globalContext()->currentNotation();
    const engraving::Score* score = notation ? notation->elements()->msScore() : nullptr;
    if (score && !score->staves().empty()) {
        engraving::Fraction tick(0, 1);
        engraving::staff_idx_t staffIdx = 0;
        const engraving::Selection& sel = score->selection();
        if (isNoteInput()) {
            tick = score->inputState().tick();
            staffIdx = score->inputState().track() / engraving::VOICES;
        } else if (sel.isRange()) {
            tick = sel.tickStart();
            staffIdx = sel.staffStart();
        } else if (sel.element()) {
            tick = sel.element()->tick();
            staffIdx = sel.element()->staffIdx();
        }
        const engraving::Staff* staff = score->staff(staffIdx);
        if (!staff) {
            staff = score->staves().front();
        }
        thm = engraving::makam::isThmKeySig(staff->keySigEvent(tick));
    }
    if (thm != m_thm) {
        m_thm = thm;
        emit accidentalsChanged();
    }
}

void MakamToolsModel::applyAccidental(const QString& id)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }

    for (const MakamAccidental& a : accidentalsOf(m_thm)) {
        if (id != QLatin1String(a.id)) {
            continue;
        }
        // as the accidental buttons of the note input bar (NotationActionController::toggleAccidental)
        INotationInteractionPtr interaction = notation->interaction();
        INotationNoteInputPtr noteInput = interaction->noteInput();
        if (noteInput->isNoteInputMode() && notationConfiguration()->addAccidentalDotsArticulationsToNextNoteEntered()) {
            noteInput->setAccidental(a.type);
        } else if (!interaction->selection()->isNone()) {
            interaction->toggleAccidentalForSelection(a.type);
        }
        return;
    }
}

void MakamToolsModel::showAllPalettes()
{
    dispatcher()->dispatch("toggle-palettes");
}

void MakamToolsModel::toggleProperties()
{
    // the Properties panel as a floating window (NotationPageModel)
    dispatcher()->dispatch("makam-properties", muse::actions::ActionData::make_arg1<bool>(false));
}

void MakamToolsModel::onNotationChanged()
{
    emit hasNotationChanged();

    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }

    notation->interaction()->noteInput()->stateChanged().onNotify(this, [this]() {
        // note input started or ended elsewhere (N, Esc, the note input bar)
        if (isNoteInput()) {
            setActiveTool(NOTE_INPUT_TOOL);
        } else if (m_activeTool == NOTE_INPUT_TOOL) {
            setActiveTool(SELECTION_TOOL);
        }
        updateThm();
    });

    // Makam: the accidentals follow the donanım where one writes (AEU or THM)
    notation->interaction()->selectionChanged().onNotify(this, [this]() {
        updateThm();
    });
    notation->notationChanged().onNotify(this, [this]() {
        updateThm();
    });

    setActiveTool(isNoteInput() ? NOTE_INPUT_TOOL : SELECTION_TOOL);
    updateThm();
}

bool MakamToolsModel::isNoteInput() const
{
    INotationPtr notation = globalContext()->currentNotation();
    return notation && notation->interaction()->noteInput()->isNoteInputMode();
}

void MakamToolsModel::setActiveTool(const QString& tool)
{
    if (m_activeTool == tool) {
        return;
    }
    m_activeTool = tool;
    emit activeToolChanged();
}

void MakamToolsModel::selectTool(const QString& tool)
{
    if (tool == "lyrics") {
        // lyrics are typed under the selected note: no palette (Properties only by its own button)
        dispatcher()->dispatch("lyrics");
        return;
    }

    // note input on only for its own tool (the others work on the selection)
    if ((tool == NOTE_INPUT_TOOL) != isNoteInput()) {
        dispatcher()->dispatch("note-input");
    }
    setActiveTool(tool);   // the column of the tool beside the tool bar
}
