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
#include <QToolButton>
#include <QWidgetAction>

#include <qgis/qgsapplication.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/kadasbookmarksmenu.h"


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

  QToolButton *deleteButton = new QToolButton();
  deleteButton->setIcon( QgsApplication::getThemeIcon( "/mIconDelete.svg" ) );
  deleteButton->setAutoRaise( true );

  QWidget *widget = new QWidget( this );
  widget->setLayout( new QHBoxLayout );
  widget->layout()->setContentsMargins( 20, 2, 2, 2 );
  widget->layout()->addWidget( new QLabel( bookmark->name ) );
  widget->layout()->addWidget( deleteButton );
  QWidgetAction *widgetAction = new QWidgetAction( this );
  widgetAction->setDefaultWidget( widget );
  connect( deleteButton, &QToolButton::clicked, [this, widgetAction, bookmark] { deleteBookmark( widgetAction, bookmark ); } );
  connect( widgetAction, &QWidgetAction::triggered, this, [this, bookmark] { restoreBookmark( bookmark ); } );
  addAction( widgetAction );
}

void KadasBookmarksMenu::addBookmark()
{
  QString name = QInputDialog::getText( mCanvas, tr( "Create Bookmark" ), tr( "Enter bookmark name" ) );
  if ( name.isEmpty() )
  {
    return;
  }

  Bookmark *bookmark = new Bookmark;
  bookmark->name = name;
  bookmark->crs = mCanvas->mapSettings().destinationCrs().authid();
  bookmark->extent = mCanvas->mapSettings().extent();

  for ( const QgsLayerTreeLayer *layer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    bookmark->layerVisibilities.insert( layer->layerId(), layer->itemVisibilityChecked() );
  }
  for ( const QgsLayerTreeGroup *group : QgsProject::instance()->layerTreeRoot()->findGroups() )
  {
    bookmark->groupVisibilities.insert( group->name(), group->itemVisibilityChecked() );
  }
  addBookmarkAction( bookmark );
}

void KadasBookmarksMenu::restoreBookmark( const Bookmark *bookmark )
{
  mCanvas->setDestinationCrs( QgsCoordinateReferenceSystem( bookmark->crs ) );
  mCanvas->setExtent( bookmark->extent );
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
  for ( const Bookmark *bookmark : mBookmarks )
  {
    QDomElement bookmarkEl = doc.createElement( "kadasbookmark" );
    bookmarkEl.setAttribute( "name", bookmark->name );
    bookmarkEl.setAttribute( "crs", bookmark->crs );
    bookmarkEl.setAttribute( "extent",
                             QString( "%1;%2;%3;%4" )
                             .arg( bookmark->extent.xMinimum(), 0, 'f', 4 )
                             .arg( bookmark->extent.yMinimum(), 0, 'f', 4 )
                             .arg( bookmark->extent.xMaximum(), 0, 'f', 4 )
                             .arg( bookmark->extent.yMaximum(), 0, 'f', 4 ) );
    for ( auto it = bookmark->layerVisibilities.begin(), itEnd = bookmark->layerVisibilities.end(); it != itEnd; ++it )
    {
      QDomElement layerTreeEl = doc.createElement( "layerTreeLayer" );
      layerTreeEl.setAttribute( "id", it.key() );
      layerTreeEl.setAttribute( "visibility", it.value() ? 1 : 0 );
      bookmarkEl.appendChild( layerTreeEl );
    }
    for ( auto it = bookmark->groupVisibilities.begin(), itEnd = bookmark->groupVisibilities.end(); it != itEnd; ++it )
    {
      QDomElement layerTreeEl = doc.createElement( "layerTreeGroup" );
      layerTreeEl.setAttribute( "id", it.key() );
      layerTreeEl.setAttribute( "visibility", it.value() ? 1 : 0 );
      bookmarkEl.appendChild( layerTreeEl );
    }
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
    addBookmarkAction( bookmark );
  }
}
