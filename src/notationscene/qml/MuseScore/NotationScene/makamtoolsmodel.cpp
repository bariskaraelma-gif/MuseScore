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
}

MakamToolsModel::MakamToolsModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

bool MakamToolsModel::isFinaleMode()
{
    return muse::settings()->value(INTERFACE_KEY).toString() == FINALE_INTERFACE;
}

void MakamToolsModel::setFinaleMode(bool finale)
{
    muse::settings()->setSharedValue(INTERFACE_KEY, muse::Val(std::string(finale ? FINALE_INTERFACE : "musescore")));
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
    m_finaleMode = isFinaleMode();
    emit finaleModeChanged();

    muse::settings()->valueChanged(INTERFACE_KEY).onReceive(this, [this](const muse::Val& value) {
        const bool finale = value.toString() == FINALE_INTERFACE;
        if (finale != m_finaleMode) {
            m_finaleMode = finale;
            emit finaleModeChanged();
            onFinaleModeChanged(finale);
        }
    });

    globalContext()->currentNotationChanged().onNotify(this, [this]() {
        onNotationChanged();
    });
    onNotationChanged();
}

void MakamToolsModel::onFinaleModeChanged(bool finale)
{
    // the tool bar takes the place of the Palettes panel; the other way round it comes back
    setPalettesPanelOpen(!finale);
    if (!finale) {
        setActiveTool(isNoteInput() ? NOTE_INPUT_TOOL : SELECTION_TOOL);
    }
}

void MakamToolsModel::setPalettesPanelOpen(bool open)
{
    if (uiActionsRegister()->actionState("toggle-palettes").checked != open) {
        dispatcher()->dispatch("toggle-palettes");
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
        setActiveTool(NOTE_INPUT_TOOL);
        emit popupRequested(tool);
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
