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

#include "kadas/gui/kadasmapwidget.h"
#include "kadasmapwidgetmanager.h"

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

KadasMapWidget *KadasMapWidgetManager::addMapWidget( const QString &id )
{
  int highestNumber = 1;
  for ( KadasMapWidget *mapWidget : mMapWidgets )
  {
    if ( mapWidget->getNumber() >= highestNumber )
    {
      highestNumber = mapWidget->getNumber() + 1;
    }
  }

  QString widgetId = id;
  if ( widgetId.isEmpty() )
  {
    widgetId = QUuid::createUuid().toString();
  }

  KadasMapWidget *mapWidget = new KadasMapWidget( highestNumber, widgetId, tr( "View #%1" ).arg( highestNumber ), mMasterCanvas );
  connect( mapWidget, &KadasMapWidget::aboutToBeDestroyed, this, &KadasMapWidgetManager::mapWidgetDestroyed );

  // Determine whether to add the new dock widget in the right or bottom area
  int nRight = 0, nBottom = 0;
  double rightAreaWidth = 0;
  double bottomAreaHeight = 0;
  for ( KadasMapWidget *&mapWidget : mMapWidgets )
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
  if ( nRight == 0 || nBottom > nRight )
  {
    addArea = Qt::RightDockWidgetArea;
    initialSize.setHeight( mMasterCanvas->height() / ( nRight + 1 ) );
    initialSize.setWidth( nRight == 0 ? 0.5 * mMasterCanvas->width() : rightAreaWidth );
  }
  else
  {
    addArea = Qt::BottomDockWidgetArea;
    initialSize.setHeight( nBottom == 0 ? 0.5 * mMasterCanvas->height() : bottomAreaHeight );
    initialSize.setWidth( ( mMasterCanvas->width() + rightAreaWidth ) / ( nBottom + 1 ) );
  }

  // Set initial layers
  QStringList initialLayers;
  for ( QgsLayerTreeLayer *layerTreeLayer : QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    if ( layerTreeLayer->layer() )
    {
      initialLayers.append( layerTreeLayer->layer()->id() );
    }
  }
  if ( addArea == Qt::RightDockWidgetArea )
  {
    mapWidget->widget()->resize( mapWidget->widget()->width(), initialSize.height() );
  }
  else
  {
    mapWidget->widget()->resize( initialSize.width(), mapWidget->widget()->width() );
  }
  mMainWindow->addDockWidget( addArea, mapWidget );
  if ( addArea == Qt::RightDockWidgetArea )
  {
    mMainWindow->resizeDocks( { mapWidget }, { initialSize.width() }, Qt::Horizontal );
    mMainWindow->resizeDocks( { mapWidget }, { initialSize.height() }, Qt::Vertical );
  }
  else
  {
    mMainWindow->resizeDocks( { mapWidget }, { initialSize.height() }, Qt::Vertical );
    mMainWindow->resizeDocks( { mapWidget }, { initialSize.width() }, Qt::Horizontal );
  }
  mMapWidgets.append( mapWidget );
  return mapWidget;
}

void KadasMapWidgetManager::removeMapWidget( const QString &id )
{
  int pos = -1;
  for ( int i = 0, n = mMapWidgets.size(); i < n; ++i )
  {
    if ( mMapWidgets[i]->id() == id )
    {
      pos = i;
      break;
    }
  }
  if ( pos != -1 )
  {
    delete mMapWidgets.takeAt( pos );
  }
}

void KadasMapWidgetManager::clearMapWidgets()
{
  while ( !mMapWidgets.isEmpty() )
  {
    KadasMapWidget *mapWidget = mMapWidgets.takeAt( 0 );
    disconnect( mapWidget, &KadasMapWidget::aboutToBeDestroyed, this, &KadasMapWidgetManager::mapWidgetDestroyed );
    delete mapWidget;
  }
}

void KadasMapWidgetManager::mapWidgetDestroyed()
{
  KadasMapWidget *mapWidget = qobject_cast<KadasMapWidget *>( QObject::sender() );
  if ( mapWidget )
  {
    mMapWidgets.removeAll( mapWidget );
  }
}

void KadasMapWidgetManager::writeProjectSettings( QDomDocument &doc )
{
  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement mapViewsElem = doc.createElement( "MapViews" );
  for ( KadasMapWidget *mapWidget : mMapWidgets )
  {
    QByteArray ba;
    QDataStream ds( &ba, QIODevice::WriteOnly );

    QDomElement mapWidgetItemElem = doc.createElement( "MapView" );
    mapWidgetItemElem.setAttribute( "width", mapWidget->widget()->width() );
    mapWidgetItemElem.setAttribute( "height", mapWidget->widget()->height() );
    mapWidgetItemElem.setAttribute( "floating", mapWidget->isFloating() );
    mapWidgetItemElem.setAttribute( "islocked", mapWidget->getLocked() );
    mapWidgetItemElem.setAttribute( "area", mMainWindow->dockWidgetArea( mapWidget ) );
    mapWidgetItemElem.setAttribute( "title", mapWidget->windowTitle() );
    mapWidgetItemElem.setAttribute( "id", mapWidget->id() );
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

void KadasMapWidgetManager::readProjectSettings( const QDomDocument &doc )
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
      KadasMapWidget *mapWidget = new KadasMapWidget(
        attributes.namedItem( "number" ).nodeValue().toInt(),
        attributes.namedItem( "id " ).nodeValue(),
        attributes.namedItem( "title" ).nodeValue(),
        mMasterCanvas
      );
      mapWidget->setAttribute( Qt::WA_DeleteOnClose );
      ba = QByteArray::fromBase64( attributes.namedItem( "layers" ).nodeValue().toLocal8Bit() );
      QStringList layersList;
      ds >> layersList;
      mapWidget->setInitialLayers( layersList );
      connect( mapWidget, &KadasMapWidget::aboutToBeDestroyed, this, &KadasMapWidgetManager::mapWidgetDestroyed );
      // Compiler bug?! If I pass it directly, value is always false
      bool islocked = attributes.namedItem( "islocked" ).nodeValue().toInt();
      mapWidget->setLocked( islocked );
      mapWidget->setFloating( attributes.namedItem( "floating" ).nodeValue().toInt() );
      mapWidget->widget()->setFixedSize(
        QSize(
          attributes.namedItem( "width" ).nodeValue().toInt(),
          attributes.namedItem( "height" ).nodeValue().toInt()
        )
      );
      ba = QByteArray::fromBase64( attributes.namedItem( "extent" ).nodeValue().toLocal8Bit() );
      QRectF extent;
      ds >> extent;
      mapWidget->setMapExtent( QgsRectangle( extent ) );
      mMainWindow->addDockWidget( static_cast<Qt::DockWidgetArea>( attributes.namedItem( "area" ).nodeValue().toInt() ), mapWidget );
      mMapWidgets.append( mapWidget );
    }
  }
}
