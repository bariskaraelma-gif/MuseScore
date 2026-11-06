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

#include "settings.h"

#include "engraving/dom/accidental.h"
#include "engraving/dom/articulation.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/rootitem.h"
#include "engraving/dom/score.h"

#include "notation/inotation.h"
#include "notation/inotationelements.h"
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"

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
    for (const MakamAccidental& a : MAKAM_ACCIDENTALS) {
        result << QVariantMap {
            { "id", QString::fromLatin1(a.id) },
            { "title", QString::fromUtf8(a.title) },
            { "glyph", QString(QChar(a.glyph)) },
            { "column", QString::fromLatin1(a.column) },
        };
    }
    return result;
}

void MakamToolsModel::applyAccidental(const QString& id)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }

    for (const MakamAccidental& a : MAKAM_ACCIDENTALS) {
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

void MakamToolsModel::addMizrap(bool down)
{
    INotationPtr notation = globalContext()->currentNotation();
    if (!notation || notation->interaction()->selection()->isNone()) {
        return;
    }

    // a strum arrow above the note, as an articulation: one more click on it takes it away
    mu::engraving::Score* score = notation->elements()->msScore();
    std::shared_ptr<mu::engraving::Articulation> arrow = mu::engraving::Factory::makeArticulation(score->dummy()->chord());
    arrow->setSymId(down ? mu::engraving::SymId::guitarStrumDown : mu::engraving::SymId::guitarStrumUp);
    arrow->setAnchor(mu::engraving::ArticulationAnchor::TOP);
    notation->interaction()->applyPaletteElement(arrow.get());
}

void MakamToolsModel::showAllPalettes()
{
    dispatcher()->dispatch("toggle-palettes");
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
    });

    setActiveTool(isNoteInput() ? NOTE_INPUT_TOOL : SELECTION_TOOL);
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
        // lyrics are typed under the selected note: no palette
        dispatcher()->dispatch("lyrics");
        return;
    }

    // note input on only for its own tool (the others work on the selection)
    if ((tool == NOTE_INPUT_TOOL) != isNoteInput()) {
        dispatcher()->dispatch("note-input");
    }
    setActiveTool(tool);

    // the note input, makam and usul tools show their column beside the tool bar; the others
    // open a popup
    if (tool != SELECTION_TOOL && tool != NOTE_INPUT_TOOL && tool != "makam" && tool != "usul") {
        emit popupRequested(tool);
    }
}
