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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Program URL: http://sir.projet-libre.org/
 */

#include <QString>
#include <QDropEvent>
#include <QMenu>
#include <QPicture>
#include <QStringList>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QTextCharFormat>
#include <QCompleter>
#include <QDirModel>
#include <QTranslator>
#include <QLocale>
#include <QMap>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QWindowStateChangeEvent>
#include <QDebug>
#include <cmath>
#include <QSvgRenderer>

#include "convertdialog.h"
#include "previewdialog.h"
#include "myqtreewidget.h"
#include "aboutdialog.h"
#include "optionsdialog.h"
#include "defines.h"
#include "rawutils.h"
#include "networkutils.h"
#include "messagebox.h"
#include "metadatautils.h"
#include "metadatadialog.h"
#include "selection.h"

/** Default constuctor.\n
  * Sets up window with saved settings like window state, position
  * and convertion preferences.
  * \param parent Pointer to parent object.
  * \param args String containing aplication argv tables records
  *     separated by ** (double star-sign).
  * \sa init
  */
ConvertDialog::ConvertDialog(QWidget *parent, QString args) : QMainWindow(parent) {
    setupUi(this);
    this->args = args;
    qtTranslator = new QTranslator(this);
    appTranslator = new QTranslator(this);
    qApp->installTranslator(appTranslator);
    statusList = new QMap<QString,int>();
    net = NULL;
    init();
}

/** Destructor.\n
  * Writes window state and position and dealocates dynamic allocated memory.
  * \sa writeWindowProperties
  */
ConvertDialog::~ConvertDialog() {
    writeWindowProperties();
    delete statusList;
    delete qtTranslator;
    delete appTranslator;
    if(net)
        delete net;
    delete convertAction;
    delete removeAction;
    delete previewAction;
    delete metadataAction;
    delete ConvertThread::shared;
}

/** Connects UI signals to corresponding slots. */
void ConvertDialog::createConnections() {
    // tree view's list menagement buttons
    connect(addFilepushButton, SIGNAL(clicked()), this, SLOT(addFile()));
    connect(addDirpushButton, SIGNAL(clicked()), this, SLOT(addDir()));
    connect(RemovepushButton, SIGNAL(clicked()), this,
            SLOT(removeSelectedFromList()));
    connect(RemoveAllpushButton, SIGNAL(clicked()), this, SLOT(removeAll()));
    // & actions
    connect(actionAdd_File, SIGNAL(triggered()), this, SLOT(addFile()));
    connect(actionAdd_Dir, SIGNAL(triggered()), this, SLOT(addDir()));
    connect(actionSelect, SIGNAL(triggered()), SLOT(showSelectionDialog()));
    connect(actionImport_files, SIGNAL(triggered()), SLOT(showSelectionDialog()));
    connect(actionRemoveAll, SIGNAL(triggered()), SLOT(removeAll()));

    // menu actions
    connect(actionExit, SIGNAL(triggered()), SLOT(close()));
    connect(actionAbout_Qt, SIGNAL(triggered()),qApp, SLOT(aboutQt()));
    connect(actionAbout_Sir, SIGNAL(triggered()), this, SLOT(about()));
    connect(actionOptions, SIGNAL(triggered()), this, SLOT(setOptions()));
    connect(actionCheckforUpdates, SIGNAL(triggered()), SLOT(checkUpdates()));
    connect(actionSendInstall, SIGNAL(triggered()), SLOT(sendInstall()));

    // tree view events
    connect(filesTreeWidget, SIGNAL(customContextMenuRequested (QPoint)),
            SLOT(showMenu(QPoint)));
    connect(filesTreeWidget, SIGNAL(itemDoubleClicked ( QTreeWidgetItem *, int)),
            SLOT(showPreview(QTreeWidgetItem *, int)));
    connect(filesTreeWidget, SIGNAL(changed()), SLOT(updateTree()));
    connect(filesTreeWidget, SIGNAL(itemSelectionChanged()), SLOT(showDetails()));

    // browse button
    connect(browseDestButton, SIGNAL(clicked()), SLOT(browseDestination()));

    // convert... & stop/exit buttons
    connect(convertButton, SIGNAL(clicked()), this, SLOT(convertAll()));
    connect(convertSelectedButton, SIGNAL(clicked()), SLOT(convertSelected()));
    connect(quitButton, SIGNAL(clicked()), SLOT(closeOrCancel()));

    // size tab
    connect(rotateCheckBox,SIGNAL(stateChanged (int)), SLOT(verifyRotate(int)));
    connect(sizeUnitComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setSizeUnit(int)));
    connectSizeLinesEdit();

    // quality spin box & slider
    connect(qualitySpinBox, SIGNAL(valueChanged(int)), qualitySlider, SLOT(setValue(int)));
    connect(qualitySlider, SIGNAL(valueChanged(int)), qualitySpinBox, SLOT(setValue(int)));
}

/** Creates actions and connects their signals to corresponding slots. */
void ConvertDialog::createActions() {
    removeAction = new QAction(tr("Remove Selected"), this);
    removeAction->setStatusTip(tr("Remove selected images"));
    connect(removeAction, SIGNAL(triggered()), this, SLOT(removeSelectedFromList()));

    convertAction = new QAction(tr("Convert Selected"), this);
    convertAction->setStatusTip(tr("Convert selected images"));
    connect(convertAction, SIGNAL(triggered()), this, SLOT(convertSelected()));

    previewAction = new QAction(tr("Show Image"), this);
    previewAction->setStatusTip(tr("Show preview selected image"));
    connect(previewAction, SIGNAL(triggered()), this, SLOT(previewAct()));

    metadataAction = new QAction(tr("Show Metadata"), this);
    metadataAction->setStatusTip(tr("Show metadata of selected image"));
    connect(metadataAction, SIGNAL(triggered()), this, SLOT(showMetadata()));
}

/** Check updates on SIR website.
  * \sa showUpdateResult
  */
void ConvertDialog::checkUpdates() {
    net = new NetworkUtils();
    connect(net, SIGNAL(checkDone(QString*, bool)),
            SLOT(showUpdateResult(QString*, bool)));
    net->checkUpdates();
}

/** Notify SIR installation on project website if alread didn't notified;
  * otherwise show message box about alread sended.
  * \sa showSendInstallResult
  */
void ConvertDialog::sendInstall() {

    if(alreadSent) {
        QMessageBox::information(this, tr("Let us know!"),
                                 tr("You alread sent information about your " \
                                    "SIR installation. Thank you very much!")
                                 );

    }
    else {
        QMessageBox msgBox(QMessageBox::Question,
                           tr("Let us know!"),
                           tr("IMPORTANT: this action will not send any " \
                              "private data about you or your computer to SIR " \
                              "developers! It will only update a counter of " \
                              "unique installations of SIR.")
                           );

        QPushButton *connectButton = msgBox.addButton(tr("Go for it!"),
                                                      QMessageBox::ActionRole);

        QPushButton *abortButton = msgBox.addButton(tr("No thanks!"),
                                                    QMessageBox::ActionRole);

        msgBox.exec();

        if (msgBox.clickedButton() == connectButton) {
            net = new NetworkUtils();
            connect(net, SIGNAL(checkDone(QString*, bool)),
                    SLOT(showSendInstallResult(QString*, bool)));
            net->sendInstalltoSite();

        }
        else if(msgBox.clickedButton() == abortButton) {
            msgBox.close();
        }
    }


}

/** Show message box about notified installation.
  * \sa sendInstall
  */
void ConvertDialog::showSendInstallResult(QString *result, bool error) {

    if(error) {
        QMessageBox::warning(this, tr("Let us know!"),
                             tr("There was an error while trying to " \
                                "connect with SIR website! " \
                                "Check your internet connection and " \
                                "try again later!"));
    }

    else {
        QMessageBox::information(this, tr("Let us know!"),
                                 tr("Thank you for let us know that you are "\
                                    "using SIR! You are the user number %1 of "\
                                    "this month!").arg(*result));

        QSettings settings("SIR");
        settings.beginGroup("Settings");
        settings.setValue("alreadSent", true);
        settings.endGroup();
        alreadSent = true;
    }

}

/** Show result of checkUpdates() in message box.
  * \sa checkUpdates
  */
void ConvertDialog::showUpdateResult(QString *result, bool error) {

    if(error) {
        QMessageBox::warning(this, tr("SIR Updates"),
                             tr("There was an error while trying to " \
                                "connect with SIR update website! " \
                                "Check your internet connection and " \
                                "try again later!"));
    }

    else if(*result == "no_new_version" ) {
        QMessageBox::information(this, tr("SIR Updates"),
                                 tr("You have the lastest version of SIR!"));
    }
    else if(*result == "new_version" ) {

        QMessageBox msgBox(QMessageBox::Information,
                           tr("SIR Updates"),
                           tr("There is a new version of SIR available for "\
                              "download!")
                           );

        QPushButton *connectButton = msgBox.addButton(tr("Go to SIR website!"),
                                                      QMessageBox::ActionRole);

        QPushButton *abortButton = msgBox.addButton(tr("I will check later!"),
                                                    QMessageBox::ActionRole);

        msgBox.exec();

        if (msgBox.clickedButton() == connectButton) {
            // Open sir website in the default browser
            QUrl sirUrl("http://sir.projet-libre.org/static.php?page=download");
            QDesktopServices::openUrl(sirUrl);
        }
        else if(msgBox.clickedButton() == abortButton) {
            msgBox.close();
        }
    }
}

/** Setups \a numThreads worker threads. */
void ConvertDialog::setupThreads(int numThreads) {
    // clearing list of threads
    while (!convertThreads.isEmpty())
        delete convertThreads.takeFirst();

    for(int i = 0; i < numThreads; i++) {
        convertThreads.append(new ConvertThread(this, i));
    }

    for (int i = 0; i < numThreads; i++) {
        connect(convertThreads[i],
                SIGNAL(question(const QString &, int, const QString&)),this,
                SLOT(query(const QString &, int, const QString&)),
                Qt::QueuedConnection);

        connect(convertThreads[i],
                SIGNAL(imageStatus(const QStringList &, const QString &, int)),
                SLOT(setImageStatus(const QStringList &, const QString &, int)),
                Qt::QueuedConnection);

        connect(convertThreads[i], SIGNAL(getNextImage(int)),this,
                SLOT(giveNextImage(int)), Qt::QueuedConnection);
    }
}

/** Creates lists of write and read supported images including raw images,
  * restore saved settings in last session, setups completers for lines edit
  * and creates connections and actions.
  * \sa readSettings createConnections createActions
  */
void ConvertDialog::init() {

    QList<QByteArray> imageFormats = QImageWriter::supportedImageFormats();
    QStringList list;
    // target format combo box setup
    foreach (QByteArray format, imageFormats)
        list.append(QString(format));
    targetFormatComboBox->insertItems(0,list);
    // read file filters setup
    imageFormats = QImageReader::supportedImageFormats();
    foreach (QByteArray format, imageFormats) {
        QString str(format);
        if (!list.contains(str))
            list.append(str);
    }
    fileFilters = "*.";
    fileFilters.append(list.join(" *.").toLower());
    fileFilters.append(" *.jpg");
    fileFilters.append(" *.JPG");
    fileFilters.append(" *.JPEG");
    fileFilters.append(" *.Jpg");
    fileFilters.append(" *.Jpeg");

    RawUtils::createRawFilesList(rawFormats);
    readSettings();

    if (!args.isEmpty()) {
        argsList = args.split("**");
        initList();
    }

    QCompleter *completer = new QCompleter(this);
    QDirModel *dir = new QDirModel(completer);
    dir->setFilter(QDir::AllDirs);
    completer->setModel(dir);
    destFileEdit->setCompleter(completer);

    QStringList wordList;
    wordList << "web" << "thumb" << "thumbnail" << "resized" << "new";
    QCompleter *completer2 = new QCompleter(wordList, this);
    completer2->setCaseSensitivity(Qt::CaseInsensitive);
    destPrefixEdit->setCompleter(completer2);
    destSuffixEdit->setCompleter(completer2);

    verticalSplitter->setStretchFactor(0,1000);
    horizontalSplitter->setStretchFactor(0,1000);

    convertProgressBar->setValue(0);
    createConnections();
    createActions();

    converting = false;
}

/** Browse destination directory button slot.
  * Choose destination directory in QFileDialog.
  * \par
  * Path from neighboring line edit is setting as origin directory in QFileDialog.
  * If the path is empty origin directory will be set to home path.
  */
void ConvertDialog::browseDestination() {

    QString initialName = destFileEdit->text();

    if (initialName.isEmpty())
        initialName = QDir::homePath();

    QString fileName = QFileDialog::getExistingDirectory(
                           this,
                           tr("Choose a directory"),
                           initialName,
                           QFileDialog::ShowDirsOnly );

    fileName = QDir::convertSeparators(fileName);

    if (!fileName.isEmpty()) {
        destFileEdit->setText(fileName);
    }
}

/** Gives next image for worker thread legitimazed \a tid thread ID. */
void ConvertDialog::giveNextImage(int threadNum) {

    QTreeWidgetItem *item = NULL;

    if(convertedImages < numImages) {
        item = itemsToConvert[convertedImages];
        convertThreads[threadNum]->convertImage(item->text(0), item->text(1),
                                                item->text(2));
        convertThreads[threadNum]->confirmImage();
        convertedImages++;
    }
    else {
        convertThreads[threadNum]->setAcceptWork(false);
        convertThreads[threadNum]->confirmImage();
    }
}

/** Remove all button and action slot.
  * Remove all items of tree widget.
  * \sa removeSelectedFromList
  */
void ConvertDialog::removeAll() {

    if (filesTreeWidget->topLevelItemCount() > 0) {
        filesTreeWidget->clear();
    }

    enableConvertButtons(false);
    convertProgressBar->reset();
    statusList->clear();

}

/** Remove selected button and action slot.
  * Remove selected items of tree widget.
  * \sa removeAll
  */
void ConvertDialog::removeSelectedFromList() {

    QTreeWidgetItem *item;
    QString fileName;

    for (int i = 0; i < filesTreeWidget->topLevelItemCount(); i++)
    {

        if ((filesTreeWidget->topLevelItem(i))->isSelected()) {
            item = filesTreeWidget->takeTopLevelItem(i);

            fileName = item->text(2) + QDir::separator() +item->text(0) + ".";
            fileName += item->text(1);

            statusList->remove(fileName);
        }

    }

    if (filesTreeWidget->topLevelItemCount() == 0) {
        convertButton->setEnabled(FALSE);
        convertSelectedButton->setEnabled(FALSE);
    }
}

/** Add file button and action slot.
  * Load selected image files into tree widget and set state to
  * \em "Not \em converted \em yet".\n
  * This function remember last opened directory in the same session.
  * Default directory is home directory.
  * \sa addDir loadFiles(const QStringList&)
  */
void ConvertDialog::addFile() {
    QString aux = tr("Images") + "(" + fileFilters + ")";

    QStringList files = QFileDialog::getOpenFileNames(
                            this,
                            tr("Select one or more files to open"),
                            lastDir,
                            aux
                        );
    if (files.isEmpty())
        return;
    aux = files.first();
    if (!aux.isEmpty()) {
        lastDir = aux.left(aux.lastIndexOf(QDir::separator()));
        loadFiles(files);
    }
}

/** Adds directory button and action slot.
  * Load all supported image files from choosed directory (non-recursive)
  * into tree widget and set state to \em "Not converted yet".\n
  * This function remember last opened directory in the same session.
  * Default directory is home directory.
  * \sa addFile
  */
void ConvertDialog::addDir() {
    QString dirPath = QFileDialog::getExistingDirectory(
                       this,
                       tr("Choose a directory"),
                       lastDir,
                       QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    dirPath = QDir::convertSeparators(dirPath);
    if (dirPath.isEmpty())
        return;
    lastDir = dirPath;
    QDir sourceFolder(dirPath,fileFilters);
    sourceFolder.setFilter( QDir::Files | QDir::NoSymLinks);
    QList<QFileInfo> list = sourceFolder.entryInfoList();
    loadFiles(list);
}

/** Loads files into tree widget.
  * \param files Full paths list.
  */
void ConvertDialog::loadFiles(const QStringList &files) {
    QStringList::const_iterator it = files.begin();
    QTreeWidgetItem *item;
    QString fileName;

    while ( it != files.end() ) {
        QList<QString> itemList;
        fileName = QDir::convertSeparators(*it);
        itemList.append(QFileInfo(fileName).completeBaseName());
        itemList.append(QFileInfo(fileName).suffix());
        itemList.append(QFileInfo(fileName).path());
        itemList.append(tr("Not converted yet"));
        statusList->insert(QFileInfo(fileName).absoluteFilePath(),
                           NOTCONVERTED);

        item = new QTreeWidgetItem(itemList);
        filesTreeWidget->addTopLevelItem(item);
        ++it;
        lastDir = QFileInfo(fileName).path();
    }
    enableConvertButtons();
    resizeColumnsToContents(filesTreeWidget);
}

/** Loads files into tree widget.
  * \param files File infos list.
  */
void ConvertDialog::loadFiles(const QList<QFileInfo> &files) {
    QListIterator<QFileInfo> it(files);
    QFileInfo fi;
    QTreeWidgetItem *item;

    while ( it.hasNext() ) {
        fi = it.next();
        QList<QString> itemList;
        itemList.append(fi.completeBaseName());
        itemList.append(fi.suffix());
        itemList.append(fi.path());
        itemList.append(tr("Not converted yet"));
        item = new QTreeWidgetItem(itemList);
        filesTreeWidget->addTopLevelItem(item);
        statusList->insert(fi.absoluteFilePath(), NOTCONVERTED);
    }
    if (!files.isEmpty()) {
        enableConvertButtons();
        resizeColumnsToContents(filesTreeWidget);
    }
}

/** Enables convertion push buttons if \a enable is true; otherwise disables it. */
void ConvertDialog::enableConvertButtons(bool enable) {
    convertButton->setEnabled(enable);
    convertSelectedButton->setEnabled(enable);
}

/** Resizes all columns of \a tree to their contents. */
void ConvertDialog::resizeColumnsToContents(myQTreeWidget *tree) {
    for (int i=0; i<tree->columnCount(); i++)
        tree->resizeColumnToContents(i);
}

/** Convert all button slot.
  * Load all items from tree widget into list of images to convert and
  * call #convert() function.
  * \sa convertSelected
  */
void ConvertDialog::convertAll() {
    itemsToConvert.clear();
    for (int i=0; i<filesTreeWidget->topLevelItemCount(); i++)
        itemsToConvert.append( filesTreeWidget->topLevelItem(i) );
    if (itemsToConvert.isEmpty()) {
        QMessageBox::warning(this, "SIR",
                             tr("Please add at least one image file." ));
        return;
    }
    convert();
}

/** Convert selected button slot.
  * Load selected items from tree widget into list of images to convert and
  * call #convert() function.
  * \sa convertAll
  */
void ConvertDialog::convertSelected() {
    itemsToConvert = filesTreeWidget->selectedItems();
    if (itemsToConvert.isEmpty()) {
        QMessageBox::warning( this, "SIR",
                              tr("Please select at least one image file." ));
        return;
    }
    convert();
}

/** Reset answers, setups and starts convertion threads.
  * \sa resetAnswers ConvertThread
  */
void ConvertDialog::convert() {
    resetAnswers();
    bool hasWidth = false;
    bool hasHeight = false;

    int w = 0;
    int h = 0;

    QDir destFolder(destFileEdit->text());

    if (!widthLineEdit->text().isEmpty()) {
        w = widthLineEdit->text().toInt();
        hasWidth = true;
    }
    if (!heightLineEdit->text().isEmpty()) {
        h = heightLineEdit->text().toInt();
        hasHeight = true;
    }

    if (!destFolder.exists()) {
        switch ( QMessageBox::question(this, tr("Create Folder? -- SIR"),
                                       tr("The folder %1 do not exists."
                                          "Do you want to create it?").arg(
                                           destFolder.absolutePath() ),
                                       tr("&Yes"), tr("&No") , 0, 1 ) ) {

            case 0:
                if (!destFolder.mkdir(destFolder.absolutePath())) {
                    QMessageBox::warning(this, "SIR",
                                         tr("Unable to create target folder.") );
                    return;
                }
            break;
            case 1:
            return;
        }
    }

    QTreeWidgetItem * item;

    numImages = itemsToConvert.count();
    convertedImages = 0;
    int nt = numThreads;

    quitButton->setText(tr("Cancel"));
    converting = true;
    this->setCursor(Qt::WaitCursor);

    if(numImages < nt) {
        nt = numImages;
    }

    //Start the worker threads
    setupThreads(nt);

    convertProgressBar->setRange(0,itemsToConvert.count());
    convertProgressBar->setValue(0);

    enableConvertButtons(false);

    if (sizeUnitComboBox->currentIndex() == 2) {
        int multiplier = 1024;
        if (fileSizeComboBox->currentIndex() == 1)
            multiplier *= 1024;
        ConvertThread::setDesiredSize( fileSizeSpinBox->value() * multiplier );
    }
    else
        ConvertThread::setDesiredSize( w, h, (sizeUnitComboBox->currentIndex() == 1),
                                       hasWidth, hasHeight,
                                       maintainCheckBox->isChecked() );
    QString desiredFormat = targetFormatComboBox->currentText().toLower();
    ConvertThread::setDesiredFormat( desiredFormat );
    ConvertThread::setDesiredFlip(flipComboBox->currentIndex());
    ConvertThread::setDesiredRotation( rotateCheckBox->isChecked(),
                                           rotateLineEdit->text().toDouble() );
    ConvertThread::setQuality(qualitySpinBox->value());
    ConvertThread::setDestPrefix(destPrefixEdit->text());
    ConvertThread::setDestSuffix(destSuffixEdit->text());
    ConvertThread::setDestFolder(destFolder);
    ConvertThread::setOverwriteAll(false);

    //Gives a image to each thread convert
    for(int i = 0; i < nt; i++) {
        convertThreads[i]->setAcceptWork( true );
        item = itemsToConvert[convertedImages];
        convertThreads[i]->convertImage(item->text(0), item->text(1),
                                        item->text(2));
        convertedImages++;
    }
}

/** Show preview dialog.
  * Show preview dialog containig selected \a item image.\n
  * This slot is called when tree widgets \a item was double clicked.
  * \param item Pointer to selected tree widgets item.
  * \param col Is ignored, exists for signal-slot compatibility only.
  * \sa showMenu previewAct
  */
void ConvertDialog::showPreview(QTreeWidgetItem *item, int col) {

    Q_UNUSED(col);

    QString imagePath = makeImagePath(item);
    QStringList *list = makeList();
    int index = list->indexOf(imagePath);

    previewForm = new PreviewDialog(this, list, index);
    previewForm->show();
}

/** Metadata action slot.
  * Shows metadata dialog containg selected tree widgets item image metadata.
  * \sa showMenu showPreview
  */
void ConvertDialog::showMetadata() {
    QString imagePath = makeImagePath(treeMenuItem);
    QStringList *list = makeList();
    int index = list->indexOf(imagePath);

    metadataForm = new MetadataDialog(this, list, index);
    metadataForm->show();
}

void ConvertDialog::showSelectionDialog() {
    Selection selection(this);
    QAction *action = static_cast<QAction*> (sender());
    if (action == actionSelect)
        qDebug() << "Selected items:" << selection.selectItems();
    else if (actionImport_files)
        qDebug() << "Imported files:" << selection.importFiles();
}

/** Shows file details or few files summary.\n
  * This function is called when selection items changed in tree view list.\n
  * When details widget this function will do nothing.
  */
void ConvertDialog::showDetails() {
    if (horizontalSplitter->widget(1)->width() == 0)
        return;
    QList<QTreeWidgetItem*> selectedFiles = filesTreeWidget->selectedItems();
    static const QString htmlOrigin = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
            "\"http://www.w3.org/TR/REC-html40/strict.dtd\">"
            "<html><head><meta name=\"qrichtext\" content=\"1\" />"
            "<style type=\"text/css\">p, li { white-space: pre-wrap; }</style>"
            "</head><body style=\" font-family:'Sans Serif';"
            "font-size:9pt; font-weight:400; font-style:normal;\">";
    QString htmlContent;
    static const QString htmlEnd = "</body></html>";
    static const QString htmlBr = "<br />";
    static const QString htmlHr = "<hr />";
    detailsBrowser->clear();
    QSize imageSize;
    bool isSvg = false;
    bool metadataEnabled = MetadataUtils::Metadata::isEnabled();
    MetadataUtils::Metadata metadata;
    MetadataUtils::ExifStruct *exifStruct = 0;
    MetadataUtils::IptcStruct *iptcStruct = 0;
    int usableWidth = detailsBrowser->width() - 12;
    if (selectedFiles.length() == 1) {
        QTreeWidgetItem *item = selectedFiles.first();
        QString ext = item->text(1);
        MetadataUtils::String imagePath = item->text(2) + QDir::separator() +
                item->text(0) + '.' + ext;
        QString thumbPath = QDir::tempPath() + QDir::separator() + "sir_thumb";
        ext = ext.toUpper();
        // thumbnail generation
        if (ext != "SVG" && ext != "SVGZ") {
            bool fromData(!MetadataUtils::Metadata::isEnabled());
            if (!fromData) {
                metadata.read(imagePath, true);
                exifStruct = metadata.exifStruct();
                iptcStruct = metadata.iptcStruct();
                Exiv2::Image::AutoPtr image = metadata.imageAutoPtr();
                imageSize = QSize(image->pixelWidth(), image->pixelHeight());
                Exiv2::PreviewManager previewManager (*image);
                Exiv2::PreviewPropertiesList previewList = previewManager.
                        getPreviewProperties();
                if (!previewList.empty()) { // read from metadata thumnail
                    Exiv2::PreviewImage preview = previewManager.getPreviewImage(
                                previewList[0]);
                    preview.writeFile(thumbPath.toStdString());
                    thumbPath += preview.extension().c_str();
                }
                else
                    fromData = true;
            }
            if (fromData) { // generate from image data
                QImage img(imagePath);
                if (!imageSize.isValid())
                    imageSize = img.size();
                thumbPath += ".tif";
                QImage thumbnail;
                if (img.width() > usableWidth)
                    thumbnail = img.scaledToWidth(usableWidth,
                                                  Qt::SmoothTransformation);
                else
                    thumbnail = img;
                thumbnail.save(thumbPath, "TIFF");
            }
        }
        else { // render from SVG file
            isSvg = true;
            metadataEnabled = false;
            QGraphicsSvgItem svg(imagePath);
            QSvgRenderer *renderer = svg.renderer();
            QSize size = renderer->defaultSize();
            imageSize = size;
            double sizeRatio = (double) usableWidth / size.width();
            size *= sizeRatio;
            QImage thumbnail (size, QImage::Format_ARGB32);
            thumbnail.fill(Qt::transparent);
            QPainter painter (&thumbnail);
            renderer->render(&painter);
            thumbPath += ".tif";
            thumbnail.save(thumbPath, "TIFF");
        }
        htmlContent = "<center><img src=\"" + thumbPath + "\" /></center>" + htmlBr;
        htmlContent += imagePath + htmlBr;
        if (isSvg)
            htmlContent += tr("Default image size: ");
        else
            htmlContent += tr("Image size: ");
        htmlContent += QString::number(imageSize.width()) + "x"
                + QString::number(imageSize.height()) + " px" + htmlBr;
        QFileInfo info(imagePath);
        htmlContent += tr("File size: ") + QString::number(
                    info.size() / pow(1024.,fileSizeComboBox->currentIndex()+1.), 'f', 2)
                + " " + fileSizeComboBox->currentText() + htmlBr;
        if (metadataEnabled) {
            if (exifStruct->version != MetadataUtils::String::noData()) {
                if (exifPhoto != 0 || exifImage != 0 || exifAuthor != 0 || exifCamera != 0)
                    htmlContent += htmlBr;
                // exif image
                if (exifImage & DetailsOptions::ExifVersion)
                    htmlContent += tr("Exif Version") + ": " + exifStruct->version
                            + htmlBr;
                if (exifImage & DetailsOptions::ProcessingSoftware)
                    htmlContent += tr("Processing Software") + ": " +
                            exifStruct->processingSoftware + htmlBr;
                if (exifImage & DetailsOptions::Orientation)
                    htmlContent += tr("Orientation") + ": " +
                            MetadataUtils::Exif::orientationString(
                                exifStruct->orientation) + htmlBr;
                if (exifImage & DetailsOptions::GeneratedDateAndTime)
                    htmlContent += tr("Generated Date and Time") + ": " +
                            exifStruct->originalDate + htmlBr;
                if (exifImage & DetailsOptions::DigitizedDateAndTime)
                    htmlContent += tr("Digitized Date and Time") + ": " +
                            exifStruct->digitizedDate + htmlBr;
                // exif photo
                if (exifPhoto & DetailsOptions::FocalLenght)
                    htmlContent += tr("Focal lenght") + ": " +
                            QString::number(exifStruct->focalLength,'f',1) + " mm"
                            + htmlBr;
                if (exifPhoto & DetailsOptions::Aperture)
                    htmlContent += tr("Aperture") + ": F" +
                            QString::number(exifStruct->aperture,'f',1) + htmlBr;
                if (exifPhoto & DetailsOptions::ExposureTime)
                    htmlContent += tr("Exposure time") + ": " + exifStruct->expTime
                            + htmlBr;
                if (exifPhoto & DetailsOptions::ShutterSpeed)
                    htmlContent += tr("Shutter Speed") + ": " +
                            exifStruct->shutterSpeed + htmlBr;
                if (exifPhoto & DetailsOptions::ExposureBias)
                    htmlContent += tr("Exposure bias") + ": " +
                            QString::number(exifStruct->expBias,'f',1) + "EV" + htmlBr;
                if (exifPhoto & DetailsOptions::IsoSpeed)
                    htmlContent += tr("ISO Speed") + ": " +
                            QString::number(exifStruct->isoSpeed) + htmlBr;
                if (exifPhoto & DetailsOptions::ExposureProgram)
                    htmlContent += tr("Exposure program") + ": " +
                            MetadataUtils::Exif::expProgramString(
                                exifStruct->expProgram) + htmlBr;
                if (exifPhoto & DetailsOptions::LightMeteringMode)
                    htmlContent += tr("Light metering mode") + ": " +
                            MetadataUtils::Exif::meteringModeString(
                                exifStruct->meteringMode) + htmlBr;
                if (exifPhoto & DetailsOptions::FlashMode)
                    htmlContent += tr("Flash mode") + ": " +
                            MetadataUtils::Exif::flashString(exifStruct->flashMode)
                            + htmlBr;
                // exif camera
                if (exifCamera & DetailsOptions::Manufacturer)
                    htmlContent += tr("Camera manufacturer: ") +
                            exifStruct->cameraManufacturer + htmlBr;
                if (exifCamera & DetailsOptions::Model)
                    htmlContent += tr("Camera model: ") + exifStruct->cameraModel
                            + htmlBr;
                // exif author
                if (exifAuthor & DetailsOptions::Artist)
                    htmlContent += tr("Artist") + ": " + exifStruct->artist + htmlBr;
                if (exifAuthor & DetailsOptions::Copyright)
                    htmlContent += tr("Copyright") + ": " + exifStruct->copyright
                            + htmlBr;
                if (exifAuthor & DetailsOptions::UserComment)
                    htmlContent += tr("User Comment") + ": " + exifStruct->userComment
                            + htmlBr;
            }
            if (MetadataUtils::Iptc::isVersionKnown()) {
                if (iptcPrint & DetailsOptions::ModelVersion)
                    htmlContent += tr("Model version") + ": " +
                            iptcStruct->modelVersion + htmlBr;
                if (iptcPrint & DetailsOptions::DateCreated)
                    htmlContent += tr("Created date") + ": " +
                            iptcStruct->dateCreated.toString(Qt::LocalDate) + htmlBr;
                if (iptcPrint & DetailsOptions::TimeCreated)
                    htmlContent += tr("Created time") + ": " +
                            iptcStruct->timeCreated.toString(Qt::LocalDate) + htmlBr;
                if (iptcPrint & DetailsOptions::DigitizedDate)
                    htmlContent += tr("Digitized date") + ": " +
                            iptcStruct->digitizationDate.toString(Qt::LocalDate)
                            + htmlBr;
                if (iptcPrint & DetailsOptions::DigitizedTime)
                    htmlContent += tr("Digitized time") + ": " +
                            iptcStruct->digitizationTime.toString(Qt::LocalDate)
                            + htmlBr;
                if (iptcPrint & DetailsOptions::Byline)
                    htmlContent += tr("Author") + ": " + iptcStruct->byline + htmlBr;
                if (iptcPrint & DetailsOptions::CopyrightIptc)
                    htmlContent += tr("Copyright") + ": " + iptcStruct->copyright
                            + htmlBr;
                if (iptcPrint & DetailsOptions::ObjectName)
                    htmlContent += tr("Object name") + ": " + iptcStruct->objectName
                            + htmlBr;
                if (iptcPrint & DetailsOptions::Keywords)
                    htmlContent += tr("Keywords") + ": " + iptcStruct->keywords
                            + htmlBr;
                if (iptcPrint & DetailsOptions::Caption)
                    htmlContent += tr("Description") + ": " + iptcStruct->caption
                            + htmlBr;
                if (iptcPrint & DetailsOptions::CountryName)
                    htmlContent += tr("Country") + ": " + iptcStruct->countryName
                            + htmlBr;
                if (iptcPrint & DetailsOptions::City)
                    htmlContent += tr("City") + ": " + iptcStruct->city + htmlBr;
                if (iptcPrint & DetailsOptions::EditStatus)
                    htmlContent += tr("Edit status") + ": " + iptcStruct->editStatus
                            + htmlBr;
            }
        }
    }
    else if (selectedFiles.length() <= 0) {
        detailsBrowser->setText(tr("Select image to show this one details."));
        return;
    }
    else { // many files summary
        QTreeWidgetItem *lastItem = selectedFiles.last();
        QString lastItemPath = lastItem->text(2) + QDir::separator() +
                lastItem->text(0) + '.' + lastItem->text(1);
        if (usableWidth > 180)
            usableWidth = 180;
        int i = 0;
        foreach (QTreeWidgetItem *item, selectedFiles) {
            QString ext = item->text(1);
            MetadataUtils::String imagePath = item->text(2) + QDir::separator() +
                    item->text(0) + '.' + ext;
            QString thumbPath = QDir::tempPath() + QDir::separator() + "sir_thumb_"
                    + QString::number(i);
            ext = ext.toUpper();
            // thumbnail generation
            if (ext != "SVG" && ext != "SVGZ") {
                bool fromData(!MetadataUtils::Metadata::isEnabled());
                isSvg = false;
                if (!fromData) {
                    metadata.read(imagePath, true);
                    exifStruct = metadata.exifStruct();
                    iptcStruct = metadata.iptcStruct();
                    Exiv2::Image::AutoPtr image = metadata.imageAutoPtr();
                    imageSize = QSize(image->pixelWidth(), image->pixelHeight());
                    Exiv2::PreviewManager previewManager (*image);
                    Exiv2::PreviewPropertiesList previewList = previewManager.
                            getPreviewProperties();
                    if (!previewList.empty()) { // read from metadata thumnail
                        Exiv2::PreviewImage preview = previewManager.getPreviewImage(
                                    previewList[0]);
                        preview.writeFile(thumbPath.toStdString());
                        thumbPath += preview.extension().c_str();
                    }
                    else
                        fromData = true;
                }
                if (fromData) { // generate from image data
                    QImage img(imagePath);
                    if (!imageSize.isValid())
                        imageSize = img.size();
                    thumbPath += ".tif";
                    QImage thumbnail;
                    if (img.width() > usableWidth)
                        thumbnail = img.scaledToWidth(usableWidth,
                                                      Qt::SmoothTransformation);
                    else
                        thumbnail = img;
                    thumbnail.save(thumbPath, "TIFF");
                }
            }
            else { // render from SVG file
                isSvg = true;
                metadataEnabled = false;
                QGraphicsSvgItem svg(imagePath);
                QSvgRenderer *renderer = svg.renderer();
                QSize size = renderer->defaultSize();
                imageSize = size;
                double sizeRatio = (double) usableWidth / size.width();
                size *= sizeRatio;
                QImage thumbnail (size, QImage::Format_ARGB32);
                thumbnail.fill(Qt::transparent);
                QPainter painter (&thumbnail);
                renderer->render(&painter);
                thumbPath += ".tif";
                thumbnail.save(thumbPath, "TIFF");
            }
            htmlContent += "<center><img src=\"" + thumbPath + "\" /></center>" + htmlBr;
            htmlContent += imagePath + htmlBr;
            if (isSvg)
                htmlContent += tr("Default image size: ");
            else
                htmlContent += tr("Image size: ");
            htmlContent += QString::number(imageSize.width()) + "x"
                    + QString::number(imageSize.height()) + " px" + htmlBr;
            QFileInfo info(imagePath);
            htmlContent += tr("File size: ") + QString::number(
                        info.size() / pow(1024.,fileSizeComboBox->currentIndex()+1.), 'f', 2)
                    + " " + fileSizeComboBox->currentText();
            if (imagePath != lastItemPath)
                htmlContent += htmlHr;
            i++;
        }
    }
    detailsBrowser->setHtml(htmlOrigin + htmlContent + htmlEnd);
}

/** Loads files into tree view from main() functions \a argv argument list. */
void ConvertDialog::initList() {

    QStringList::Iterator it2 = argsList.begin();
    QString fileName;
    QTreeWidgetItem *item;

    for ( ; it2 != argsList.end(); ++it2 ) {
        fileName = *it2;
        fileName = QDir::convertSeparators(fileName);

        if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
            //Directory
            if (QFileInfo(fileName).isDir()) {
                QDir sourceFolder(fileName,fileFilters);
                sourceFolder.setFilter( QDir::Files | QDir::NoSymLinks);

                QList<QFileInfo> list = sourceFolder.entryInfoList();
                QListIterator<QFileInfo> it(list);
                QFileInfo fi;

                while ( it.hasNext() ) {
                    fi = it.next();
                    QList<QString> itemList;
                    itemList.append(fi.completeBaseName());
                    itemList.append(fi.suffix());
                    itemList.append(fi.path());
                    itemList.append(tr("Not converted yet"));
                    item = new QTreeWidgetItem(itemList);
                    filesTreeWidget->addTopLevelItem(item);
                    statusList->insert(fi.absoluteFilePath(), NOTCONVERTED);
                }
            }
            //File
            else {

                int comp = QString::compare("",QFileInfo(fileName).suffix());

                if((fileFilters.contains(QFileInfo(fileName).suffix()))
                    && (comp !=0)) {

                    QList<QString> itemList;
                    itemList.append(QFileInfo(fileName).completeBaseName());
                    itemList.append(QFileInfo(fileName).suffix());
                    itemList.append(QFileInfo(fileName).path());
                    itemList.append(tr("Not converted yet"));
                    item = new QTreeWidgetItem(itemList);
                    filesTreeWidget->addTopLevelItem(item);
                    statusList->insert(QFileInfo(fileName).absoluteFilePath(),
                                       NOTCONVERTED);
                }
            }
        }
    }

    if (filesTreeWidget->topLevelItemCount() > 0) {
        convertButton->setEnabled(TRUE);
        convertSelectedButton->setEnabled(TRUE);
    }
    resizeColumnsToContents(filesTreeWidget);
}

/** Rotate checkbox slot.
  * Disables/enables rotation angle line edit.
  * \param status Status of the checkbox.
  */
void ConvertDialog::verifyRotate(int status) {

    if (status == Qt::Checked ) {
        rotateLineEdit->setEnabled(TRUE);
    }
    else {
        rotateLineEdit->setEnabled(FALSE);
    }

}

/** Shows context menu.
  * Shows context menu for selected tree widgets item.
  * \param point Global position of context menu.
  * \sa showPreview showMetadata convertSelected removeSelectedFromList
  */
void ConvertDialog::showMenu(const QPoint & point) {

    treeMenuItem = filesTreeWidget->itemAt(point);

    if (treeMenuItem) {
        QMenu contextMenu(this);
        contextMenu.addAction(previewAction);
        contextMenu.addAction(metadataAction);
        contextMenu.addSeparator();
        contextMenu.addAction(convertAction);
        contextMenu.addAction(removeAction);
        contextMenu.setDefaultAction(previewAction);
        contextMenu.exec(QCursor::pos());
    }
}

/** previewAction slot. \sa showPreview */
void ConvertDialog::previewAct()
{
    showPreview(treeMenuItem, 0);
}

/** Shows window containing information about SIR. */
void ConvertDialog::about() {
    AboutDialog *about = new AboutDialog();
    about->setVersion(tr("Version") + " " + VERSION);
    about->exec();
    delete about;
}

/** Writes window state and position and show options dialog. */
void ConvertDialog::setOptions() {
    writeWindowProperties();
    OptionsDialog *options = new OptionsDialog(this);
    connect( options, SIGNAL( ok() ), SLOT( readSettings() ) );
    options->exec();
    delete options;
}

/** Reads settings and sets up window state, position and convertion preferences. */
void ConvertDialog::readSettings() {

    QString locale = QLocale::system().name();

    QString defaultLanguage = "sir_"+locale+".qm";

    QSettings settings("SIR");
    settings.beginGroup("MainWindow");
    if (settings.value("cores",-1).toInt() != -1) {
        //Old format settings from SIR 2.1 - need migration to SIR 2.2 format
        QFile configFile(settings.fileName());
        if (configFile.open(QIODevice::ReadWrite)) {
            QString fileContent = configFile.read(configFile.size());
            fileContent.replace("[MainWindow]","[Settings]");
            configFile.resize(fileContent.size());
            if (configFile.seek(0))
                configFile.write(fileContent.toAscii());
            else
                qWarning("ConvertDialog: settings migration failed");
            configFile.close();
        }
    }
    else {
        this->move( settings.value("possition",this->pos()).toPoint() );
        this->resize( settings.value("size",this->size()).toSize() );
        if (settings.value("maximized",false).toBool())
            this->showMaximized();
        horizontalSplitter->restoreState(settings.value("horizontalSplitter").toByteArray());
        verticalSplitter->restoreState(settings.value("verticalSplitter").toByteArray());
    }
    settings.endGroup(); // MainWindow

    // Old format settings from SIR 2.2
    QString testStr = "Errare humanum est.";
    if (settings.value("Settings/width", testStr).toString() != testStr) {
        // migrate settings from SIR 2.2 to 2.3
        bool sent = settings.value("MainWindow/alreadSent",false).toBool();
        settings.remove("MainWindow/alreadSent");
        settings.setValue("Settings/alreadSent",sent);
        sizeWidthString = settings.value("Settings/width", "800").toString();
        settings.remove("Settings/width");
        settings.setValue("Size/widthPx",sizeWidthString);
        sizeHeightString = settings.value("Settings/height", "600").toString();
        settings.remove("Settings/height");
        settings.setValue("Size/heightPx",sizeHeightString);
        bool metadata = settings.value("Settings/metadata",true).toBool();
        settings.remove("Settings/metadata");
        settings.setValue("Metadata/metadata",metadata);
        bool saveMetadata = settings.value("Settings/saveMetadata",true).toBool();
        settings.remove("Settings/saveMetadata");
        settings.setValue("Metadata/saveMetadata",saveMetadata);
        rawEnabled = settings.value("Settings/raw", false).toBool();
        settings.remove("Settings/raw");
        settings.setValue("Raw/raw",rawEnabled);
    }
    // migrate settings from SIR 2.3 to 2.4
    settings.beginGroup("Exif");
    if (!settings.value("originalDateMap").toMap().isEmpty()) {
        settings.remove("originalDateMap");
        settings.remove("originalDateList");
        settings.remove("digitizedDateMap");
        settings.remove("digitizedDateList");
    }
    settings.endGroup(); // Exif

    settings.beginGroup("Settings");

    destFileEdit->setText(settings.value("targetFolder",
                                         QDir::homePath()).toString());

    targetFormatComboBox->setCurrentIndex(settings.value("targetFormat",
                                                         0).toInt());

    destPrefixEdit->setText(settings.value("targetPrefix", "web").toString());
    destSuffixEdit->setText(settings.value("targetSuffix", "thumb").toString());
    int quality = settings.value("quality", 100).toInt();
    qualitySpinBox->setValue(quality);
    qualitySlider->setValue(quality);

    numThreads = settings.value("cores", 0).toInt();
    if (numThreads == 0)
        numThreads = OptionsDialog::detectCoresCount();
    lastDir = settings.value("lastDir", QDir::homePath()).toString();

    QString selectedTranslationFile = ":/translations/";
    selectedTranslationFile += settings.value("languageFileName",
                                              defaultLanguage).toString();
    QString qtTranslationFile = "qt_" +
            selectedTranslationFile.split('_').at(1).split('.').first();

    qtTranslator->load(qtTranslationFile, QT_TRANSLATIONS_DIR);
    appTranslator->load(selectedTranslationFile);

    alreadSent = settings.value("alreadSent",false).toBool();

    retranslateStrings();
    settings.endGroup(); // Settings

    settings.beginGroup("Size");
    int sizeUnitIndex = settings.value("sizeUnit", 0).toInt();
    setSizeUnit(sizeUnitIndex);
    sizeUnitComboBox->setCurrentIndex(sizeUnitIndex);
    fileSizeSpinBox->setValue(settings.value("fileSizeValue", 300.).toDouble());
    fileSizeComboBox->setCurrentIndex(settings.value("fileSizeUnit", 0).toInt());
    if (sizeUnitComboBox->currentIndex() == 1) {
        sizeWidthString = settings.value("widthPx", "800").toString();
        sizeHeightString = settings.value("heightPx", "600").toString();
        widthLineEdit->setText(sizeWidthString);
        widthLineEdit->setText(settings.value("widthPercent", "100").toString());
        heightLineEdit->setText(sizeHeightString);
        heightLineEdit->setText(settings.value("heightPercent", "100").toString());
    }
    else if (sizeUnitComboBox->currentIndex() == 0) {
        sizeWidthString = settings.value("widthPercent", "100").toString();
        sizeHeightString = settings.value("heightPercent", "100").toString();
        widthLineEdit->setText(sizeWidthString);
        widthLineEdit->setText(settings.value("widthPx", "800").toString());
        heightLineEdit->setText(sizeHeightString);
        heightLineEdit->setText(settings.value("heightPx", "600").toString());
    }
    settings.endGroup(); // Size

    settings.beginGroup("Raw");
    rawEnabled = settings.value("raw", false).toBool();
    if(rawEnabled) {
        foreach(QString ext, rawFormats) {
            if(!fileFilters.contains(ext)) {
                fileFilters.append(ext);
            }
        }
    }
    else {
        foreach(QString ext, rawFormats) {
            if(fileFilters.contains(ext) && ext != " *.tif") {
                fileFilters.remove(ext);
            }
        }
    }
    settings.endGroup(); // Raw

    settings.beginGroup("Metadata");
    bool metadataEnabled = settings.value("metadata",true).toBool();
    MetadataUtils::Metadata::setEnabled(metadataEnabled);
    ConvertThread::setMetadataEnabled(metadataEnabled);
    bool saveMetadata = settings.value("saveMetadata",true).toBool();
    MetadataUtils::Metadata::setSave(saveMetadata);
    ConvertThread::setSaveMetadata(saveMetadata);
    if (saveMetadata) {
        ConvertThread::setRealRotate(settings.value("realRotate",false).toBool());
        ConvertThread::setUpdateThumbnail(
                    settings.value("updateThumbnail",true).toBool() );
        ConvertThread::setRotateThumbnail(
                    settings.value("rotateThumbnail",false).toBool() );
    }
    else
        ConvertThread::setRealRotate(true);
    settings.endGroup(); // Metadata

    settings.beginGroup("Exif");
    bool exifOverwrite;

    exifOverwrite = settings.value("artistOverwrite",false).toBool();
    MetadataUtils::Exif::setArtistOverwrite(exifOverwrite);
    if (exifOverwrite)
        MetadataUtils::Exif::setArtistString( MetadataUtils::String(
                    settings.value("artistMap").toMap().keys().first() ) );

    exifOverwrite = settings.value("copyrightOverwrite",false).toBool();
    MetadataUtils::Exif::setCopyrightOverwrite(exifOverwrite);
    if (exifOverwrite)
        MetadataUtils::Exif::setCopyrightString( MetadataUtils::String(
                    settings.value("copyrightMap").toMap().keys().first() ) );

    exifOverwrite = settings.value("userCommentOverwrite",false).toBool();
    MetadataUtils::Exif::setUserCommentOverwrite(exifOverwrite);
    if (exifOverwrite)
        MetadataUtils::Exif::setUserCommentString( MetadataUtils::String(
                    settings.value("userCommentMap").toMap().keys().first() ) );

    settings.endGroup(); // Exif

    if (metadataEnabled) {
        settings.beginGroup("Details");
        exifAuthor = settings.value("exifAuthor",0x1).toInt();
        exifCamera = settings.value("exifCamera",0x2).toInt();
        exifPhoto = settings.value("exifPhoto",0x1f).toInt();
        exifImage = settings.value("exifImage",0x14).toInt();
        iptcPrint = settings.value("iptc",0xd00).toInt();
        settings.endGroup(); // Details
    }
    else {
        exifAuthor = 0;
        exifCamera = 0;
        exifPhoto = 0;
        exifImage = 0;
        iptcPrint = 0;
    }
}

/** Save new window state and size in private fields.
  * \par
  * This is overloaded QWidget's method.
  */
void ConvertDialog::changeEvent(QEvent *e) {

    if (e->type()!=QEvent::WindowStateChange)
        return;

    QWindowStateChangeEvent *event = (QWindowStateChangeEvent*)(e);
    if (event->oldState()==Qt::WindowNoState) {
        windowSize = this->size();
        windowPossition = this->pos();
    }
}

/** Creates (dynamic allocated) and returns pointer to new string list.
  * In the list is stored files path to images on tree view.
  * \sa makeImagePath
  */
QStringList * ConvertDialog::makeList() {
    int count = filesTreeWidget->topLevelItemCount();
    QStringList *list = new QStringList();
    for (int i=0; i<count; i++)
        list->append(makeImagePath(filesTreeWidget->topLevelItem(i)));
    return list;
}

/** Returns image file path corresponding to \b item.
  * \sa makeList
  */
QString ConvertDialog::makeImagePath(QTreeWidgetItem *item) {
    QString imagePath = item->text(2);
    if (!item->text(2).endsWith("/"))
        imagePath += QDir::separator();
    imagePath += item->text(0) + "." + item->text(1);
    return QDir::fromNativeSeparators(imagePath);
}

/** Updates tree widget when it will change. */
void ConvertDialog::updateTree() {
    if (filesTreeWidget->topLevelItemCount() > 0) {
        enableConvertButtons(true);
    }
    resizeColumnsToContents(filesTreeWidget);
}

/** Set converting status of image.
  * \param imageData List of strings containing path, image name and file extension.
  * \param status Status message.
  * \param statusNum Status code defined in src/defines.h.
  */
void ConvertDialog::setImageStatus(const QStringList& imageData,
                                   const QString& status, int statusNum) {

    if(statusNum != CONVERTING) {
        //We don't want to update the status bar if the statusValue is
        //CONVERTING
        convertProgressBar->setValue(convertProgressBar->value()+1);
    }
    int count = filesTreeWidget->topLevelItemCount();
    QString fileName;

    for (int i = 0; i < count; i++)
    {
        QTreeWidgetItem *item = filesTreeWidget->topLevelItem(i);
        if(item->text(0) == imageData.at(0) && item->text(1) == imageData.at(1)
            && item->text(2) == imageData.at(2)) {
            item->setText(3, status);
            fileName = item->text(2) + QDir::separator() +item->text(0) + ".";
            fileName += item->text(1);
            statusList->insert(fileName,statusNum);
            break;
        }
    }

    if(convertProgressBar->value() == convertProgressBar->maximum()) {
        updateInterface();
    }
}

/** Ask for users agreement of typed action on file.
  * \param targetFile Asking file path.
  * \param tid Worker thread ID.
  * \param whatToDo Action on target file. Support for \em overwrite and \em enlarge only.
  */
void ConvertDialog::query(const QString& targetFile, int tid, const QString& whatToDo) {

    QueryData data = {targetFile,tid};
    QString what = whatToDo.toLower();
    if (what == "overwrite")
        overwriteQueue.enqueue(data);
    else if (what == "enlarge")
        enlargeQueue.enqueue(data);
    else {
        qWarning("ConvertDialog::query(): bad \"whatToDo\" argument");
        convertThreads[tid]->confirmEnlarge(1);
        convertThreads[tid]->confirmOverwrite(1);
        return;
    }
    static bool flag = true;
    static int id = -1;
    if (id == -1)
        id = tid;
    if (tid==id && flag) {
        flag = false;
        while (!enlargeQueue.isEmpty())
            questionEnlarge(enlargeQueue.dequeue());
        while (!overwriteQueue.isEmpty())
            questionOverwrite(overwriteQueue.dequeue());
        id = -1;
        flag = true;
    }
}

/** Asks the user for agree to overwrite image corresponding to \b data. */
void ConvertDialog::questionOverwrite(QueryData data) {

    if (ConvertThread::shared->noOverwriteAll)
        convertThreads[data.tid]->confirmOverwrite(3);
    else if(ConvertThread::shared->abort)
        convertThreads[data.tid]->confirmOverwrite(4);
    else if(!ConvertThread::shared->overwriteAll) {

        int result = MessageBox::question(
                         this,
                         tr("Overwrite File? -- SIR"),
                         tr("A file called %1 already exists."
                            "Do you want to overwrite it?").arg( data.filePath) );

        if (result == MessageBox::YesToAll)
            ConvertThread::shared->overwriteAll = true;
        else if (result == MessageBox::NoToAll)
            ConvertThread::shared->noOverwriteAll = true;
        else if (result == MessageBox::Cancel)
            ConvertThread::shared->abort = true;

        convertThreads[data.tid]->confirmOverwrite(result);
    }
    else
        convertThreads[data.tid]->confirmOverwrite(2);
}

/** Asks the user for agree to enlarge image corresponding to \b data. */
void ConvertDialog::questionEnlarge(QueryData data) {

    if (ConvertThread::shared->noEnlargeAll)
        convertThreads[data.tid]->confirmEnlarge(3);
    else if (ConvertThread::shared->abort)
        convertThreads[data.tid]->confirmEnlarge(4);
    else if (!ConvertThread::shared->enlargeAll) {

        int result = MessageBox::question(
                    this,
                    tr("Enlarge File? - SIR"),
                    tr("A file called %1 is smaller than the requested size. "
                       "Enlargement can cause deterioration of picture quality. "
                       "Do you want enlarge it?").arg(data.filePath));

        if (result == MessageBox::YesToAll)
            ConvertThread::shared->enlargeAll = true;
        else if (result == MessageBox::NoToAll)
            ConvertThread::shared->noEnlargeAll = true;
        else if (result == MessageBox::Cancel)
            ConvertThread::shared->abort = true;

        convertThreads[data.tid]->confirmEnlarge(result);
    }
    else
        convertThreads[data.tid]->confirmEnlarge(2);
}

/** Retranslates GUI. */
void ConvertDialog::retranslateStrings() {

    QList<QString> itemList;
    QString fileName;
    int sizeUnitIndex = sizeUnitComboBox->currentIndex();
    int fileSizeIndex = fileSizeComboBox->currentIndex();
    retranslateUi(this);
    // restoring nulled indexes
    sizeUnitComboBox->setCurrentIndex(sizeUnitIndex);
    fileSizeComboBox->setCurrentIndex(fileSizeIndex);

    itemList.append(tr("Name"));
    itemList.append(tr("Ext"));
    itemList.append(tr("Path"));
    itemList.append(tr("Status"));
    filesTreeWidget->setHeaderLabels(itemList);

    QTreeWidgetItemIterator it(filesTreeWidget);
    int count;
    count = 0;

    while (*it)
    {
        fileName = (*it)->text(2) + QDir::separator() +(*it)->text(0) + ".";
        fileName += (*it)->text(1);

        switch(statusList->value(fileName)) {
            case CONVERTED:
            (*it)->setText(3,tr("Converted"));
            break;
            case SKIPPED:
            (*it)->setText(3,tr("Skipped"));
            break;
            case FAILED:
            (*it)->setText(3,tr("Failed to convert"));
            break;
            case NOTCONVERTED:
            (*it)->setText(3,tr("Not converted yet"));
            break;
            case CONVERTING:
            (*it)->setText(3,tr("Converting"));
            break;
            case CANCELLED:
            (*it)->setText(3,tr("Cancelled"));
            break;
        }
        count++;
        ++it;
    }
}

/** Cancels converting if converting runs; otherwise close window. */
void ConvertDialog::closeOrCancel() {
    if (converting) {
        stopConvertThreads();
        updateInterface();
        setCanceled();
    }
    else
        close();
}

/** Terminates all worker threads. */
void ConvertDialog::stopConvertThreads() {
    for (int i=0; i<convertThreads.length(); i++)
        convertThreads[i]->terminate();
}

/** Updates user interface after convering. */
void ConvertDialog::updateInterface() {
    converting = false;
    convertSelectedButton->setEnabled(true);
    convertButton->setEnabled(true);
    resizeColumnsToContents(filesTreeWidget);
    setCursor(Qt::ArrowCursor);
    quitButton->setText(tr("Quit"));
}

/** Sets image status to \em Cancelled if the image isn't converted yet. */
void ConvertDialog::setCanceled() {
    int count = filesTreeWidget->topLevelItemCount();
    QTreeWidgetItem *item = new QTreeWidgetItem();
    QString status = tr("Cancelled");
    QString converted = tr("Converted");
    for(int i = 0; i < count; i++) {
        item = filesTreeWidget->topLevelItem(i);
        if (item->text(3) != converted)
            item->setText(3, status);
    }
}

/** Shows size values corresponding index of size unit combo box. */
void ConvertDialog::setSizeUnit(int index) {
    if (index < 0)
        return;
    static int lastIndex = index;
    static int lastIndexPxPercent = index+1;
    static bool maintainRatioAspect = maintainCheckBox->isChecked();
    if (index == 2) { // bytes
        geometryWidget->hide();
        fileSizeWidget->show();
        maintainRatioAspect = maintainCheckBox->isCheckable();
        maintainCheckBox->setChecked(true);
        maintainCheckBox->setEnabled(false);
    }
    else { // px or %
        fileSizeWidget->hide();
        geometryWidget->show();
        disconnectSizeLinesEdit();
        if (lastIndexPxPercent != index) {
            QString tmp = sizeWidthString;
            sizeWidthString = widthLineEdit->text();
            widthLineEdit->setText(tmp);
            tmp = sizeHeightString;
            sizeHeightString = heightLineEdit->text();
            heightLineEdit->setText(tmp);
            lastIndexPxPercent = index;
        }
        if (lastIndex == 2) {
            maintainCheckBox->setEnabled(true);
            maintainCheckBox->setChecked(maintainRatioAspect);
        }
        if (maintainCheckBox->isChecked() && index == 1) // %
            heightLineEdit->setText( widthLineEdit->text() );
        connectSizeLinesEdit();
    }
    lastIndex = index;
}

/** If desired size unit is percent and it keeps aspect ratio it will be change
  * width or heigth percent value following the user change in adjacent line edit.
  */
void ConvertDialog::sizeChanged(const QString &value) {
    // size unit is % and maintainCheckBox is checked
    if (sizeUnitComboBox->currentIndex() == 1 && maintainCheckBox->isChecked()) {
        QLineEdit *line = static_cast<QLineEdit*>(sender());
        int pos = line->cursorPosition();
        QString senderName = sender()->objectName();
        if (senderName == "widthLineEdit")
            heightLineEdit->setText(value);
        else if(senderName == "heightLineEdit")
            widthLineEdit->setText(value);
        line->setCursorPosition(pos);
    }
}
