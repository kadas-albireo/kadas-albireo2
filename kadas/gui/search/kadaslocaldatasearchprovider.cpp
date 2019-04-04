/***************************************************************************
    kadaslocaldatasearchprovider.cpp
    --------------------------------
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

#include <QMutexLocker>
#include <QThread>

//#include <qgis/qgslegendinterface.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaplayer.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsfeaturerequest.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsgeometry.h>

#include "kadaslocaldatasearchprovider.h"


KadasLocalDataSearchProvider::KadasLocalDataSearchProvider( QgsMapCanvas* mapCanvas )
    : KadasSearchProvider( mapCanvas )
{
}

void KadasLocalDataSearchProvider::startSearch( const QString& searchtext, const SearchRegion& searchRegion )
{
  QList<QgsMapLayer*> visibleLayers;
  for ( QgsMapLayer* layer : QgsProject::instance()->mapLayers() )
  {
    if ( layer->type() == QgsMapLayer::VectorLayer && mMapCanvas->layers().contains( layer ) )
      visibleLayers.append( layer );
  }

  QThread* crawlerThread = new QThread( this );
  mCrawler = new KadasLocalDataSearchCrawler( searchtext, searchRegion, visibleLayers );
  mCrawler->moveToThread( crawlerThread );
  connect( crawlerThread, &QThread::started, mCrawler, &KadasLocalDataSearchCrawler::run );
  connect( crawlerThread, &QThread::finished, crawlerThread, &QThread::deleteLater );
  connect( mCrawler, &KadasLocalDataSearchCrawler::searchResultFound, this, &KadasLocalDataSearchProvider::searchResultFound );
  connect( mCrawler, &KadasLocalDataSearchCrawler::searchFinished, this, &KadasLocalDataSearchProvider::searchFinished );
  connect( mCrawler, &KadasLocalDataSearchCrawler::searchFinished, crawlerThread, &QThread::quit );
  connect( mCrawler, &KadasLocalDataSearchCrawler::searchFinished, mCrawler, &QThread::deleteLater );
  crawlerThread->start();
}

void KadasLocalDataSearchProvider::cancelSearch()
{
  if ( mCrawler )
    mCrawler->abort();
}


const int KadasLocalDataSearchCrawler::sResultCountLimit = 50;

void KadasLocalDataSearchCrawler::run()
{
  int resultCount = 0;

  QString escapedSearchText = mSearchText;
  escapedSearchText.replace( "'", "\\'" );
  for ( QgsMapLayer* layer : mLayers )
  {
    QMutexLocker locker( &mAbortMutex );
    if ( mAborted )
    {
      break;
    }
    locker.unlock();

    QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( layer );

    const QgsFields& fields = vlayer->fields();
    QStringList conditions;
    for ( int idx = 0, nFields = fields.count(); idx < nFields; ++idx )
    {
      conditions.append( QString( "regexp_matchi( \"%1\" ,'%2')" ).arg( fields[idx].name(), escapedSearchText ) );
    }
    QString exprText = conditions.join( " OR " );

    QgsFeatureRequest req;
    QgsFeature feature;
    if ( !mSearchRegion.polygon.isEmpty() )
    {
      QgsLineString* exterior = new QgsLineString();
      QgsCoordinateTransform ct( QgsCoordinateReferenceSystem(mSearchRegion.crs), layer->crs(), QgsProject::instance() );
      for ( const QgsPointXY& p : mSearchRegion.polygon )
      {
        exterior->addVertex( QgsPoint( ct.transform( p ) ) );
      }
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( exterior );
      QgsGeometry filterGeom( poly );

      req.setFilterRect( filterGeom.boundingBox() );
      QgsExpression expr( exprText );
      QgsExpressionContext ectx;
      ectx.setFields( vlayer->fields() );
      expr.prepare( &ectx );
      QgsFeatureIterator it = vlayer->getFeatures( req );
      while ( it.nextFeature( feature ) && resultCount < sResultCountLimit )
      {
        locker.relock();
        if ( mAborted )
        {
          break;
        }
        locker.unlock();
        ectx.setFeature(feature);
        if ( expr.evaluate( &ectx ).toBool() && filterGeom.contains( feature.geometry() ) )
        {
          buildResult( feature, vlayer );
          ++resultCount;
        }
      }
    }
    else
    {
      req.setFilterExpression( exprText );
      QgsFeatureIterator it = vlayer->getFeatures( req );
      while ( it.nextFeature( feature ) && resultCount < sResultCountLimit )
      {
        locker.relock();
        if ( mAborted )
        {
          break;
        }
        locker.unlock();
        buildResult( feature, vlayer );
        ++resultCount;
      }
    }
    if ( resultCount >= sResultCountLimit )
    {
      QgsDebugMsg( "Stopping search due to result count limit hit" );
      break;
    }
  }
  emit searchFinished();
}

void KadasLocalDataSearchCrawler::buildResult( const QgsFeature &feature, QgsVectorLayer* layer )
{
  // Get the string which matched the search term
  const QgsFields& fields = layer->fields();
  QString matchText = mSearchText;
  for ( int idx = 0, nFields = fields.count(); idx < nFields; ++idx )
  {
    QString attribute = feature.attribute( idx ).toString();
    if ( attribute.contains( mSearchText, Qt::CaseInsensitive ) )
    {
      matchText = attribute;
      break;
    }
  }

  KadasSearchProvider::SearchResult result;
  result.bbox = feature.geometry().boundingBox();
  result.category = tr( "Layer %1" ).arg( layer->name() );
  result.categoryPrecedence = 10;
  result.crs = layer->crs().authid();
  result.zoomScale = 1000;
  result.showPin = true;
  QgsGeometry pt = feature.geometry().pointOnSurface();
  if ( pt.isEmpty() )
  {
    result.pos = pt.asPoint();
  }
  else
  {
    result.pos = result.bbox.center();
  }
  result.text = tr( "%1 (feature %2)" ).arg( matchText ).arg( feature.id() );
  emit searchResultFound( result );
}

void KadasLocalDataSearchCrawler::abort()
{
  mAbortMutex.lock();
  mAborted = true;
  mAbortMutex.unlock();
}
