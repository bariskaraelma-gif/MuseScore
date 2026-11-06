/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
#include "paletterootmodel.h"

#include <map>
#include <vector>

#include "internal/palette.h"
#include "internal/palettemodel.h"

using namespace mu::palette;

PaletteRootModel::PaletteRootModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

PaletteRootModel::~PaletteRootModel()
{
    PaletteProvider* provider = paletteProvider_property();
    if (provider) {
        provider->setSearching(false);
    }
}

void PaletteRootModel::classBegin()
{
    init();
}

void PaletteRootModel::init()
{
    dispatcher()->reg(this, "palette-search", [this]() {
        emit paletteSearchRequested();
    });
    dispatcher()->reg(this, "apply-current-palette-element", [this]() {
        emit applyCurrentPaletteElementRequested();
    });
}

PaletteProvider* PaletteRootModel::paletteProvider_property() const
{
    return dynamic_cast<PaletteProvider*>(paletteProvider().get());
}

QVariantList PaletteRootModel::makamToolPalettes(const QString& tool) const
{
    static const std::map<QString, std::vector<Palette::Type> > TOOL_PALETTES = {
        { "makam", { Palette::Type::KeySig } },
        { "usul", { Palette::Type::TimeSig } },
        { "barline", { Palette::Type::BarLine } },
        { "layout", { Palette::Type::Layout } },
        { "articulation", { Palette::Type::Articulation } },
        { "ornament", { Palette::Type::Ornament, Palette::Type::Breath } },
        { "dynamics", { Palette::Type::Dynamic } },
        { "tempo", { Palette::Type::Tempo } },
        { "text", { Palette::Type::Text } },
        { "repeat", { Palette::Type::Repeat } },
    };

    QVariantList result;
    PaletteProvider* provider = paletteProvider_property();
    auto it = TOOL_PALETTES.find(tool);
    if (!provider || it == TOOL_PALETTES.end()) {
        return result;
    }

    // the model QML sees (mainPaletteModel() itself is private)
    const QAbstractItemModel* model = provider->property("mainPaletteModel").value<QAbstractItemModel*>();
    if (!model) {
        return result;
    }
    for (Palette::Type type : it->second) {
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, 0);
            if (index.data(PaletteTreeModel::PaletteTypeRole).value<Palette::Type>() == type) {
                result << QVariant::fromValue(index);
                break;
            }
        }
    }
    return result;
}
