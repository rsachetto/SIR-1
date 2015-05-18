/* This file is part of SIR, an open-source cross-platform Image tool
 * 2007-2010  Rafael Sachetto <rsachetto@gmail.com>
 * 2011-2015  Marek Jędryka   <jedryka89@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Program URL: http://marek629.github.io/SIR/
 */

#include "raw/RawController.hpp"

#include "raw/RawModel.hpp"
#include "raw/RawToolbox.hpp"
#include "raw/RawView.hpp"
#include "widgets/MessageBox.hpp"
#include "widgets/options/CommonOptions.hpp"

#include <QFileDialog>


RawController::RawController(RawModel *model, RawView *view, QObject *parent)
    : QObject(parent)
{
    this->model = model;
    this->view = view;
    this->view->setController(this);
}

void RawController::loadSettings()
{
    view->setEnabledChecked(model->isEnabled());

    setRawStatus(model->isEnabled());

    view->setOptionsText(model->getDcrawOptions());

    if (checkDcrawPath(model->getDcrawPath())) {
        setDcrawPath(model->getDcrawPath());
    }
}

void RawController::saveSettings()
{
    bool dcrawOk = false;
    bool firstState = view->isEnabledChecked();

    // check dcraw executable
    if (view->isEnabledChecked()) {
        if ((dcrawOk = checkDcrawPath(view->pathText()))) {
            model->setEnabled(true);
            model->setDcrawPath(view->pathText());
            model->setDcrawOptions(view->optionsText());
        } else {
            view->setEnabledChecked(false);

            model->setEnabled(false);
            model->setDcrawPath(view->pathText());
            model->setDcrawOptions(view->optionsText());

            setRawStatus(false);
        }
    } else {
        model->setEnabled(false);
        model->setDcrawPath(view->pathText());
        model->setDcrawOptions(view->optionsText());
    }

    if (dcrawOk || !firstState) {
        emit ok();
        view->closeWindow();
    }
}

void RawController::browseDcraw()
{
    QString fileName = QFileDialog::getOpenFileName(
                view->qWidget(), tr("Select dcraw executable"),
                CommonOptions::instance()->targetDirPath());

    if (fileName.isEmpty())
        return;

    fileName = QDir::toNativeSeparators(fileName);

    if (checkDcrawPath(fileName)) {
        setDcrawPath(fileName);
    }
}

void RawController::setRawStatus(int state)
{
    view->setPathTextEnabledInput(state);
    view->setButtonEnabledInput(state);
    view->setOptionsTextEnabledInput(state);
}

bool RawController::checkDcrawPath(const QString &fileName)
{
    if (fileName.isEmpty()) {
        QString message = tr("No dcraw executable chosen. "
                             "RAW support will not be enabled!");
        MessageBox::warning(view->qWidget(), "SIR", message);
        return false;
    }

    QFile dcraw(fileName);
    if (dcraw.exists() && dcraw.permissions().testFlag(QFile::ExeOwner)) {
        return true;
    }

    if (dcraw.exists()) {
        QString message = tr("The chosen file is not executable. "
                             "RAW support will not be enabled!");
        MessageBox::warning(view->qWidget(), "SIR", message);
        return false;
    } else {
        QString message = tr("dcraw executable not found. "
                             "RAW support will not be enabled!");
        MessageBox::warning(view->qWidget(), "SIR", message);
        return false;
    }
}

void RawController::setDcrawPath(const QString &fileName)
{
    view->setPathText(fileName);

    RawToolbox toolbox = RawToolbox(model);
    view->setHelpTextPlain(toolbox.helpMessage());
}
