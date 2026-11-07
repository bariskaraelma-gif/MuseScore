/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Makam: model of the "Makam akordu" window (calibration of the makam tuning engine,
 * like the second tab of the Makam plugin).
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

#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"

namespace mu::notation {
class MakamTuningModel : public QObject, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList makamList READ makamList CONSTANT)
    Q_PROPERTY(QString currentMakam READ currentMakam WRITE setCurrentMakam NOTIFY currentMakamChanged)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY valuesChanged)
    Q_PROPERTY(QString unit READ unit WRITE setUnit NOTIFY valuesChanged)
    Q_PROPERTY(double yeden READ yeden NOTIFY valuesChanged)
    Q_PROPERTY(QString scoreInfo READ scoreInfo NOTIFY scoreInfoChanged)
    Q_PROPERTY(bool hasSeyir READ hasSeyir NOTIFY currentMakamChanged)

    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };

public:
    explicit MakamTuningModel(QObject* parent = nullptr);

    QVariantList makamList() const;
    QString currentMakam() const;
    QVariantList rows() const;
    QString unit() const;
    double yeden() const;
    QString scoreInfo() const;
    bool hasSeyir() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE void listenDizi(bool withSeyir);   // the dizi score of the current makam (action "makam-dizi")
    Q_INVOKABLE void setValue(const QString& perde, const QString& field, double value);
    Q_INVOKABLE void setYeden(double value);
    Q_INVOKABLE void resetCurrentMakam();
    Q_INVOKABLE double step() const;
    Q_INVOKABLE int decimals() const;

public slots:
    void setCurrentMakam(const QString& id);
    void setUnit(const QString& unit);

signals:
    void currentMakamChanged();
    void valuesChanged();
    void scoreInfoChanged();

private:
    double toDisplay(double cents, int alter) const;
    double fromDisplay(double value, int alter) const;
    void retune();

    QString m_currentMakam;
    QString m_unit = "mandal";
    QString m_scoreInfo;
};
}
