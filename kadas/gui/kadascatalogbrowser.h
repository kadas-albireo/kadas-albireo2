/***************************************************************************
    kadascatalogbrowser.h
    ---------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASCATALOGBROWSER_H
#define KADASCATALOGBROWSER_H

#include <QStandardItemModel>
#include <QWidget>

#include <kadas/gui/kadas_gui.h>

class KadasCatalogProvider;
class QgsFilterLineEdit;
class QTreeView;


class KADAS_GUI_EXPORT KadasCatalogBrowser : public QWidget
{
  Q_OBJECT
public:
  KadasCatalogBrowser ( QWidget* parent = 0 );
  void addProvider ( KadasCatalogProvider* provider ) { mProviders.append ( provider ); }
  QStandardItem* addItem ( QStandardItem* parent, QString text, int sortIndex, bool isLeaf = false, QMimeData* mimeData = 0 );

public slots:
  void reload();

private:
  class CatalogModel;
  class CatalogItem;
  class TreeFilterProxyModel;

  QgsFilterLineEdit* mFilterLineEdit;
  QTreeView* mTreeView;
  CatalogModel* mCatalogModel;
  QStandardItemModel* mLoadingModel;
  QStandardItemModel* mOfflineModel;
  TreeFilterProxyModel* mFilterProxyModel;
  QList<KadasCatalogProvider*> mProviders;
  int mFinishedProviders;

private slots:
  void filterChanged ( const QString& text );
  void itemDoubleClicked ( const QModelIndex& index );
  void providerFinished();
};

#endif // KADASCATALOGBROWSER_H
