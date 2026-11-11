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
#include <QFontDatabase>
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

// Makam: on the faint staff of LoadingScreen.svg (lines y = 262 … 294, 8 px apart) a G clef, the
// Hüzzam donanım (Si koma bemol, Mi bakiye bemol, Fa diyez) and the accidentals of the THM button
// (Si ♭2, Fa ♯3). The music fonts are the program's own (Qt resources of the engraving module);
// Leland has no THM accidentals, they are Bravura's.
static constexpr qreal staffTop = 262.0;          // Fa (top line)
static constexpr qreal staffSpace = 8.0;
static constexpr qreal musicFontSize = 4 * staffSpace;   // SMuFL: the em is 4 staff spaces

static QString musicFontFamily(const QString& resource, const QString& fallback)
{
    const int id = QFontDatabase::addApplicationFont(resource);
    const QStringList families = id >= 0 ? QFontDatabase::applicationFontFamilies(id) : QStringList();
    return families.isEmpty() ? fallback : families.first();
}

// y of a note: steps (half staff spaces) below the top line (Fa 0, Mi 1, Re 2, Do 3, Si 4, La 5, Sol 6)
static qreal noteY(int stepsBelowTop)
{
    return staffTop + stepsBelowTop * staffSpace / 2.0;
}

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

    drawStaffSigns(painter);
}

void LoadingScreenView::drawStaffSigns(QPainter* painter)
{
    static const QString leland = musicFontFamily(":/fonts/leland/Leland.otf", "Leland");
    static const QString bravura = musicFontFamily(":/fonts/bravura/Bravura.otf", "Bravura");

    QFont lelandFont(leland);
    lelandFont.setPixelSize(musicFontSize);
    QFont bravuraFont(bravura);
    bravuraFont.setPixelSize(musicFontSize);

    auto glyph = [painter](const QFont& font, qreal x, qreal y, char16_t code) {
        painter->setFont(font);
        painter->drawText(QPointF(x, y), QString(QChar(code)));
    };

    QColor clefColor(titleColor);
    clefColor.setAlphaF(0.55);
    painter->setPen(clefColor);
    glyph(lelandFont, 62.0, noteY(6), 0xE050);           // G clef on the Sol line

    painter->setPen(accentColor);
    // Hüzzam (AEU): Si koma bemol (ters bemol), Mi bakiye bemol, Fa diyez
    glyph(lelandFont, 96.0, noteY(4), 0xE280);
    glyph(lelandFont, 107.0, noteY(1), 0xE442);
    glyph(lelandFont, 119.0, noteY(0), 0xE262);

    // a bar line, then the THM button's accidentals: Si ♭2, Fa ♯3
    QColor barColor(titleColor);
    barColor.setAlphaF(0.35);
    painter->setPen(QPen(barColor, 1.0));
    painter->drawLine(QPointF(142.5, noteY(0)), QPointF(142.5, noteY(8)));

    painter->setPen(accentColor);
    glyph(bravuraFont, 156.0, noteY(4), 0xE455);
    glyph(bravuraFont, 171.0, noteY(0), 0xE452);
}
