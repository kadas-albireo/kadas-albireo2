/***************************************************************************
    kadasbookmarksmenu.cpp
    ----------------------
    copyright            : (C) 2021 by Sandro Mani
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

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QToolButton>
#include <QWidgetAction>

#include <qgis/qgsapplication.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproject.h>

#include "kadasapplication.h"
#include "kadasbookmarksmenu.h"
#include "kadasmainwindow.h"


KadasBookmarksMenu::KadasBookmarksMenu( QgsMapCanvas *canvas, QgsMessageBar *messageBar, QWidget *parent )
  : QMenu( parent ), mCanvas( canvas ), mMessageBar( messageBar )
{
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasBookmarksMenu::restoreFromProject );
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasBookmarksMenu::saveToProject );

  clearMenu();
}

KadasBookmarksMenu::~KadasBookmarksMenu()
{
  qDeleteAll( mBookmarks );
}

void KadasBookmarksMenu::clearMenu()
{
  clear();
  addAction( tr( "Create bookmark..." ), this, &KadasBookmarksMenu::addBookmark );
  addSeparator();
}

void KadasBookmarksMenu::addBookmarkAction( Bookmark *bookmark )
{
  mBookmarks.append( bookmark );

  QWidget *widget = new QWidget( this );

  QToolButton *replaceButton = new QToolButton( widget );
  replaceButton->setIcon( QIcon( ":/kadas/icons/refresh-v2" ) );
  replaceButton->setToolTip( tr( "Replace bookmark" ) );
  replaceButton->setAutoRaise( true );

  QToolButton *deleteButton = new QToolButton( widget );
  deleteButton->setIcon( QgsApplication::getThemeIcon( "/mIconDelete.svg" ) );
  deleteButton->setToolTip( tr( "Delete bookmark" ) );
  deleteButton->setAutoRaise( true );

  widget->setLayout( new QHBoxLayout );
  widget->layout()->setContentsMargins( 20, 2, 2, 2 );
  widget->layout()->addWidget( new QLabel( bookmark->name ) );
  widget->layout()->addWidget( replaceButton );
  widget->layout()->addWidget( deleteButton );
  QWidgetAction *widgetAction = new QWidgetAction( this );
  widgetAction->setDefaultWidget( widget );
  connect( replaceButton, &QToolButton::clicked, this, [=] { replaceBookmark( bookmark ); } );
  connect( deleteButton, &QToolButton::clicked, this, [=] { deleteBookmark( widgetAction, bookmark ); } );
  connect( widgetAction, &QWidgetAction::triggered, this, [=] { restoreBookmark( bookmark ); } );
  addAction( widgetAction );
}

void KadasBookmarksMenu::addBookmark()
{
  QString name = QInputDialog::getText( mCanvas, tr( "Create Bookmark" ), tr( "Enter bookmark name" ) );
  if ( name.isEmpty() )
  {
    return;
  }

  bool replaceExisting = false;
  Bookmark *bookmark = nullptr;

  for ( QList<Bookmark *>::iterator it = mBookmarks.begin(); it != mBookmarks.end(); it++ )
  {
    if ( ( *it )->name == name )
    {
      int res = QMessageBox::question( kApp->mainWindow(), tr( "Replace Bookmark" ), tr( "Are you sure you want to replace the existing bookmark “%1”?" ).arg( name ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
      if ( res != QMessageBox::Yes )
        return;

      replaceExisting = true;
      bookmark = *it;
      break;
    }
  }
  if ( !bookmark )
  {
    bookmark = new Bookmark();
    bookmark->name = name;
  }

  QgsLayerTreeGroup *root = QgsProject::instance()->layerTreeRoot();
  QgsLayerTreeModel *model = kApp->mainWindow()->layerTreeView()->layerTreeModel();
  QgsMapThemeCollection::MapThemeRecord record = QgsMapThemeCollection::createThemeFromCurrentState( root, model );
  QgsProject::instance()->mapThemeCollection()->insert( name, record );

  bookmark->crs = mCanvas->mapSettings().destinationCrs().authid();
  bookmark->extent = mCanvas->mapSettings().extent();

  bookmark->groupVisibilities.clear();
  bookmark->layerVisibilities.clear();

  if ( !replaceExisting )
    addBookmarkAction( bookmark );
}

void KadasBookmarksMenu::replaceBookmark( Bookmark *bookmark )
{
  QgsLayerTreeGroup *root = QgsProject::instance()->layerTreeRoot();
  QgsLayerTreeModel *model = kApp->mainWindow()->layerTreeView()->layerTreeModel();
  QgsMapThemeCollection::MapThemeRecord record = QgsMapThemeCollection::createThemeFromCurrentState( root, model );
  QgsProject::instance()->mapThemeCollection()->insert( bookmark->name, record );

  bookmark->crs = mCanvas->mapSettings().destinationCrs().authid();
  bookmark->extent = mCanvas->mapSettings().extent();

  bookmark->groupVisibilities.clear();
  bookmark->layerVisibilities.clear();
}

void KadasBookmarksMenu::restoreBookmark( Bookmark *bookmark )
{
  mCanvas->setDestinationCrs( QgsCoordinateReferenceSystem( bookmark->crs ) );
  mCanvas->setExtent( bookmark->extent );

  QgsLayerTreeGroup *root = QgsProject::instance()->layerTreeRoot();
  QgsLayerTreeModel *model = kApp->mainWindow()->layerTreeView()->layerTreeModel();
  if ( !bookmark->layerVisibilities.isEmpty() )
  {
    // compatibility code
    mCanvas->freeze( true );
    // Disable all entries first, then re-enable the ones stored in the bookmark
    for ( QgsLayerTreeLayer *layer : QgsProject::instance()->layerTreeRoot()->findLayers() )
    {
      layer->setItemVisibilityChecked( false );
    }
    for ( QgsLayerTreeGroup *group : QgsProject::instance()->layerTreeRoot()->findGroups() )
    {
      group->setItemVisibilityChecked( false );
    }

    bool missing = false;
    for ( auto it = bookmark->layerVisibilities.begin(), itEnd = bookmark->layerVisibilities.end(); it != itEnd; ++it )
    {
      QgsLayerTreeLayer *layer = QgsProject::instance()->layerTreeRoot()->findLayer( it.key() );
      if ( layer )
      {
        layer->setItemVisibilityChecked( it.value() );
      }
      else
      {
        missing = true;
      }
    }
    for ( auto it = bookmark->groupVisibilities.begin(), itEnd = bookmark->groupVisibilities.end(); it != itEnd; ++it )
    {
      QgsLayerTreeGroup *group = QgsProject::instance()->layerTreeRoot()->findGroup( it.key() );
      if ( group )
      {
        group->setItemVisibilityChecked( it.value() );
      }
      else
      {
        missing = true;
      }
    }
    mCanvas->freeze( false );
    if ( missing )
    {
      int timeout = QgsSettings().value( QStringLiteral( "qgis/messageTimeout" ), 5 ).toInt();
      mMessageBar->pushMessage( tr( "Some layers stored in the bookmark don't exist" ), Qgis::MessageLevel::Warning, timeout );
    }

    // now create the map theme and delete old state
    QgsLayerTreeGroup *root = QgsProject::instance()->layerTreeRoot();
    QgsLayerTreeModel *model = kApp->mainWindow()->layerTreeView()->layerTreeModel();
    QgsMapThemeCollection::MapThemeRecord record = QgsMapThemeCollection::createThemeFromCurrentState( root, model );
    QgsProject::instance()->mapThemeCollection()->insert( bookmark->name, record );

    bookmark->groupVisibilities.clear();
    bookmark->layerVisibilities.clear();
  }
  else
  {
    QgsProject::instance()->mapThemeCollection()->applyTheme( bookmark->name, root, model );
  }
}

void KadasBookmarksMenu::deleteBookmark( QAction *action, Bookmark *bookmark )
{
  removeAction( action );
  mBookmarks.removeAll( bookmark );
}

void KadasBookmarksMenu::saveToProject( QDomDocument &doc )
{
  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement root = nl.at( 0 ).toElement();

  QDomElement bookmarksEl = doc.createElement( "kadasbookmarks" );
  for ( const Bookmark *bookmark : std::as_const( mBookmarks ) )
  {
    QDomElement bookmarkEl = doc.createElement( "kadasbookmark" );
    bookmarkEl.setAttribute( "name", bookmark->name );
    bookmarkEl.setAttribute( "crs", bookmark->crs );
    bookmarkEl.setAttribute( "extent", QString( "%1;%2;%3;%4" ).arg( bookmark->extent.xMinimum(), 0, 'f', 4 ).arg( bookmark->extent.yMinimum(), 0, 'f', 4 ).arg( bookmark->extent.xMaximum(), 0, 'f', 4 ).arg( bookmark->extent.yMaximum(), 0, 'f', 4 ) );
    bookmarksEl.appendChild( bookmarkEl );
  }
  root.appendChild( bookmarksEl );
}

void KadasBookmarksMenu::restoreFromProject( const QDomDocument &doc )
{
  qDeleteAll( mBookmarks );
  mBookmarks.clear();
  clearMenu();

  QDomNodeList nl = doc.elementsByTagName( "kadasbookmarks" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement bookmarksEl = nl.at( 0 ).toElement();
  QDomNodeList bookmarkNodes = bookmarksEl.elementsByTagName( "kadasbookmark" );
  for ( int i = 0, n = bookmarkNodes.length(); i < n; ++i )
  {
    QDomElement bookmarkEl = bookmarkNodes.at( i ).toElement();
    QStringList coords = bookmarkEl.attribute( "extent" ).split( ";" );
    if ( coords.size() != 4 )
    {
      continue;
    }
    Bookmark *bookmark = new Bookmark;
    bookmark->name = bookmarkEl.attribute( "name" );
    bookmark->crs = bookmarkEl.attribute( "crs" );
    bookmark->extent = QgsRectangle( coords[0].toDouble(), coords[1].toDouble(), coords[2].toDouble(), coords[3].toDouble() );

    QDomNodeList layerTreeLayers = bookmarkEl.elementsByTagName( "layerTreeLayer" );
    if ( !layerTreeLayers.isEmpty() )
    {
      // compatatibility code
      for ( int j = 0, m = layerTreeLayers.size(); j < m; ++j )
      {
        QDomElement layerTreeEl = layerTreeLayers.at( j ).toElement();
        bookmark->layerVisibilities.insert( layerTreeEl.attribute( "id" ), layerTreeEl.attribute( "visibility" ).toInt() );
      }
      QDomNodeList layerTreeGroups = bookmarkEl.elementsByTagName( "layerTreeGroup" );
      for ( int j = 0, m = layerTreeGroups.size(); j < m; ++j )
      {
        QDomElement layerTreeEl = layerTreeGroups.at( j ).toElement();
        bookmark->groupVisibilities.insert( layerTreeEl.attribute( "id" ), layerTreeEl.attribute( "visibility" ).toInt() );
      }
    }

    addBookmarkAction( bookmark );
  }
}
