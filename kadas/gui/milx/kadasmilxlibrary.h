/***************************************************************************
    kadasmilxlibrary.h
    ------------------
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

#ifndef KADASMILXLIBRARY_H
#define KADASMILXLIBRARY_H

#include <QFrame>
#include <QFutureWatcher>
#include <QThread>

class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QgsFilterLineEdit;

#include "kadas/gui/milx/kadasmilxclient.h"

class KADAS_GUI_EXPORT KadasMilxLibrary : public QFrame {
  Q_OBJECT
public:
  KadasMilxLibrary(WId winId, QWidget *parent = 0);
  ~KadasMilxLibrary();
  void focusFilter();

signals:
  void symbolSelected(const KadasMilxSymbolDesc &symbolDesc);
  void visibilityChanged(bool visible);

protected:
  void showEvent(QShowEvent *) { emit visibilityChanged(true); }
  void hideEvent(QHideEvent *) { emit visibilityChanged(false); }

private:
  class TreeFilterProxyModel;
  friend class KadasMilxLibraryLoader;

  static const int SymbolXmlRole;
  static const int SymbolMilitaryNameRole;
  static const int SymbolPointCountRole;
  static const int SymbolVariablePointsRole;
  static const int SymbolTypeRole;

  WId mWinId;
  QgsFilterLineEdit *mFilterLineEdit = nullptr;
  QTreeView *mTreeView = nullptr;
  QStandardItemModel *mGalleryModel = nullptr;
  QStandardItemModel *mLoadingModel = nullptr;
  TreeFilterProxyModel *mFilterProxyModel = nullptr;

  QAtomicInt mLoaderAborted = 0;
  QFutureWatcher<QStandardItemModel *> mLibraryFuture;

private slots:
  void filterChanged(const QString &text);
  void itemClicked(const QModelIndex &index);

private:
  QStandardItemModel *loadLibrary(const QSize &viewIconSize);
  static QStandardItem *
  addItem(QStandardItem *parent, const QString &value,
          const QImage &image = QImage(), const QSize &viewIconSize = QSize(),
          bool isLeaf = false, const QString &symbolXml = QString(),
          const QString &symbolMilitaryName = QString(),
          int symbolPointCount = 0, bool symbolHasVariablePoints = false,
          const QString &symbolType = QString());
  void loaderFinished();
};

#endif // KADASMILXLIBRARY_H
