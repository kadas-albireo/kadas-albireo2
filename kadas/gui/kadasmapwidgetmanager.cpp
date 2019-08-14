/***************************************************************************
    kadasmapwidgetmanager.cpp
    -------------------------
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

#include <QMainWindow>

#include <qgis/qgslayertree.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>

#include <kadas/gui/kadasmapwidget.h>
#include <kadas/gui/kadasmapwidgetmanager.h>

KadasMapWidgetManager::KadasMapWidgetManager( QgsMapCanvas *masterCanvas, QMainWindow *parent )
    : QObject( parent ), mMainWindow( parent ), mMasterCanvas( masterCanvas )
{
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasMapWidgetManager::readProjectSettings );
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasMapWidgetManager::writeProjectSettings );
}

KadasMapWidgetManager::~KadasMapWidgetManager()
{
  clearMapWidgets();
}

void KadasMapWidgetManager::addMapWidget()
{
  int highestNumber = 1;
  for ( const QPointer<KadasMapWidget>& mapWidget : mMapWidgets )
  {
    if ( mapWidget && mapWidget->getNumber() >= highestNumber )
    {
      highestNumber = mapWidget->getNumber() + 1;
    }
  }

  KadasMapWidget* mapWidget = new KadasMapWidget( highestNumber, tr( "View #%1" ).arg( highestNumber ), mMasterCanvas );
  connect( mapWidget, &QObject::destroyed, this, &KadasMapWidgetManager::mapWidgetDestroyed );

  // Determine whether to add the new dock widget in the right or bottom area
  int nRight = 0, nBottom = 0;
  double rightAreaWidth = 0;
  double bottomAreaHeight = 0;
  for ( const QPointer<KadasMapWidget>& mapWidget : mMapWidgets )
  {
    Qt::DockWidgetArea area = mMainWindow->dockWidgetArea( mapWidget );
    if ( area == Qt::RightDockWidgetArea )
    {
      ++nRight;
      rightAreaWidth = mapWidget->width();
    }
    else if ( area == Qt::BottomDockWidgetArea )
    {
      ++nBottom;
      bottomAreaHeight = mapWidget->height();
    }
  }
  QSize initialSize;
  Qt::DockWidgetArea addArea;
  if ( nBottom >= nRight - 1 )
  {
    addArea = Qt::RightDockWidgetArea;
    initialSize.setHeight(( mMasterCanvas->height() + bottomAreaHeight ) / ( nRight + 1 ) );
    initialSize.setWidth( nRight > 0 ? rightAreaWidth : mMasterCanvas->width() / 2 );
  }
  else
  {
    addArea = Qt::BottomDockWidgetArea;
    initialSize.setHeight( nBottom > 0 ? bottomAreaHeight : mMasterCanvas->height() / 2 );
    initialSize.setWidth( mMasterCanvas->width() / ( nBottom + 1 ) );
  }

  // Set initial layers
  QStringList initialLayers;
  for ( QgsLayerTreeLayer* layerTreeLayer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    if(layerTreeLayer->layer()) {
      initialLayers.append( layerTreeLayer->layer()->id() );
    }
  }
  mapWidget->setFixedSize( initialSize );
  mMainWindow->addDockWidget( addArea, mapWidget );
  mapWidget->resize( initialSize );
  mMapWidgets.append( QPointer<KadasMapWidget>( mapWidget ) );
}

void KadasMapWidgetManager::clearMapWidgets()
{
  for ( const QPointer<KadasMapWidget>& mapWidget : mMapWidgets )
  {
    delete mapWidget.data();
  }
  mMapWidgets.clear();
}

void KadasMapWidgetManager::mapWidgetDestroyed( QObject *mapWidget )
{
  QMutableListIterator<QPointer<KadasMapWidget>> i( mMapWidgets );
  while ( i.hasNext() )
  {
    const QPointer<KadasMapWidget>& p = i.next();
    if ( p.data() == nullptr || static_cast<QObject*>( p.data() ) == mapWidget )
    {
      i.remove();
    }
  }
}

void KadasMapWidgetManager::writeProjectSettings( QDomDocument& doc )
{
  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement mapViewsElem = doc.createElement( "MapViews" );
  for ( KadasMapWidget* mapWidget : mMapWidgets )
  {
    QByteArray ba;
    QDataStream ds( &ba, QIODevice::WriteOnly );

    QDomElement mapWidgetItemElem = doc.createElement( "MapView" );
    mapWidgetItemElem.setAttribute( "width", mapWidget->width() );
    mapWidgetItemElem.setAttribute( "height", mapWidget->height() );
    mapWidgetItemElem.setAttribute( "floating", mapWidget->isFloating() );
    mapWidgetItemElem.setAttribute( "islocked", mapWidget->getLocked() );
    mapWidgetItemElem.setAttribute( "area", mMainWindow->dockWidgetArea( mapWidget ) );
    mapWidgetItemElem.setAttribute( "title", mapWidget->windowTitle() );
    mapWidgetItemElem.setAttribute( "number", mapWidget->getNumber() );
    ds << mapWidget->getLayers();
    mapWidgetItemElem.setAttribute( "layers", QString( ba.toBase64() ) );
    ba.clear();
    ds << mapWidget->getMapExtent().toRectF();
    mapWidgetItemElem.setAttribute( "extent", QString( ba.toBase64() ) );
    mapViewsElem.appendChild( mapWidgetItemElem );
  }
  qgisElem.appendChild( mapViewsElem );
}

void KadasMapWidgetManager::readProjectSettings( const QDomDocument& doc )
{
  clearMapWidgets();

  QDomNodeList nl = doc.elementsByTagName( "MapViews" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }

  QDomElement mapViewsElem = nl.at( 0 ).toElement();
  QDomNodeList nodes = mapViewsElem.childNodes();
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement mapWidgetItemElem = nodes.at( iNode ).toElement();
    if ( mapWidgetItemElem.nodeName() == "MapView" )
    {
      QDomNamedNodeMap attributes = mapWidgetItemElem.attributes();
      QByteArray ba;
      QDataStream ds( &ba, QIODevice::ReadOnly );
      KadasMapWidget* mapWidget = new KadasMapWidget(
        attributes.namedItem( "number" ).nodeValue().toInt(),
        attributes.namedItem( "title" ).nodeValue(),
        mMasterCanvas );
      mapWidget->setAttribute( Qt::WA_DeleteOnClose );
      ba = QByteArray::fromBase64( attributes.namedItem( "layers" ).nodeValue().toLocal8Bit() );
      QStringList layersList;
      ds >> layersList;
      mapWidget->setInitialLayers( layersList );
      connect( mapWidget, &QObject::destroyed, this, &KadasMapWidgetManager::mapWidgetDestroyed );
      // Compiler bug?! If I pass it directly, value is always false
      bool islocked = attributes.namedItem( "islocked" ).nodeValue().toInt();
      mapWidget->setLocked( islocked );
      mapWidget->setFloating( attributes.namedItem( "floating" ).nodeValue().toInt() );
      mapWidget->setFixedWidth( attributes.namedItem( "width" ).nodeValue().toInt() );
      mapWidget->setFixedHeight( attributes.namedItem( "height" ).nodeValue().toInt() );
      ba = QByteArray::fromBase64( attributes.namedItem( "extent" ).nodeValue().toLocal8Bit() );
      QRectF extent;
      ds >> extent;
      mapWidget->setMapExtent( QgsRectangle( extent ) );
      mMainWindow->addDockWidget( static_cast<Qt::DockWidgetArea>( attributes.namedItem( "area" ).nodeValue().toInt() ), mapWidget );
      mMapWidgets.append( mapWidget );
    }
  }
}
