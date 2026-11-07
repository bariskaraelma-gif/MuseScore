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
#include "projectuiactions.h"

#include "modularity/ioc.h"
#include "types/translatablestring.h"
#include "context/shortcutcontext.h"

using namespace mu::project;
using namespace muse;
using namespace muse::ui;
using namespace muse::actions;

const UiActionList ProjectUiActions::m_actions = {
    UiAction("file-open",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&Open…"),
             TranslatableString("action", "Open"),
             IconCode::Code::OPEN_FILE
             ),
    UiAction("file-new",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&New…"),
             TranslatableString("action", "New"),
             IconCode::Code::NEW_FILE
             ),
    // Makam: blank score with a Türk Müziği staff and an usul staff
    UiAction("file-new-with-usul",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Yeni (çalgı + usul)"),
             TranslatableString("action", "Yeni nota: çalgı dizeği ve usul dizeği"),
             IconCode::Code::NEW_FILE
             ),
    // Makam: blank score with an usul staff only
    UiAction("file-new-usul",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Yeni (yalnız usul)"),
             TranslatableString("action", "Yeni nota: yalnız usul dizeği"),
             IconCode::Code::NEW_FILE
             ),
    // Makam: calibration of the makam tuning (toolbar button)
    UiAction("makam-tuning",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Makam akordu"),
             TranslatableString("action", "Makam akordu: perdelerin çıkış, iniş ve durak değerleri"),
             IconCode::Code::TUNING_FORK
             ),
    // Makam: note entry by voice, Bona (toolbar button)
    UiAction("makam-bona",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Bona"),
             TranslatableString("action", "Bona: do-re-mi hecelerini okuyarak nota yaz"),
             IconCode::Code::MICROPHONE
             ),
    // Makam: the dizi (and a sample seyir) of a makam, tuned (button of the "Makam akordu" window;
    // args: makam id, with the seyir)
    UiAction("makam-dizi",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Diziyi dinle"),
             TranslatableString("action", "Makamın dizisini ve örnek seyirini akortlu çal"),
             IconCode::Code::PLAY
             ),
    // Makam: built-in Image Capture tool (toolbar button)
    UiAction("image-capture",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Görüntü yakala"),
             TranslatableString("action", "Görüntü yakala: notanın bir bölümünü resim olarak kaydet"),
             IconCode::Code::CAMERA
             ),
    UiAction("file-close",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&Close"),
             TranslatableString("action", "Close")
             ),
    UiAction("file-save",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&Save"),
             TranslatableString("action", "Save"),
             IconCode::Code::SAVE
             ),
    UiAction("file-save-as",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Save &as…"),
             TranslatableString("action", "Save as")
             ),
    UiAction("file-save-a-copy",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Save a &copy…"),
             TranslatableString("action", "Save a copy")
             ),
    UiAction("file-save-selection",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Save &selection…"),
             TranslatableString("action", "Save selection")
             ),
    UiAction("file-save-to-cloud",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Save to clo&ud…"),
             TranslatableString("action", "Save to cloud"),
             IconCode::Code::CLOUD_FILE
             ),
    UiAction("file-publish",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Publish to &MuseScore.com…"),
             TranslatableString("action", "Publish to MuseScore.com"),
             IconCode::Code::CLOUD_FILE
             ),
    UiAction("file-share-audio",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Share on &Audio.com…"),
             TranslatableString("action", "Share on Audio.com"),
             IconCode::Code::SHARE_AUDIO
             ),
    UiAction("file-export",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&Export…"),
             TranslatableString("action", "Export"),
             IconCode::Code::SHARE_FILE
             ),
    UiAction("file-convert-to-score",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Convert &file…"),
             TranslatableString("action", "Convert file"),
             IconCode::Code::IMPORT
             ),
    UiAction("project-properties",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "Project propert&ies…"),
             TranslatableString("action", "Project properties")
             ),
    UiAction("print",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&Print…"),
             TranslatableString("action", "Print"),
             IconCode::Code::PRINT
             ),
    UiAction("clear-recent",
             mu::context::UiCtxAny,
             mu::context::CTX_ANY,
             TranslatableString("action", "&Clear list of recent files"),
             TranslatableString("action", "Clear list of recent files")
             )
};

ProjectUiActions::ProjectUiActions(std::shared_ptr<ProjectActionsController> controller, const muse::modularity::ContextPtr& iocCtx)
    : muse::Contextable(iocCtx), m_controller(controller)
{
}

const UiActionList& ProjectUiActions::actionsList() const
{
    return m_actions;
}

bool ProjectUiActions::actionEnabled(const UiAction& act) const
{
    if (!m_controller->canReceiveAction(act.code)) {
        return false;
    }

    return true;
}

bool ProjectUiActions::actionChecked(const UiAction&) const
{
    return false;
}

muse::async::Channel<ActionCodeList> ProjectUiActions::actionEnabledChanged() const
{
    return m_actionEnabledChanged;
}

muse::async::Channel<ActionCodeList> ProjectUiActions::actionCheckedChanged() const
{
    return m_actionCheckedChanged;
}
