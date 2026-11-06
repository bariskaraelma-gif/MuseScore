/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the Finale-like tool bar on the left of the score (Preferences > Appearance >
 * Arayüz). Each tool opens its palettes in a popup; while the Makam or the Usul tool is on, a
 * click on a measure opens the popup of that tool at once. The interaction of the score is not
 * changed: the tool only listens to the selection.
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

#include <QObject>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"
#include "ui/iuiactionsregister.h"

namespace mu::notation {
class MakamToolsModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool finaleMode READ finaleMode NOTIFY finaleModeChanged)
    Q_PROPERTY(QString activeTool READ activeTool NOTIFY activeToolChanged)
    Q_PROPERTY(bool hasNotation READ hasNotation NOTIFY hasNotationChanged)

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<muse::ui::IUiActionsRegister> uiActionsRegister = { this };

public:
    explicit MakamToolsModel(QObject* parent = nullptr);

    bool finaleMode() const;
    QString activeTool() const;
    bool hasNotation() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void selectTool(const QString& tool);   // a click on a tool button
    Q_INVOKABLE void setPopupOpen(bool open);
    Q_INVOKABLE void showAllPalettes();                  // "Diğer": the Palettes panel

    static bool isFinaleMode();
    static void setFinaleMode(bool finale);   // Preferences

signals:
    void finaleModeChanged();
    void activeToolChanged();
    void hasNotationChanged();
    void popupRequested(const QString& tool);

private:
    void onNotationChanged();
    void onSelectionChanged();
    void onFinaleModeChanged(bool finale);
    void setActiveTool(const QString& tool);
    bool isNoteInput() const;
    bool isOpenOnClickTool() const;
    bool currentSelectionKey(const void*& key, size_t& staff) const;
    void setPalettesPanelOpen(bool open);

    bool m_finaleMode = false;
    QString m_activeTool = "selection";
    bool m_popupOpen = false;
    const void* m_lastSelectionKey = nullptr;   // the selection that last opened the popup
    size_t m_lastSelectionStaff = 0;
};
}
