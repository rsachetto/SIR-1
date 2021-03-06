/* This file is part of SIR, an open-source cross-platform Image tool
 * 2007-2010  Rafael Sachetto <rsachetto@gmail.com>
 * 2011-2016  Marek Jędryka   <jedryka89@gmail.com>
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

#include <typeinfo>

#include "widgets/options/MetadataGroupBoxView.hpp"
#include "widgets/options/MetadataGroupBoxController.hpp"

MetadataGroupBoxView::MetadataGroupBoxView(QWidget *parent)
    : AbstractOptionsGroupBox(parent) {
    setupUi(this);

    // validate only ASCII characters
    validator = new QRegExpValidator(QRegExp("^[\\00-\\0177]+"), this);
    exifArtistComboBox->setValidator(validator);
    exifCopyrightComboBox->setValidator(validator);

    connect(metadataCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(metadataCheckBoxChecked(bool)));
    connect(saveMetadataCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(saveMetadataCheckBoxChecked(bool)));
    connect(thumbUpdateCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(thumbUpdateCheckBoxChecked(bool)));
    connect(exifArtistCheckBox, SIGNAL(toggled(bool)),
            exifArtistComboBox, SLOT(setEnabled(bool)));
    connect(exifCopyrightCheckBox, SIGNAL(toggled(bool)),
            exifCopyrightComboBox, SLOT(setEnabled(bool)));
    connect(exifUserCommentCheckBox, SIGNAL(toggled(bool)),
            exifUserCommentComboBox, SLOT(setEnabled(bool)));
}

MetadataGroupBoxView::~MetadataGroupBoxView() {
    delete validator;
}

void MetadataGroupBoxView::loadSettings() {
    controller->loadSettings();
}

void MetadataGroupBoxView::saveSettings() {
    controller->saveSettings();
}

void MetadataGroupBoxView::setController(AbstractOptionsController *controller) {
    Q_ASSERT(typeid(*controller) == typeid(MetadataGroupBoxController));

    this->controller = (MetadataGroupBoxController *)controller;
}

void MetadataGroupBoxView::metadataCheckBoxChecked(bool checked) {
    controller->enableMetadata(checked);
}

void MetadataGroupBoxView::saveMetadataCheckBoxChecked(bool checked) {
    controller->saveMetadata(checked);
}

void MetadataGroupBoxView::thumbUpdateCheckBoxChecked(bool checked) {
    controller->updateThumbnail(checked);
}
