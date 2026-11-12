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
#include "paletteworkspacesetup.h"

#include <optional>

#include "io/buffer.h"
#include "settings.h"

#include "palettecreator.h"
#include "../palettetypes.h"

#include "engraving/dom/keysig.h"
#include "engraving/dom/makamdata.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/timesig.h"
#include "engraving/rw/xmlreader.h"
#include "engraving/rw/xmlwriter.h"

#include "log.h"

using namespace mu;
using namespace mu::palette;
using namespace muse;
using namespace muse::io;
using namespace muse::workspace;

static const AsciiStringView PALETTE_XML_TAG("PaletteBox");

// Makam: raised when the Makam palettes change, so that palettes saved in the workspace by an
// earlier version are replaced once by the new default palettes
static const Settings::Key MAKAM_PALETTE_VERSION_KEY("palette", "makam/paletteVersion");
static constexpr int MAKAM_PALETTE_VERSION = 9;   // 2: line end in the Layout palette, 3-4: mızrap arrows,
                                                  // 5: Sultaniyegâh in a donanım's name, 6: THM donanıms,
                                                  // 7: order of the THM donanıms, 8: THM mark (THM_KEY_MODE),
                                                  // 9: beams and strokes of the usuls (makamRefreshUsulCells)
// The last version whose THM palette replaces the saved one (it has no cells of the user's own)
static constexpr int MAKAM_THM_PALETTE_VERSION = 8;
// The last version whose palettes replace the saved ones. A later version only renames the
// built-in donanım cells (makamRenameDonanimCells), so the user's own cells ("Oluştur") and
// changes to the palettes stay.
static constexpr int MAKAM_PALETTE_RESET_VERSION = 4;

static bool makamIsThmPalette(const PalettePtr& palette)
{
    return palette->type() == Palette::Type::KeySig && palette->name() == QString::fromUtf8(engraving::makam::THM_PALETTE_NAME);
}

// Makam: the built-in donanım cells of saved palettes get the names of makamdata.h
static void makamRenameDonanimCells(const PaletteTreePtr& tree)
{
    for (const PalettePtr& palette : tree->palettes) {
        if (palette->type() != Palette::Type::KeySig || makamIsThmPalette(palette)) {
            continue;
        }
        for (const PaletteCellPtr& cell : palette->cells()) {
            if (cell->custom || !cell->element || !cell->element->isKeySig()) {
                continue;
            }
            const engraving::KeySigEvent saved = engraving::toKeySig(cell->element.get())->keySigEvent();
            for (const engraving::makam::Donanim& donanim : engraving::makam::donanimlar()) {
                const engraving::KeySigEvent builtIn = engraving::makam::donanimKeySigEvent(donanim);
                if (!builtIn.custom() || !saved.custom() || builtIn.customKeyDefs().size() != saved.customKeyDefs().size()) {
                    continue;
                }
                bool same = true;
                for (size_t i = 0; i < builtIn.customKeyDefs().size(); ++i) {
                    same = same && builtIn.customKeyDefs()[i].degree == saved.customKeyDefs()[i].degree
                           && builtIn.customKeyDefs()[i].sym == saved.customKeyDefs()[i].sym;
                }
                if (same) {
                    cell->name = QString::fromUtf8(donanim.makamlar);
                    break;
                }
            }
        }
    }
}

// Makam: the built-in usul cells of saved palettes get the beams and strokes of makamdata.h
// (an usul is known by the marker in its time signature; the user's own usuls stay as they are)
static void makamRefreshUsulCells(const PaletteTreePtr& tree)
{
    for (const PalettePtr& palette : tree->palettes) {
        for (const PaletteCellPtr& cell : palette->cells()) {
            if (!cell->element || !cell->element->isTimeSig()) {
                continue;
            }
            engraving::TimeSig* ts = engraving::toTimeSig(cell->element.get());
            const std::optional<engraving::makam::Usul> usul = engraving::makam::usulOfGroups(ts->groups(), ts->sig());
            if (usul && usul->code != engraving::makam::USUL_CUSTOM_CODE) {
                ts->setGroups(engraving::makam::usulGroups(*usul));
            }
        }
    }
}

// Makam: palettes saved before the current THM donanıms get the new THM palette in place of the
// old one, or after the donanım palette
static void makamAddThmPalette(const PaletteTreePtr& tree, const muse::modularity::ContextPtr& iocCtx)
{
    std::optional<size_t> at;
    for (size_t i = 0; i < tree->palettes.size(); ++i) {
        if (makamIsThmPalette(tree->palettes[i])) {
            const bool visible = tree->palettes[i]->isVisible();
            const bool expanded = tree->palettes[i]->isExpanded();
            tree->palettes[i] = PaletteCreator(iocCtx).newThmKeySigPalette();
            tree->palettes[i]->setVisible(visible);
            tree->palettes[i]->setExpanded(expanded);
            return;
        }
        if (!at && tree->palettes[i]->type() == Palette::Type::KeySig) {
            at = i + 1;
        }
    }
    tree->insert(at.value_or(0), PaletteCreator(iocCtx).newThmKeySigPalette());
}

PaletteTreePtr PaletteWorkspaceSetup::readPalette(const ByteArray& data, const muse::modularity::ContextPtr& iocCtx)
{
    ByteArray ba = ByteArray::fromRawData(data.constData(), data.size());
    Buffer buf(&ba);
    buf.open(IODevice::ReadOnly);
    mu::engraving::XmlReader reader(&buf);

    while (!reader.atEnd()) {
        reader.readNextStartElement();

        if (reader.name() == PALETTE_XML_TAG) {
            int mscVersion = 0;
            if (reader.hasAttribute("version")) {
                const String version = reader.attribute("version");
                const StringList sl = version.split(u'.');
                mscVersion = sl.size() == 2 ? sl[0].toInt() * 100 + sl[1].toInt() : 0;
            } else {
                // Versioning workspace palette files started in 4.7. All unversioned files should be treated like 4.6 files
                mscVersion = 460;
            }
            engraving::gpaletteScore->setMscVersion(mscVersion);

            PaletteTreePtr tree = std::make_shared<PaletteTree>();
            tree->read(reader, false, iocCtx);
            return tree;
        }
    }

    return nullptr;
}

void PaletteWorkspaceSetup::writePalette(const PaletteTreePtr& tree, QByteArray& data)
{
    Buffer buf;
    buf.open(IODevice::WriteOnly);
    mu::engraving::XmlWriter writer(&buf);
    tree->write(writer, false);
    writer.flush();
    data = buf.data().toQByteArray();
}

void PaletteWorkspaceSetup::setup()
{
    if (!workspacesDataProvider()) {
        return;
    }

    paletteProvider()->setDefaultPaletteTree(PaletteCreator(iocContext()).newDefaultPaletteTree());

    auto saveData = [this]() {
        PaletteTreePtr tree = paletteProvider()->userPaletteTree();

        QByteArray newData;
        writePalette(tree, newData);

        workspacesDataProvider()->setRawData(WS_Palettes, newData);
    };

    paletteProvider()->userPaletteTreeChanged().onNotify(this, saveData);

    auto loadData = [this, saveData]() {
        RetVal<QByteArray> data = workspacesDataProvider()->rawData(WS_Palettes);
        const int makamSavedVersion = settings()->value(MAKAM_PALETTE_VERSION_KEY).toInt();
        const bool makamOutdated = makamSavedVersion < MAKAM_PALETTE_VERSION;
        PaletteTreePtr tree;
        if (data.ret && !data.val.isEmpty() && makamSavedVersion >= MAKAM_PALETTE_RESET_VERSION) {
            LOGD() << "there is palette data in the workspace, we will use it";
            ByteArray ba = ByteArray::fromQByteArrayNoCopy(data.val);
            tree = readPalette(ba, iocContext());
            if (tree && makamOutdated) {
                makamRenameDonanimCells(tree);
                makamRefreshUsulCells(tree);
                if (makamSavedVersion < MAKAM_THM_PALETTE_VERSION) {
                    makamAddThmPalette(tree, iocContext());
                }
            }
        }
        if (!tree) {
            LOGD() << "no palette data in workspace, will use default";
            tree = PaletteCreator(iocContext()).newDefaultPaletteTree();
        }
        paletteProvider()->setUserPaletteTree(tree);

        if (makamOutdated) {
            settings()->setSharedValue(MAKAM_PALETTE_VERSION_KEY, Val(MAKAM_PALETTE_VERSION));
            saveData();
        } else if (engraving::gpaletteScore->mscVersion() < engraving::Constants::MSC_VERSION) {
            LOGD() << "Workspace file found with palette file version " << engraving::gpaletteScore->mscVersion() <<
                ". Migrating palette file to " << engraving::Constants::MSC_VERSION;
            saveData();
        }
    };

    workspacesDataProvider()->workspaceChanged().onNotify(this, loadData);

    loadData();
}
