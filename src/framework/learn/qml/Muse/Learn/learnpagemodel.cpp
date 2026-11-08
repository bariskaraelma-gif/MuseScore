/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
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
#include "learnpagemodel.h"

#include <QVariant>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>

#include "translation.h"

using namespace muse::learn;

// Makam: the training tab ("Makam Eğitimi"). Each folder in <app data>/makam-egitim is a section,
// each .mscz / .mscx in it a lesson; folders and files are sorted by name and a leading number
// ("1-", "01_") is not shown. A lesson may have a text file of the same name (the beste2mscx
// source format): its "# title:", "# altbaslik:" (else "# makam:", "# form:", "# usul:") and
// "# notlar:" lines give the card's title and texts. Without it the file name is the title.
static QString makamDisplayName(QString name)
{
    static const QRegularExpression numberPrefix("^\\d+[-_ .]*");
    name.remove(numberPrefix);
    name.replace('_', ' ');
    return name.trimmed();
}

static QVariantMap makamLessonInfo(const QFileInfo& file)
{
    QVariantMap lesson;
    lesson["title"] = makamDisplayName(file.completeBaseName());
    lesson["path"] = file.absoluteFilePath();

    QFile meta(file.absolutePath() + "/" + file.completeBaseName() + ".txt");
    if (!meta.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return lesson;
    }

    static const QRegularExpression line("^#\\s*(\\w+):\\s*(.*)$");
    QMap<QString, QString> tags;
    QString lastKey;
    for (const QString& text : QString::fromUtf8(meta.readAll()).split('\n')) {
        const QString trimmed = text.trimmed();
        if (trimmed.startsWith("m") && !trimmed.startsWith("#")) {
            break; // the measures begin
        }
        const QRegularExpressionMatch m = line.match(trimmed);
        if (m.hasMatch()) {
            lastKey = m.captured(1);
            tags[lastKey] = m.captured(2).trimmed();
        } else if (lastKey == "notlar" && trimmed.startsWith("#") && trimmed.size() > 1) {
            tags[lastKey] += " " + trimmed.mid(1).trimmed(); // continued notes
        } else {
            lastKey.clear();
        }
    }

    if (!tags.value("title").isEmpty()) {
        lesson["title"] = tags.value("title");
    }

    QStringList sub;
    for (const char* key : { "makam", "form", "usul" }) {
        if (!tags.value(key).isEmpty()) {
            sub << tags.value(key);
        }
    }
    lesson["subtitle"] = tags.value("altbaslik", sub.join(" · "));
    lesson["notes"] = tags.value("notlar");
    return lesson;
}

QVariantList LearnPageModel::makamLessons() const
{
    QVariantList sections;
    const QDir root(globalConfiguration()->appDataPath().toQString() + "/makam-egitim");
    const QStringList scoreFilters { "*.mscz", "*.mscx" };

    for (const QFileInfo& dir : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        QVariantList lessons;
        for (const QFileInfo& file : QDir(dir.absoluteFilePath()).entryInfoList(scoreFilters, QDir::Files, QDir::Name)) {
            lessons << makamLessonInfo(file);
        }
        if (lessons.isEmpty()) {
            continue;
        }

        QVariantMap section;
        section["title"] = makamDisplayName(dir.fileName());
        section["lessons"] = lessons;
        sections << section;
    }

    return sections;
}

void LearnPageModel::openMakamLesson(const QString& path, const QString& title)
{
    // NotationProject::load treats a file in makam-egitim as new: saving asks for a place, the
    // lesson itself is never overwritten
    dispatcher()->dispatch("file-open", actions::ActionData::make_arg2<QUrl, QString>(QUrl::fromLocalFile(path), title));
}

LearnPageModel::LearnPageModel(QObject* parent)
    : QObject(parent), Contextable(muse::iocCtxForQmlObject(this))
{
}

QVariantList LearnPageModel::startedPlaylist() const
{
    Playlist filteredPlaylist = filterPlaylistBySearch(m_startedPlaylist);
    return playlistToVariantList(filteredPlaylist);
}

QVariantList LearnPageModel::advancedPlaylist() const
{
    Playlist filteredPlaylist = filterPlaylistBySearch(m_advancedPlaylist);
    return playlistToVariantList(filteredPlaylist);
}

void LearnPageModel::load()
{
    learnService()->refreshPlaylists();

    setStartedPlaylist(learnService()->startedPlaylist());
    learnService()->startedPlaylistChanged().onReceive(this, [this](const Playlist& playlist) {
        setStartedPlaylist(playlist);
    });

    setAdvancedPlaylist(learnService()->advancedPlaylist());
    learnService()->advancedPlaylistChanged().onReceive(this, [this](const Playlist& playlist) {
        setAdvancedPlaylist(playlist);
    });
}

void LearnPageModel::setSearchText(const QString& text)
{
    if (m_searchText == text) {
        return;
    }

    m_searchText = text;

    emit startedPlaylistChanged();
    emit advancedPlaylistChanged();
}

QVariantMap LearnPageModel::classesAuthor() const
{
    QVariantMap author;
    author["name"] = muse::qtrc("learn", "Marc Sabatella");
    author["role"] = muse::qtrc("learn", "Instructor");

    // Rename to "MuseScore Studio" in this description?
    author["position"] = muse::qtrc("learn", "Creator, Mastering MuseScore");
    author["description"] = muse::qtrc("learn", "Welcome to Mastering MuseScore – the most comprehensive resource "
                                                "for learning the world’s most popular music notation software! "
                                                "My name is Marc Sabatella, and I have been helping develop, support, "
                                                "and promote MuseScore since its initial release over ten years ago.\n\n"
                                                "Whether you are just getting started with music notation software, "
                                                "or are a power user eager to explore advanced engraving and playback techniques, "
                                                "my flagship online course Mastering MuseScore "
                                                "covers everything you need to know to get the most out of MuseScore.\n\n"
                                                "In addition, Mastering MuseScore features a supportive community of musicians, "
                                                "with discussion spaces, live streams, "
                                                "and other related courses and services to help you create your best music. "
                                                "Take advantage of this opportunity to learn MuseScore from one of its most recognized experts!\n\n"
                                                "(Note: Mastering MuseScore is available in English only)");
    author["avatarUrl"] = "qrc:/qt/qml/Muse/Learn/resources/marc_sabatella.jpg";
    author["organizationName"] = muse::qtrc("learn", "Mastering MuseScore");
    author["organizationUrl"] = "https://www.masteringmusescore.com/musescore4";

    return author;
}

bool LearnPageModel::classesEnabled()
{
    return learnConfiguration()->classesEnabled();
}

void LearnPageModel::setStartedPlaylist(Playlist startedPlaylist)
{
    if (m_startedPlaylist == startedPlaylist) {
        return;
    }

    m_startedPlaylist = startedPlaylist;
    emit startedPlaylistChanged();
}

void LearnPageModel::setAdvancedPlaylist(Playlist advancedPlaylist)
{
    if (m_advancedPlaylist == advancedPlaylist) {
        return;
    }

    m_advancedPlaylist = advancedPlaylist;
    emit advancedPlaylistChanged();
}

QVariantList LearnPageModel::playlistToVariantList(const Playlist& playlist) const
{
    QVariantList result;

    // h:mm:ss for anything over an hour
    //    m:ss for anything under an hour
    //    0:ss for anything under a minute
    auto formatDuration = [](int durationSecs) {
        int seconds = durationSecs;
        int minutes = seconds / 60;
        seconds -= minutes * 60;
        int hours = minutes / 60;
        minutes -= hours * 60;

        return ((hours > 0)
                ? (QString::number(hours) + ":" + QString::number(minutes).rightJustified(2, '0'))
                : QString::number(minutes)
                ) + ":"
               + QString::number(seconds).rightJustified(2, '0');
    };

    for (const PlaylistItem& item : playlist) {
        QVariantMap itemObj;
        itemObj["title"] = item.title;
        itemObj["author"] = item.author;
        itemObj["url"] = item.url;
        itemObj["thumbnailUrl"] = item.thumbnailUrl;
        itemObj["duration"] = formatDuration(item.durationSecs);

        result << itemObj;
    }

    return result;
}

Playlist LearnPageModel::filterPlaylistBySearch(const Playlist& playlist) const
{
    Playlist result;

    for (const PlaylistItem& playlistItem : playlist) {
        if (playlistItem.title.contains(m_searchText, Qt::CaseInsensitive)
            || playlistItem.author.contains(m_searchText, Qt::CaseInsensitive)) {
            result.push_back(playlistItem);
        }
    }

    return result;
}
