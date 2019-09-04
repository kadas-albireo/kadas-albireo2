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
#include <QThread>

class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QgsFilterLineEdit;

class KadasMilxLibraryLoader;

#include <kadas/app/milx/kadasmilxclient.h>

class KadasMilxLibrary : public QFrame
{
    Q_OBJECT
  public:
    KadasMilxLibrary( QWidget *parent = 0 );
    ~KadasMilxLibrary();
    void focusFilter();

  signals:
    void symbolSelected( const KadasMilxClient::SymbolDesc &symbolDesc );
    void visibilityChanged( bool visible );

  private:
    class TreeFilterProxyModel;
    friend class KadasMilxLibraryLoader;

    static const int SymbolXmlRole;
    static const int SymbolMilitaryNameRole;
    static const int SymbolPointCountRole;
    static const int SymbolVariablePointsRole;

    KadasMilxLibraryLoader *mLoader;
    QgsFilterLineEdit *mFilterLineEdit;
    QTreeView *mTreeView;
    QStandardItemModel *mGalleryModel;
    QStandardItemModel *mLoadingModel;
    TreeFilterProxyModel *mFilterProxyModel;

    void showEvent( QShowEvent * ) { emit visibilityChanged( true ); }
    void hideEvent( QHideEvent * ) { emit visibilityChanged( false ); }

  private slots:
    void filterChanged( const QString &text );
    void itemClicked( const QModelIndex &index );
    void loaderFinished();
    QStandardItem *addItem( QStandardItem *parent, const QString &value, const QImage &image = QImage(), bool isLeaf = false, const QString &symbolXml = QString(), const QString &symbolMilitaryName = QString(), int symbolPointCount = 0, bool symbolHasVariablePoints = false );
};


class KadasMilxLibraryLoader : public QThread
{
    Q_OBJECT
  public:
    KadasMilxLibraryLoader( KadasMilxLibrary *library, QObject *parent = 0 ) : QThread( parent ), mAborted( false ), mLibrary( library ) {}
    void abort() { mAborted = true; }

  private:
    bool mAborted;
    KadasMilxLibrary *mLibrary;

    void run() override;
    QStandardItem *addItem( QStandardItem *parent, const QString &value, const QImage &image = QImage(), bool isLeaf = false, const QString &symbolXml = QString(), const QString &symbolMilitaryName = QString(), int symbolPointCount = 0, bool symbolHasVariablePoints = false );
};

#endif // KADASMILXLIBRARY_H
