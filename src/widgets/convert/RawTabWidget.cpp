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

#include "widgets/convert/RawTabWidget.hpp"

#include "Settings.hpp"
#include "raw/AdvancedRawViewWidget.hpp"
#include "raw/BasicRawViewScrollArea.hpp"
#include "raw/RawController.hpp"
#include "raw/RawModel.hpp"


RawTabWidget::RawTabWidget(QWidget *parent) : QTabWidget(parent)
{
    setTabPosition(QTabWidget::South);

    basicTab = new QWidget();
    basicTab->setObjectName("basicTab");
    basicView = new BasicRawViewScrollArea(basicTab);
    QLayout *basicLayout = new QHBoxLayout(basicTab);
    basicLayout->addWidget(basicView->qWidget());

    addTab(basicTab, tr("Basic"));

    advancedTab = new QWidget();
    advancedTab->setObjectName("advancedTab");
    advancedView = new AdvancedRawViewWidget(advancedTab);
    QLayout *advancedLayout = new QHBoxLayout(advancedTab);
    advancedLayout->addWidget(advancedView->qWidget());

    addTab(advancedTab, tr("Advanced"));

    model = new RawModel(Settings::instance()->raw);

    controller = new RawController(model, basicView);
    controller->loadSettings();

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(onTabChange(int)));
}

RawTabWidget::~RawTabWidget()
{
    delete controller;
    delete model;
}

void RawTabWidget::loadSettings(const RawModel &rawModel)
{
    delete model;
    model = new RawModel(rawModel);
    controller->setModel(model);
    controller->loadSettings();
}

RawModel RawTabWidget::rawModel() const
{
    return RawModel(*model);
}

void RawTabWidget::onTabChange(int tabIndex)
{
    Q_UNUSED(tabIndex);

    if (this->currentWidget() == basicTab) {
        model->setDcrawOptions(advancedView->optionsText());
        controller->setView(basicView);
        basicView->setController(controller);
    } else {
        model->setDcrawOptions(basicView->optionsText());
        controller->setView(advancedView);
        advancedView->setController(controller);
    }

    controller->loadSettings();
}