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

#ifndef PREVIEWDIALOG_H
#define PREVIEWDIALOG_H

#include <QString>
#include <QGraphicsSvgItem>

#include "ui_previewdialog.h"
#include "metadatautils.h"

class QStringList;
class QKeyEvent;

//! Preview image window.
class PreviewDialog : public QDialog, public Ui::PreviewDialog {
    Q_OBJECT
public:
    PreviewDialog(QWidget *parent = 0, QStringList *images = 0,
                  int currentImage = 0);
    ~PreviewDialog();

private:
    //Methods
    void initBar();
    void createConnections();
    void verifyImages();
    void reloadImage(QString imagePath);
    void loadPixmap();
    inline bool isntTiffImage();
    inline QGraphicsItem *addImageIntoScene();

    //Class Variables
    QGraphicsScene *scene;
    QString imagePath;
    QByteArray fileExt;
    QPixmap *image;
    QGraphicsSvgItem *svgImage;
    double zoomFactor;
    int currentImage;
    int rotation;
    int flip;
    QStringList *images;
    QGraphicsItem *imageItem;
    int imageW;
    int imageH;
    bool rawEnabled;
    bool svgLoaded;
    bool metadataEnabled;
    bool saveMetadata;
    MetadataUtils::Metadata* metadata;
    QString destFileExtension;

public slots:
    //Slots
    void zoom(const QString &text);
    void rotatecw( );
    void rotateccw( );
    void flipVertical();
    void flipHorizontal();
    void nextImage();
    void previousImage();
    void fullScreen();
    bool save();
    bool saveAs();
    bool saveFile(const QString &fileName);
    void print();
    void setDestFileExt(const QString &ext);

protected:
    virtual void resizeEvent(QResizeEvent *);
};

bool PreviewDialog::isntTiffImage() {
    QString ext = imagePath.split('.').last().toLower();
    bool tiffMetadataDisabled = !(ext == "tif" || ext == "tiff");
    if (MetadataUtils::Metadata::isEnabled() && tiffMetadataDisabled) {
        metadata = new MetadataUtils::Metadata();
        return true;
    }
    return false;
}

QGraphicsItem * PreviewDialog::addImageIntoScene() {
    if (svgLoaded) {
        scene->addItem(svgImage);
        return svgImage;
    }
    return scene->addPixmap(*image);
}

#endif
