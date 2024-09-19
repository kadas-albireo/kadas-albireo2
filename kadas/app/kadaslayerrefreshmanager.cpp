/***************************************************************************
    kadaslayerrefreshmanager.cpp
    ----------------------------
    copyright            : (C) 2022 by Sandro Mani
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

#include <qgis/qgsdataprovider.h>
#include <qgis/qgsproject.h>

#include "kadasapplication.h"
#include "kadaslayerrefreshmanager.h"

KadasLayerRefreshManager::KadasLayerRefreshManager( QObject *parent )
  : QObject( parent )
{
  connect( kApp, &KadasApplication::projectWillBeClosed, this, &KadasLayerRefreshManager::clear );
  connect( QgsProject::instance(), qOverload<const QString &>( &QgsProject::layerWillBeRemoved ), this, &KadasLayerRefreshManager::clearLayer );
  connect( QgsProject::instance(), &QgsProject::readProject, this, &KadasLayerRefreshManager::readProjectSettings );
  connect( QgsProject::instance(), &QgsProject::writeProject, this, &KadasLayerRefreshManager::writeProjectSettings );
}

void KadasLayerRefreshManager::setLayerRefreshInterval( const QString &layerId, int refreshIntervalSec )
{
  if ( layerId.isEmpty() )
  {
    return;
  }
  auto it = mLayerTimers.find( layerId );
  if ( it != mLayerTimers.end() )
  {
    if ( refreshIntervalSec > 0 )
    {
      it.value()->setInterval( refreshIntervalSec * 1000 );
    }
    else
    {
      delete it.value();
      mLayerTimers.erase( it );
    }
  }
  else if ( refreshIntervalSec > 0 )
  {
    QTimer *timer = new QTimer( this );
    timer->setSingleShot( false );
    connect( timer, &QTimer::timeout, this, [layerId]
    {
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
      if ( layer && layer->dataProvider() )
      {
        layer->dataProvider()->reloadData();
        layer->repaintRequested();
      }
    } );
    timer->start( refreshIntervalSec * 1000 );
    mLayerTimers.insert( layerId, timer );
  }
}

int KadasLayerRefreshManager::layerRefreshInterval( const QString &layerId ) const
{
  auto it = mLayerTimers.find( layerId );
  return it != mLayerTimers.end() ? it.value()->interval() / 1000 : 0;
}

void KadasLayerRefreshManager::clear()
{
  qDeleteAll( mLayerTimers.values() );
  mLayerTimers.clear();
}

void KadasLayerRefreshManager::clearLayer( const QString &layerId )
{
  auto it = mLayerTimers.find( layerId );
  if ( it != mLayerTimers.end() )
  {
    delete it.value();
    mLayerTimers.erase( it );
  }
}

void KadasLayerRefreshManager::writeProjectSettings( QDomDocument &doc )
{
  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement layerRefreshIntervalsEl = doc.createElement( "layerRefreshIntervals" );
  for ( auto it = mLayerTimers.begin(), itEnd = mLayerTimers.end(); it != itEnd; ++it )
  {
    QDomElement layerEl = doc.createElement( "layer" );
    layerEl.setAttribute( "layerId", it.key() );
    layerEl.setAttribute( "interval", it.value()->interval() / 1000 );
    layerRefreshIntervalsEl.appendChild( layerEl );
  }
  qgisElem.appendChild( layerRefreshIntervalsEl );
}

void KadasLayerRefreshManager::readProjectSettings( const QDomDocument &doc )
{
  clear();

  QDomNodeList nl = doc.elementsByTagName( "layerRefreshIntervals" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement layerRefreshIntervalsEl = nl.at( 0 ).toElement();
  QDomNodeList nodes = layerRefreshIntervalsEl.childNodes();
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement layerEl = nodes.at( iNode ).toElement();
    if ( layerEl.nodeName() == "layer" )
    {
      QString layerId = layerEl.attribute( "layerId" );
      int interval = layerEl.attribute( "interval" ).toInt();
      QTextStream( stdout ) << "xxx layerID=" << layerId << Qt::endl;
      QTextStream( stdout ) << "xxx interval=" << interval << Qt::endl;

      setLayerRefreshInterval( layerId, interval );
    }
  }
}
