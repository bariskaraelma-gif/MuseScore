/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the Finale-like tool bar on the left of the score (Preferences > Appearance >
 * Arayüz). Each tool shows a column beside the tool bar: the AEU accidentals for note input,
 * the palette of the tool for the others. The two mızrap buttons put a strum arrow above the
 * selected notes. (The panels on the left are closed, and Properties floats, by NotationPageModel.)
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
    Q_INVOKABLE void addMizrap(bool down);               // the direction of the plectrum
    Q_INVOKABLE void showAllPalettes();                  // "Bütün paletler": the Palettes panel
    Q_INVOKABLE void toggleProperties();                 // the floating Properties window

signals:
    void finaleModeChanged();
    void activeToolChanged();
    void hasNotationChanged();

private:
    void onNotationChanged();
    void setActiveTool(const QString& tool);
    bool isNoteInput() const;

    bool m_finaleMode = false;
    QString m_activeTool = "selection";
};
}
