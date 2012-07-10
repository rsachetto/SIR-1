/*
 * This file is part of SIR, an open-source cross-platform Image tool
 * 2007-2010  Rafael Sachetto
 * 2011-2012  Marek Jędryka
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
 * Contact e-mail: Rafael Sachetto <rsachetto@gmail.com>
 *                 Marek Jędryka   <jedryka89@gmail.com>
 * Program URL: http://sir.projet-libre.org/
 *
 */

#ifndef HISTORYCOMBOBOX_H
#define HISTORYCOMBOBOX_H

#include <QComboBox>

template <typename T> class QList;

/** \brief Combo box with text edition history support.
  * \par Favorite items
  * This class provides favorites items marked by star icon.\n
  * Favorite item (full star icon) can't be removed from combo box.\n
  * Unfavorite item (empty star icon) can be removed from combo box if maximum of
  * items count is overloaded while adding new item into combo box.
  */
class HistoryComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit HistoryComboBox(QWidget *parent = 0);
    ~HistoryComboBox();
    bool removeFromEnd();
    void importHistory(const QMap<QString,QVariant> &currentMap,
                     const QList<QVariant> &historyList,
                     int maxCount);
    void exportHistory(QMap<QString,QVariant> *currentMap,
                       QList<QVariant> *historyList,
                       int newMaxCount = -1);

private:
    void createActions();
    QIcon favIcon;
    QIcon unfavIcon;
    int listViewRow;
    QAction *favAction;
    QAction *rmAction;
    QAction *clearAction;
    QAction *surviveAction;

public slots:
    void prependText();
    void promoteItem(int);
    void favoriteAct();
    void removeAct();
    void clearAct();
    void surviveAct();
    void showMenu(const QPoint&);

protected:
    inline bool connectPromoteItem();
    inline bool disconnectPromoteItem();
};

/** Connects currentIndexChanged(int) signal to promoteItem(int) slot.\n
  * When this signal connected and current index changed (i.e. selected by user),
  * selected item will be moved to top of combo box.
  * \sa disconnectPromoteItem
  */
bool HistoryComboBox::connectPromoteItem() {
    return connect(this,SIGNAL(currentIndexChanged(int)),SLOT(promoteItem(int)));
}

/** Disconnects all signals connected to promoteItem(int) slot.\n
  * If you want to change order of items in this combo box manually use
  * this method before changing.
  * \sa connectPromoteItem
  */
bool HistoryComboBox::disconnectPromoteItem() {
    return disconnect(this,SLOT(promoteItem(int)));
}

#endif // HISTORYCOMBOBOX_H