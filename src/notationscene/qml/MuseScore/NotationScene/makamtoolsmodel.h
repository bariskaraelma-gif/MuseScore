/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the Finale-like tool bar on the left of the score (Preferences > Appearance >
 * Arayüz). Each tool opens its palettes in a popup; while the Makam or the Usul tool is on, a
 * click on a measure opens the popup of that tool at once. The interaction of the score is not
 * changed: the tool only listens to the selection. The note input tool shows a column of the
 * AEU accidentals next to the tool bar. (The Palettes panel is closed by NotationPageModel.)
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
#include <QVariantList>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"
#include "notation/inotationconfiguration.h"

namespace mu::notation {
class MakamToolsModel : public QObject, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool finaleMode READ finaleMode NOTIFY finaleModeChanged)
    Q_PROPERTY(QString activeTool READ activeTool NOTIFY activeToolChanged)
    Q_PROPERTY(bool hasNotation READ hasNotation NOTIFY hasNotationChanged)
    Q_PROPERTY(QVariantList accidentals READ accidentals CONSTANT)

    muse::GlobalInject<INotationConfiguration> notationConfiguration;
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };

public:
    explicit MakamToolsModel(QObject* parent = nullptr);

    bool finaleMode() const;
    QString activeTool() const;
    bool hasNotation() const;

    //! the accidental column: { id, title, glyph (music font), column: "natural" | "sharp" | "flat" }
    QVariantList accidentals() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void selectTool(const QString& tool);   // a click on a tool button
    Q_INVOKABLE void applyAccidental(const QString& id);
    Q_INVOKABLE void setPopupOpen(bool open);
    Q_INVOKABLE void showAllPalettes();                  // "Bütün paletler": the Palettes panel

signals:
    void finaleModeChanged();
    void activeToolChanged();
    void hasNotationChanged();
    void popupRequested(const QString& tool);

private:
    void onNotationChanged();
    void onSelectionChanged();
    void setActiveTool(const QString& tool);
    bool isNoteInput() const;
    bool isOpenOnClickTool() const;
    bool currentSelectionKey(const void*& key, size_t& staff) const;

    bool m_finaleMode = false;
    QString m_activeTool = "selection";
    bool m_popupOpen = false;
    const void* m_lastSelectionKey = nullptr;   // the selection that last opened the popup
    size_t m_lastSelectionStaff = 0;
};
}
