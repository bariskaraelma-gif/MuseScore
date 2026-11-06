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

#include <QTimer>

#include "settings.h"

#include "engraving/dom/accidental.h"
#include "engraving/dom/engravingitem.h"
#include "engraving/dom/measurebase.h"

#include "notation/inotation.h"
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
            m_lastSelectionKey = nullptr;
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

void MakamToolsModel::showAllPalettes()
{
    dispatcher()->dispatch("toggle-palettes");
}

void MakamToolsModel::onNotationChanged()
{
    emit hasNotationChanged();
    m_lastSelectionKey = nullptr;

    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return;
    }

    notation->interaction()->selectionChanged().onNotify(this, [this]() {
        onSelectionChanged();
    });

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

bool MakamToolsModel::isOpenOnClickTool() const
{
    return m_activeTool == "makam" || m_activeTool == "usul";
}

void MakamToolsModel::setActiveTool(const QString& tool)
{
    if (m_activeTool == tool) {
        return;
    }
    m_activeTool = tool;
    m_lastSelectionKey = nullptr;
    emit activeToolChanged();
}

void MakamToolsModel::selectTool(const QString& tool)
{
    if (tool == SELECTION_TOOL) {
        if (isNoteInput()) {
            dispatcher()->dispatch("note-input");
        }
        setActiveTool(SELECTION_TOOL);
        return;
    }

    if (tool == NOTE_INPUT_TOOL) {
        if (!isNoteInput()) {
            dispatcher()->dispatch("note-input");
        }
        setActiveTool(NOTE_INPUT_TOOL);   // the accidental column, no popup
        return;
    }

    if (tool == "lyrics") {
        // lyrics are typed under the selected note: no palette
        dispatcher()->dispatch("lyrics");
        return;
    }

    if (isNoteInput()) {
        dispatcher()->dispatch("note-input");
    }
    setActiveTool(tool);

    // the popup applies to the present selection; that selection opens no second popup
    currentSelectionKey(m_lastSelectionKey, m_lastSelectionStaff);
    emit popupRequested(tool);
}

bool MakamToolsModel::currentSelectionKey(const void*& key, size_t& staff) const
{
    key = nullptr;
    staff = 0;

    INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return false;
    }
    INotationSelectionPtr selection = notation->interaction()->selection();
    if (selection->isNone()) {
        return false;
    }

    if (selection->isRange()) {
        key = selection->startMeasureBase();
        staff = selection->range()->startStaffIndex();
        return key != nullptr;
    }

    // one element (several single elements: no measure to speak of)
    key = selection->element();
    return key != nullptr;
}

void MakamToolsModel::setPopupOpen(bool open)
{
    if (m_popupOpen == open) {
        return;
    }
    m_popupOpen = open;

    if (!open) {
        // a click on a measure closes the popup first: look at the selection once it is made
        QTimer::singleShot(0, this, [this]() {
            onSelectionChanged();
        });
    }
}

void MakamToolsModel::onSelectionChanged()
{
    if (!m_finaleMode || !isOpenOnClickTool() || m_popupOpen || isNoteInput()) {
        return;
    }

    const void* key = nullptr;
    size_t staff = 0;
    if (!currentSelectionKey(key, staff)) {
        return;
    }
    if (key == m_lastSelectionKey && staff == m_lastSelectionStaff) {
        return;
    }
    m_lastSelectionKey = key;
    m_lastSelectionStaff = staff;

    INotationPtr notation = globalContext()->currentNotation();
    const mu::engraving::EngravingItem* item = notation->interaction()->selection()->element();
    if (item && (item->isKeySig() || item->isTimeSig())) {
        return;   // what a palette cell just put there: no new popup for it
    }
    emit popupRequested(m_activeTool);
}
