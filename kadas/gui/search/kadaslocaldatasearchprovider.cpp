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

#include "kadas/gui/search/kadaslocaldatasearchprovider.h"


const int KadasLocalDataSearchFilter::sResultCountLimit = 50;


KadasLocalDataSearchFilter::KadasLocalDataSearchFilter( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
}

QgsLocatorFilter *KadasLocalDataSearchFilter::clone() const
{
  return new KadasLocalDataSearchFilter( mMapCanvas );
}

void KadasLocalDataSearchFilter::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  int resultCount = 0;
  QString escapedSearchText = string;
  escapedSearchText.replace( "'", "\\'" );

  const QVector<QgsVectorLayer *> layers = QgsProject::instance()->layers<QgsVectorLayer *>();
  for ( QgsVectorLayer *layer : layers )
  {
    if ( !mMapCanvas->layers().contains( layer ) )
      continue;

    const QgsFields &fields = layer->fields();
    QStringList conditions;
    for ( int idx = 0, nFields = fields.count(); idx < nFields; ++idx )
    {
      conditions.append( QString( "\"%1\" ILIKE '%%2%'" ).arg( fields[idx].name(), escapedSearchText ) );
    }
    QString exprText = conditions.join( " OR " );

    QgsFeatureRequest req;
    QgsFeature feature;
    if ( !context.targetExtent.isNull() )
    {
      QgsCoordinateTransform ct( QgsCoordinateReferenceSystem( context.targetExtentCrs ), layer->crs(), QgsProject::instance() );
      QgsRectangle box = ct.transformBoundingBox( context.targetExtent );
      req.setFilterRect( box );
      req.setFilterExpression( exprText );
      QgsFeatureIterator it = layer->getFeatures( req );
      while ( it.nextFeature( feature ) && resultCount < sResultCountLimit )
      {
        if ( feedback->isCanceled() )
        {
          break;
        }
        if ( context.targetExtent.intersects( feature.geometry().boundingBox() ) )
        {
          buildResult( feature, layer, escapedSearchText );
          ++resultCount;
        }
      }
    }
    else
    {
      req.setFilterExpression( exprText );
      QgsFeatureIterator it = layer->getFeatures( req );
      while ( it.nextFeature( feature ) && resultCount < sResultCountLimit )
      {
        if ( feedback->isCanceled() )
        {
          break;
        }
        buildResult( feature, layer, escapedSearchText );
        ++resultCount;
      }
    }
    if ( resultCount >= sResultCountLimit )
    {
      QgsDebugMsgLevel( "Stopping search due to result count limit hit", 2 );
      break;
    }
  }
}

void KadasLocalDataSearchFilter::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().value<QVariantMap>();
  QgsVectorLayer *layer = QgsProject::instance()->mapLayer<QgsVectorLayer *>( data.value( QStringLiteral( "layer_id" ) ).toString() );
  QgsFeatureId fid = data.value( QStringLiteral( "feature_id" ) ).value<QgsFeatureId>();

  if ( layer )
  {
    mMapCanvas->zoomToFeatureIds( layer, QgsFeatureIds() << fid );
    mMapCanvas->flashFeatureIds( layer, QgsFeatureIds() << fid );
  }
}


void KadasLocalDataSearchFilter::buildResult( const QgsFeature &feature, QgsVectorLayer *layer, const QString &searchText )
{
  // Get the string which matched the search term
  const QgsFields &fields = layer->fields();
  QString matchText = searchText;
  for ( int idx = 0, nFields = fields.count(); idx < nFields; ++idx )
  {
    QString attribute = feature.attribute( idx ).toString();
    if ( attribute.contains( searchText, Qt::CaseInsensitive ) )
    {
      matchText = attribute;
      break;
    }
  }

  QgsLocatorResult result;
  result.displayString = tr( "%1 (feature %2)" ).arg( matchText ).arg( feature.id() );
  result.setUserData( QVariantMap(
    {
      { QStringLiteral( "feature_id" ), feature.id() },
      { QStringLiteral( "layer_id" ), layer->id() },
    }
  ) );
  emit resultFetched( result );
}
