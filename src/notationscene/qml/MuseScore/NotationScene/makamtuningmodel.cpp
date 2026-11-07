/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the "Makam akordu" window, see makamtuningmodel.h
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
#include "makamtuningmodel.h"

#include <cmath>

#include "settings.h"
#include "types/translatablestring.h"

#include "engraving/dom/masterscore.h"
#include "engraving/editing/makamdizi.h"
#include "engraving/editing/makamtuner.h"

using namespace mu::notation;

static const muse::Settings::Key MAKAM_UNIT_KEY("makam", "makam/unit");
static constexpr double MAKAM_MANDAL = 1200.0 / 72.0;   // kanun mandal in cents

MakamTuningModel::MakamTuningModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void MakamTuningModel::load()
{
    const muse::Val savedUnit = muse::settings()->value(MAKAM_UNIT_KEY);
    if (!savedUnit.isNull() && savedUnit.toQString() == "cent") {
        m_unit = "cent";
    }

    // the makams of the open score, the first one selected
    QStringList parts;
    QString first;
    const IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (master && master->masterScore()) {
        for (const mu::engraving::makam::ScoreMakam& m : mu::engraving::makam::scoreMakams(master->masterScore())) {
            parts << QString("%1. ölçü %2 (%3)").arg(m.measureNo).arg(m.name, m.source);
            if (first.isEmpty()) {
                first = QString::fromStdString(m.id);
            }
        }
    }
    m_scoreInfo = parts.isEmpty() ? QString("Açık eser yok.") : QString("Bu eserde: ") + parts.join("   ·   ");
    emit scoreInfoChanged();

    setCurrentMakam(first.isEmpty() ? QString("ussak") : first);
    emit valuesChanged();
}

QVariantList MakamTuningModel::makamList() const
{
    QVariantList list;
    for (const mu::engraving::makam::MakamName& m : mu::engraving::makam::allMakams()) {
        QVariantMap item;
        item["text"] = m.name;
        item["value"] = QString::fromStdString(m.id);
        list << item;
    }
    return list;
}

QString MakamTuningModel::currentMakam() const
{
    return m_currentMakam;
}

void MakamTuningModel::setCurrentMakam(const QString& id)
{
    if (m_currentMakam == id) {
        return;
    }
    m_currentMakam = id;
    emit currentMakamChanged();
    emit valuesChanged();
}

QString MakamTuningModel::unit() const
{
    return m_unit;
}

void MakamTuningModel::setUnit(const QString& unit)
{
    if (m_unit == unit) {
        return;
    }
    m_unit = unit;
    muse::settings()->setSharedValue(MAKAM_UNIT_KEY, muse::Val(unit));
    emit valuesChanged();
}

double MakamTuningModel::step() const
{
    return 1.0;
}

int MakamTuningModel::decimals() const
{
    return m_unit == "cent" ? 1 : 0;
}

// Shown from the perde's natural note (e.g. Eviç = Fa +5 mandal); stored from the written note
double MakamTuningModel::toDisplay(double cents, int alter) const
{
    const double fromNatural = cents + alter * 100.0;
    if (m_unit == "cent") {
        return std::round(fromNatural * 10.0) / 10.0;
    }
    return std::round(fromNatural / MAKAM_MANDAL);
}

double MakamTuningModel::fromDisplay(double value, int alter) const
{
    const double fromNatural = m_unit == "cent" ? value : value * MAKAM_MANDAL;
    return std::round((fromNatural - alter * 100.0) * 100.0) / 100.0;
}

QVariantList MakamTuningModel::rows() const
{
    QVariantList list;
    const std::string id = m_currentMakam.toStdString();
    const std::vector<mu::engraving::makam::PerdeValues> defaults = mu::engraving::makam::calibration(id, true);
    const std::vector<mu::engraving::makam::PerdeValues> values = mu::engraving::makam::calibration(id, false);
    for (size_t i = 0; i < values.size(); ++i) {
        const mu::engraving::makam::PerdeValues& v = values[i];
        const int alter = mu::engraving::makam::perdeAlter(v.perde);
        QVariantMap row;
        row["perde"] = QString::fromStdString(v.perde);
        row["label"] = mu::engraving::makam::perdeLabel(v.perde);
        row["up"] = toDisplay(v.up, alter);
        row["down"] = toDisplay(v.down, alter);
        row["stop"] = toDisplay(v.stop, alter);
        if (i < defaults.size()) {
            const mu::engraving::makam::PerdeValues& d = defaults[i];
            row["changed"] = std::abs(d.up - v.up) > 0.01 || std::abs(d.down - v.down) > 0.01 || std::abs(d.stop - v.stop) > 0.01;
        }
        list << row;
    }
    return list;
}

double MakamTuningModel::yeden() const
{
    return toDisplay(mu::engraving::makam::yedenSegah(), 0);
}

QString MakamTuningModel::scoreInfo() const
{
    return m_scoreInfo;
}

void MakamTuningModel::setValue(const QString& perde, const QString& field, double value)
{
    const std::string id = m_currentMakam.toStdString();
    for (mu::engraving::makam::PerdeValues v : mu::engraving::makam::calibration(id, false)) {
        if (v.perde != perde.toStdString()) {
            continue;
        }
        const double cents = fromDisplay(value, mu::engraving::makam::perdeAlter(v.perde));
        if (field == "up") {
            v.up = cents;
        } else if (field == "down") {
            v.down = cents;
        } else {
            v.stop = cents;
        }
        mu::engraving::makam::setCalibration(id, v);
        break;
    }
    // no valuesChanged here: the rows would be rebuilt while the user is typing
    retune();
}

void MakamTuningModel::setYeden(double value)
{
    mu::engraving::makam::setYedenSegah(fromDisplay(value, 0));
    retune();
}

void MakamTuningModel::resetCurrentMakam()
{
    mu::engraving::makam::resetCalibration(m_currentMakam.toStdString());
    emit valuesChanged();
    retune();
}

bool MakamTuningModel::hasSeyir() const
{
    return mu::engraving::makam::hasSeyir(m_currentMakam.toStdString());
}

void MakamTuningModel::listenDizi(bool withSeyir)
{
    dispatcher()->dispatch("makam-dizi", muse::actions::ActionData::make_arg2<QString, bool>(m_currentMakam, withSeyir));
}

// The engine runs at the end of every command: an empty command retunes the open score
void MakamTuningModel::retune()
{
    const IMasterNotationPtr master = globalContext()->currentMasterNotation();
    if (!master) {
        return;
    }
    INotationUndoStackPtr undoStack = master->notation()->undoStack();
    undoStack->prepareChanges(muse::TranslatableString("undoableAction", "Makam akordu"));
    undoStack->commitChanges();
}
