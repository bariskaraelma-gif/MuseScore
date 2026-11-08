/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2022 MuseScore Limited and others
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

#include "loadingscreenview.h"

#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>

#include "translation.h"

using namespace mu::appshell;

static const QString imagePath(":/resources/LoadingScreen.svg");

static constexpr QSize loadingScreenSize(800, 380);

// Makam: the TurkScore loading screen. The background (LoadingScreen.svg: navy, a gold
// geometric rosette, faint staff lines, the gold rule under the subtitle) is drawn first,
// then the texts in the UI font.
static const QColor titleColor("#F3E9D2");
static const QColor accentColor("#C9A45C");

static const QString title("TurkScore");
static const QString subtitle = QString::fromUtf8("Türk makam müziği nota yazım programı");
static const QString basedOn = QString::fromUtf8("MuseScore Studio 4.7.6 tabanlı · GPL-3.0");

static constexpr qreal sideMargin = 56.0;
static constexpr qreal bottomMargin = 48.0;

LoadingScreenView::LoadingScreenView(QWidget* parent)
    : QWidget(parent), muse::Contextable(muse::iocCtxForQWidget(this)),
    m_backgroundRenderer(new QSvgRenderer(imagePath, this))
{
    setAttribute(Qt::WA_TranslucentBackground);
    resize(loadingScreenSize);
}

bool LoadingScreenView::event(QEvent* event)
{
    if (event->type() == QEvent::Paint) {
        QPainter painter(this);
        painter.setLayoutDirection(layoutDirection());
        draw(&painter);
    }

    return QWidget::event(event);
}

void LoadingScreenView::draw(QPainter* painter)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // Draw background
    m_backgroundRenderer->render(painter);

    // Makam: the texts of the TurkScore screen (always left to right, as the design)
    const QString family = QString::fromStdString(uiConfiguration()->fontFamily());
    const qreal w = loadingScreenSize.width();
    const qreal h = loadingScreenSize.height();

    QFont titleFont(family);
    titleFont.setPixelSize(52);
    titleFont.setBold(true);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    painter->setFont(titleFont);
    painter->setPen(titleColor);
    painter->drawText(QRectF(sideMargin, 120.0, 440.0, 70.0), Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip, title);

    QFont subtitleFont(family);
    subtitleFont.setPixelSize(18);
    painter->setFont(subtitleFont);
    painter->setPen(accentColor);
    painter->drawText(QRectF(sideMargin, 190.0, 440.0, 28.0), Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip, subtitle);

    QFont smallFont(family);
    smallFont.setPixelSize(12);
    painter->setFont(smallFont);

    QColor basedOnColor(titleColor);
    basedOnColor.setAlphaF(0.55);
    painter->setPen(basedOnColor);
    painter->drawText(QRectF(sideMargin, h - bottomMargin, 0.0, 0.0), Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip, basedOn);

    painter->setPen(accentColor);
    painter->drawText(QRectF(w - bottomMargin, h - bottomMargin, 0.0, 0.0), Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip,
                      QString::fromUtf8("Sürüm %1").arg(application()->fullVersion().toString()));
}
