/* This file is part of SIR, an open-source cross-platform Image tool
 * 2007-2010  Rafael Sachetto <rsachetto@gmail.com>
 * 2011-2012  Marek Jędryka   <jedryka89@gmail.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Program URL: http://sir.projet-libre.org/
 */

#include "metadatadialog.h"
#include "metadatautils.h"
#include <QLineEdit>
#include <QSettings>
#include <QMessageBox>
#include <exiv2/exif.hpp>

 /** Default constructor.\n
   * Sets up window and reads metadata from file within \b currentImage index
   * into \b images list.
   */
MetadataDialog::MetadataDialog(QWidget *parent, QStringList *images,
                               int currentImage) : QDialog(parent) {
    setupUi(this);
    this->images = images;
    this->currentImage = currentImage;
    this->imagePath = this->images->at(this->currentImage);
    metadata = new MetadataUtils::Metadata;

    // setup Exif orientation combo box
    for (char i=1; i<=8; i++)
        exifOrientationComboBox->addItem(MetadataUtils::Exif::orientationString(i));
    // setup Exif exposure program combo box
    for (uchar i=0; i<=8; i++)
        exifExpProgramComboBox->addItem(MetadataUtils::Exif::expProgramString(i));
    // setup Exif light metering mode combo box
    for (short i=0; i<8; i++)
        exifLightMeteringModeComboBox->addItem(
                    MetadataUtils::Exif::meteringModeString(i));

    iptcCreatedDateEdit->setSpecialValueText(MetadataUtils::String::noData());
    iptcCreatedTimeEdit->setSpecialValueText(MetadataUtils::String::noData());
    iptcDigitizedDateEdit->setSpecialValueText(MetadataUtils::String::noData());
    iptcDigitizedTimeEdit->setSpecialValueText(MetadataUtils::String::noData());

    if (currentImage == 0)
        previousButton->setEnabled(false);
    if (currentImage == images->length()-1)
        nextButton->setEnabled(false);
    filePathLabel->setText(imagePath);

    readFile();
    setupValues();

    connect(previousButton, SIGNAL(clicked()), this, SLOT(previousImage()));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(nextImage()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveChanges()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteMetadata()));
}

/** Destructor.
  * \note Deallocates \b images list typed in \ref MetadataDialog() "constructor".
  */
MetadataDialog::~MetadataDialog() {
    delete metadata;
    delete images;
}

/** Resets metadata structs to default values. */
void MetadataDialog::resetStructs() {
    exifStruct->reset();
}

/** Read current image file and shows information about issues if exist.\n
  * This function will call resetStructs() function if error cought.
  */
void MetadataDialog::readFile() {
    bool readSuccess = metadata->read(imagePath,true);
    exifStruct = metadata->exifStruct();
    iptcStruct = metadata->iptcStruct();
    if (!readSuccess) {
        QString errorTitle = tr("Metadata error");
        MetadataUtils::Error *e = metadata->lastError();
        QString errorMessage = e->message();
        errorMessage += tr("\nError code: %1\nError message: %2").
                arg(e->code()).arg(e->what());
        QMessageBox::critical(this, errorTitle, errorMessage);
        resetStructs();
        tabWidget->setCurrentWidget(iptcTab);
    }
    Exiv2::Image::AutoPtr image = metadata->imageAutoPtr();
    QString message;
    if (image->exifData().empty()) {
        tabWidget->setCurrentWidget(iptcTab);
        message = tr("No Exif metadata.");
    }
    else if (image->iptcData().empty()) {
        tabWidget->setCurrentWidget(exifTab);
        message = tr("No IPTC metadata.");
    }
    if (!message.isNull())
        QMessageBox::warning(this, tr("Metadata warning"), message);
}

void MetadataDialog::setupValues() {
    QSettings settings("SIR");
    settings.beginGroup("Settings");
    int maxHistoryCount = settings.value("maxHistoryCount", 5).toInt();
    settings.endGroup();

    // Exif tab
    settings.beginGroup("Exif");
    // Image toolbox
    exifVersionLabel->setText( exifStruct->version );
    exifProcessingSoftLabel->setText( exifStruct->processingSoftware );
    exifWidthLabel->setText( exifStruct->imageWidth );
    exifHeightLabel->setText( exifStruct->imageHeight );
    exifOrientationComboBox->setCurrentIndex( exifStruct->orientation - 1 );
    exifOriginalDateComboBox->
                importHistory( settings.value("originalDateMap").toMap(),
                             settings.value("originalDateList").toList(),
                             maxHistoryCount );
    exifOriginalDateComboBox->lineEdit()->setText( exifStruct->originalDate );
    exifDigitizedDateComboBox->
            importHistory( settings.value("digitizedDateMap").toMap(),
                         settings.value("digitizedDateList").toList(),
                         maxHistoryCount );
    exifDigitizedDateComboBox->lineEdit()->setText( exifStruct->digitizedDate );

    // Thumbnail toolbox
    exifThumbnailLabel->setPixmap(QPixmap::fromImage(exifStruct->thumbnailImage));
    exifThumbnailWidthLabel->setText( exifStruct->thumbnailWidth );
    exifThumbnailHeightLabel->setText( exifStruct->thumbnailHeight );

    // Photo toolbox
    exifFocalLengthSpinBox->setSpecialValueText(MetadataUtils::String::noData());
    exifFocalLengthSpinBox->setValue( exifStruct->focalLength );
    exifExpTimeComboBox->insertItem(0,MetadataUtils::String::noData());
    if (exifStruct->expTime.isEmpty() || exifStruct->expTime == "0/0 s")
        exifExpTimeComboBox->setCurrentIndex(0);
    else
        exifExpTimeComboBox->addItem( exifStruct->expTime );
    exifShutterTimeComboBox->insertItem(0,MetadataUtils::String::noData());
    if(exifStruct->shutterSpeed.isEmpty())
        exifShutterTimeComboBox->setCurrentIndex(0);
    else
        exifShutterTimeComboBox->addItem(exifStruct->shutterSpeed);
    exifExpBiasSpinBox->setSpecialValueText(MetadataUtils::String::noData());
    exifExpBiasSpinBox->setValue( exifStruct->expBias );
    exifApertureSpinBox->setSpecialValueText(MetadataUtils::String::noData());
    exifApertureSpinBox->setValue( exifStruct->aperture );
    exifIsoSpeedSpinBox->setSpecialValueText(MetadataUtils::String::noData());
    exifIsoSpeedSpinBox->setValue( exifStruct->isoSpeed );
    exifExpProgramComboBox->setCurrentIndex( exifStruct->expProgram );
    exifLightMeteringModeComboBox->setCurrentIndex( exifStruct->meteringMode );
    // setup flashmode combobox
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(-1));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(1));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(7));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(9));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x18));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x19));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x1d));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x1f));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x41));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x45));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x4d));
    exifFlashModeComboBox->addItem(MetadataUtils::Exif::flashString(0x47));
    exifFlashModeComboBox->setCurrentIndex(
                exifFlashModeComboBox->findText( MetadataUtils::Exif::flashString(
                                                     exifStruct->flashMode) ));

    // Camera toolbox
    exifManufacturerComboBox->lineEdit()->setText( exifStruct->cameraManufacturer );
    exifModelComboBox->lineEdit()->setText( exifStruct->cameraModel );

    // Author toolbox
    exifArtistComboBox->importHistory( settings.value("artistMap").toMap(),
                                     settings.value("artistList").toList(),
                                     maxHistoryCount );
    exifArtistComboBox->setCurrentIndex(-1);
    exifArtistComboBox->lineEdit()->setText( exifStruct->artist );
    exifCopyrightComboBox->importHistory( settings.value("copyrightMap").toMap(),
                                        settings.value("copyrightList").toList(),
                                        maxHistoryCount );
    exifCopyrightComboBox->setCurrentIndex(-1);
    exifCopyrightComboBox->lineEdit()->setText( exifStruct->copyright );
    exifUserCommentComboBox->importHistory( settings.value("userCommentMap").toMap(),
                                          settings.value("userCommentList").toList(),
                                          maxHistoryCount );
    exifUserCommentComboBox->setCurrentIndex(-1);
    exifUserCommentComboBox->lineEdit()->setText( exifStruct->userComment );

    settings.endGroup();

    // IPTC tab
    settings.beginGroup("IPTC");
    iptcVersionLabel->setText(iptcStruct->modelVersion);
    iptcCreatedDateEdit->setDate(iptcStruct->dateCreated);
    iptcCreatedTimeEdit->setTime(iptcStruct->timeCreated);
    iptcDigitizedDateEdit->setDate(iptcStruct->digitizationDate);
    iptcDigitizedTimeEdit->setTime(iptcStruct->digitizationTime);
    iptcByLineEdit->setText(iptcStruct->byline);
    iptcCopyrightLineEdit->setText(iptcStruct->copyright);
    iptcObjectNameLineEdit->setText(iptcStruct->objectName);
    iptcKeywordsLineEdit->setText(iptcStruct->keywords);
    iptcDescriptionTextEdit->setPlainText(iptcStruct->caption);
    iptcCountryLineEdit->setText(iptcStruct->countryName);
    iptcCityLineEdit->setText(iptcStruct->city);
    iptcEditStatusComboBox->importHistory(settings.value("editStatusMap").toMap(),
                                          settings.value("editStatusList").toList(),
                                          maxHistoryCount);
    iptcEditStatusComboBox->setCurrentIndex(-1);
    iptcEditStatusComboBox->lineEdit()->setText(iptcStruct->editStatus);
    settings.endGroup();
}

void MetadataDialog::previousImage() {
    currentImage--;
    if (currentImage == 0)
        previousButton->setEnabled(false);
    if (!nextButton->isEnabled() && currentImage < images->length()-1)
        nextButton->setEnabled(true);
    imagePath = images->at(currentImage);
    readFile();
    setupValues();
    filePathLabel->setText(imagePath);
}

void MetadataDialog::nextImage() {
    currentImage++;
    if (currentImage == images->length()-1)
        nextButton->setEnabled(false);
    if (!previousButton->isEnabled())
        previousButton->setEnabled(true);
    imagePath = images->at(currentImage);
    readFile();
    setupValues();
    filePathLabel->setText(imagePath);
}

void MetadataDialog::saveChanges() {
    // Exif tab
    // Image toolbox
    exifStruct->orientation = exifOrientationComboBox->currentIndex();
    exifStruct->originalDate = exifOriginalDateComboBox->lineEdit()->text();
    exifStruct->digitizedDate = exifDigitizedDateComboBox->lineEdit()->text();
    // Photo toolbox
    exifStruct->focalLength = exifFocalLengthSpinBox->value();
    exifStruct->expTime = exifExpTimeComboBox->currentText();
    exifStruct->expBias = exifExpBiasSpinBox->value();
    exifStruct->aperture = exifApertureSpinBox->value();
    exifStruct->shutterSpeed = exifShutterTimeComboBox->currentText();
    exifStruct->isoSpeed = exifIsoSpeedSpinBox->value();
    exifStruct->expProgram = exifExpProgramComboBox->currentIndex();
    exifStruct->meteringMode = exifLightMeteringModeComboBox->currentIndex();
    exifStruct->flashMode = MetadataUtils::Exif::flashShort(
                exifFlashModeComboBox->currentText() );
    // Camera toolbox
    exifStruct->cameraManufacturer = exifManufacturerComboBox->lineEdit()->text();
    exifStruct->cameraModel = exifModelComboBox->lineEdit()->text();
    // Author toolbox
    exifStruct->artist = exifArtistComboBox->lineEdit()->text();
    exifStruct->copyright = exifCopyrightComboBox->lineEdit()->text();
    exifStruct->userComment = exifUserCommentComboBox->lineEdit()->text();

    // IPTC tab
    if (iptcCreatedDateEdit->date() == iptcCreatedDateEdit->minimumDate())
        iptcStruct->dateCreated = QDate();
    else
        iptcStruct->dateCreated = iptcCreatedDateEdit->date();
    if (iptcCreatedTimeEdit->time() == iptcCreatedTimeEdit->minimumTime())
        iptcStruct->timeCreated = QTime();
    else
        iptcStruct->timeCreated = iptcCreatedTimeEdit->time();
    if (iptcDigitizedDateEdit->date() == iptcDigitizedDateEdit->minimumDate())
        iptcStruct->digitizationDate = QDate();
    else
        iptcStruct->digitizationDate = iptcDigitizedDateEdit->date();
    if (iptcDigitizedTimeEdit->time() == iptcDigitizedTimeEdit->minimumTime())
        iptcStruct->digitizationTime = QTime();
    else
        iptcStruct->digitizationTime = iptcDigitizedTimeEdit->time();
    iptcStruct->byline = iptcByLineEdit->text();
    iptcStruct->copyright = iptcCopyrightLineEdit->text();
    iptcStruct->objectName = iptcObjectNameLineEdit->text();
    iptcStruct->keywords = iptcKeywordsLineEdit->text();
    iptcStruct->caption = iptcDescriptionTextEdit->toPlainText();
    iptcStruct->countryName = iptcCountryLineEdit->text();
    iptcStruct->city = iptcCityLineEdit->text();
    iptcStruct->editStatus = iptcEditStatusComboBox->lineEdit()->text();

    // saving
    metadata->setExifData();
    metadata->setIptcData();
    metadata->write(imagePath);
    this->close();
}

void MetadataDialog::deleteMetadata() {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Delete metadata"));
    msgBox.setText(tr("Do you really want to delete metadata from this image?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    if(msgBox.exec() == QMessageBox::Yes) {
        metadata->clearMetadata();
        metadata->write(imagePath);
        resetStructs();
        setupValues();
        this->close();
    }
}