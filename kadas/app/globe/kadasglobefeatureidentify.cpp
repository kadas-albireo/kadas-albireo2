/***************************************************************************
    kadasglobefeatureidentify.cpp
    -----------------------------
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

#include <osg/ValueObject>
#include <osgEarthFeatures/FeatureIndex>
#include <osgEarth/Registry>

#include <qgis/qgslogger.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrubberband.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/app/globe/kadasglobefeatureidentify.h>
#include <kadas/app/globe/featuresource/kadasglobefeaturesource.h>


KadasGlobeFeatureIdentifyCallback::KadasGlobeFeatureIdentifyCallback( QgsMapCanvas *mapCanvas )
  : mCanvas( mapCanvas )
  , mRubberBand( new QgsRubberBand( mapCanvas, QgsWkbTypes::PolygonGeometry ) )
{
  QColor color( Qt::green );
  color.setAlpha( 190 );

  mRubberBand->setColor( color );
}

KadasGlobeFeatureIdentifyCallback::~KadasGlobeFeatureIdentifyCallback()
{
  mCanvas->scene()->removeItem( mRubberBand );
  delete mRubberBand;
}

void KadasGlobeFeatureIdentifyCallback::onHit( osgEarth::ObjectID id )
{
  osgEarth::Features::FeatureIndex *index = osgEarth::Registry::objectIndex()->get<osgEarth::Features::FeatureIndex>( id );
  osgEarth::Features::Feature *feature = index->getFeature( id );
  osgEarth::Features::FeatureID fid = feature->getFID();
  std::string layerId;
  if ( feature->getUserValue( "qgisLayerId", layerId ) )
  {
    QgsVectorLayer *lyr = QgsProject::instance()->mapLayer<QgsVectorLayer *>( QString::fromStdString( layerId ) );
    if ( lyr )
    {
      QgsFeature feat;
      lyr->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );

      if ( feat.isValid() )
        mRubberBand->setToGeometry( feat.geometry(), lyr );
      else
        mRubberBand->reset( QgsWkbTypes::PolygonGeometry );
    }
  }
  else
  {
    QgsDebugMsg( "Clicked feature was not on a QGIS layer" );
  }

}


void KadasGlobeFeatureIdentifyCallback::onMiss()
{
  mRubberBand->reset( QgsWkbTypes::PolygonGeometry );
}
