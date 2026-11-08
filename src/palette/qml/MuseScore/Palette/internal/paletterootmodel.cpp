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
#include <set>
#include <vector>

#include <QMimeData>

#include "internal/palette.h"
#include "internal/palettecell.h"
#include "internal/palettemodel.h"

#include "engraving/dom/factory.h"
#include "engraving/dom/keysig.h"
#include "engraving/dom/makamdata.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/timesig.h"

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
        { "dynamics", { Palette::Type::Dynamic } },
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

//---------------------------------------------------------
//   Makam: "Oluştur" of the Makam and Usul tools
//---------------------------------------------------------

QString PaletteRootModel::makamAddCell(const QString& tool, mu::engraving::ElementPtr element, const QString& name)
{
    const QVariantList indices = makamToolPalettes(tool);
    PaletteProvider* provider = paletteProvider_property();
    QAbstractItemModel* model = provider ? provider->property("mainPaletteModel").value<QAbstractItemModel*>() : nullptr;
    if (indices.isEmpty() || !model) {
        return "Bu aracın paleti palet panelinde gizli ya da kaldırılmış.";
    }
    const QModelIndex paletteIndex = indices.first().value<QModelIndex>();

    // the same way as an element dropped on the palette: a custom cell, saved with the palettes
    const PaletteCellPtr cell = std::make_shared<PaletteCell>(iocContext(), element, name);
    QMimeData data;
    data.setData(PaletteCell::mimeDataFormat, cell->toMimeData());
    if (!model->dropMimeData(&data, Qt::CopyAction, model->rowCount(paletteIndex), 0, paletteIndex)) {
        return "Palete eklenemedi.";
    }

    // applied to the selected measures at once, as a click on the new cell would
    AbstractPaletteController* controller = provider->property("mainPaletteController").value<AbstractPaletteController*>();
    const int rows = model->rowCount(paletteIndex);
    if (controller && rows > 0) {
        controller->applyPaletteElement(model->index(rows - 1, 0, paletteIndex), Qt::NoModifier);
    }
    return QString();
}

QString PaletteRootModel::makamCreateDonanim(const QString& name, const QVariantList& accidentals)
{
    const QString cellName = name.trimmed();
    if (cellName.isEmpty()) {
        return "Bir ad yazın (ör. makamın adı).";
    }
    if (accidentals.isEmpty()) {
        return "En az bir arıza ekleyin.";
    }

    static const QString LETTERS = "CDEFGAB";
    mu::engraving::makam::Donanim donanim;
    std::set<char> used;
    for (const QVariant& v : accidentals) {
        const QVariantMap m = v.toMap();
        const QString letter = m.value("letter").toString();
        const int koma = m.value("koma").toInt();
        if (letter.size() != 1 || !LETTERS.contains(letter)
            || mu::engraving::makam::komaSym(koma) == mu::engraving::SymId::noSym) {
            return "Geçersiz arıza.";
        }
        const char c = letter.at(0).toLatin1();
        if (!used.insert(c).second) {
            return "Aynı nota iki kez yazılmış.";
        }
        donanim.acc.push_back({ c, koma });
    }

    auto keySig = mu::engraving::Factory::makeKeySig(mu::engraving::gpaletteScore->dummy()->segment());
    keySig->setKeySigEvent(mu::engraving::makam::donanimKeySigEvent(donanim));
    return makamAddCell("makam", keySig, cellName);
}

QString PaletteRootModel::makamCreateUsul(const QString& name, int numerator, int denominator, const QVariantList& darbs)
{
    const QString usulName = name.trimmed();
    if (usulName.isEmpty()) {
        return "Usulün adını yazın.";
    }
    if (numerator < 1 || numerator > 64 || (denominator != 2 && denominator != 4 && denominator != 8 && denominator != 16)) {
        return "Geçersiz ölçü.";
    }
    if (darbs.isEmpty()) {
        return "En az bir darb ekleyin.";
    }

    mu::engraving::makam::Usul usul;
    usul.code = mu::engraving::makam::USUL_CUSTOM_CODE;
    usul.name = usulName.toStdString();
    usul.n = numerator;
    usul.d = denominator;
    int total = 0;
    for (const QVariant& v : darbs) {
        const QVariantMap m = v.toMap();
        const int len = m.value("len").toInt();
        if (len < 1) {
            return "Darbların süresi en az 1 olmalı.";
        }
        usul.darb.push_back({ m.value("dum").toBool(), len });
        total += len;
    }
    if (total != numerator) {
        return QString("Darbların süreleri toplamı %1, ölçü %2/%3: toplam %2 olmalı.").arg(total).arg(numerator).arg(denominator);
    }

    auto timeSig = mu::engraving::Factory::makeTimeSig(mu::engraving::gpaletteScore->dummy()->segment());
    timeSig->setSig(mu::engraving::Fraction(numerator, denominator), mu::engraving::TimeSigType::NORMAL);
    timeSig->setGroups(mu::engraving::makam::usulGroups(usul));
    return makamAddCell("usul", timeSig, usulName + QString(" (%1/%2)").arg(numerator).arg(denominator));
}
