/***************************************************************************
    kadasglobefeaturesource.cpp
    ---------------------------
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

#include <osgDB/ReaderWriter>
#include <osgDB/FileNameUtils>

#include <qgis/qgsfeature.h>
#include <qgis/qgsfeatureiterator.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadasgeometryitem.h>
#include <kadas/app/globe/featuresource/kadasglobefeaturesource.h>
#include <kadas/app/globe/featuresource/kadasglobefeatureutils.h>


osgEarth::Status KadasGlobeFeatureSource::initialize( const osgDB::Options *dbOptions )
{
  osgEarth::SpatialReference *ref = osgEarth::SpatialReference::create( mOptions.layer()->crs().toWkt().toStdString() );
  if ( 0 == ref )
  {
    std::cout << "Cannot find the spatial reference" << std::endl;
    return osgEarth::Status( osgEarth::Status::ConfigurationError );
  }
  QgsRectangle ext = mOptions.layer()->extent();
  osgEarth::GeoExtent geoext( ref, ext.xMinimum(), ext.yMinimum(), ext.xMaximum(), ext.yMaximum() );
  setFeatureProfile( new osgEarth::Features::FeatureProfile( geoext ) );
  return osgEarth::Status::NoError;
}

osgEarth::Features::FeatureCursor *KadasGlobeFeatureSource::createFeatureCursor( const osgEarth::Symbology::Query &query, osgEarth::ProgressCallback *progress )
{
  return new KadasGlobeFeatureCursor( this, progress );
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeVectorFeatureSource::KadasGlobeVectorFeatureSource( const KadasGlobeFeatureOptions &options )
  : KadasGlobeFeatureSource( options )
{
  QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( mOptions.layer() );
  connect( layer, &QgsVectorLayer::featureAdded, this, &KadasGlobeVectorFeatureSource::featureAdded );
  connect( layer, &QgsVectorLayer::featureDeleted, this, &KadasGlobeVectorFeatureSource::featureDeleted );
  connect( layer, &QgsVectorLayer::attributeValueChanged, this, &KadasGlobeVectorFeatureSource::attributeValueChanged );
  connect( layer, &QgsVectorLayer::geometryChanged, this, &KadasGlobeVectorFeatureSource::geometryChanged );

  switch ( layer->geometryType() )
  {
    case  QgsWkbTypes::PointGeometry:
      mGeomType = osgEarth::Features::Geometry::TYPE_POINTSET; break;
    case QgsWkbTypes::LineGeometry:
      mGeomType = osgEarth::Features::Geometry::TYPE_LINESTRING; break;
    case QgsWkbTypes::PolygonGeometry:
      mGeomType = osgEarth::Features::Geometry::TYPE_POLYGON; break;
    default:
      mGeomType = osgEarth::Features::Geometry::TYPE_UNKNOWN; break;
  }

  mSchema = KadasGlobeFeatureUtils::schemaForFields( layer->fields() );

  // Populate initial cache with featureIds, features are build on-demand
  for ( const QgsFeatureId &featureId : layer->allFeatureIds() )
  {
    mFeatures.insert( featureId, nullptr );
  }
}

osgEarth::Features::Feature *KadasGlobeVectorFeatureSource::loadFeature( osgEarth::Features::FeatureID featureId )
{
  QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( mOptions.layer() );
  QgsFeature feature = layer->getFeature( featureId );
  osgEarth::Features::Feature *feat = KadasGlobeFeatureUtils::featureFromQgsFeature( layer, feature );
  mFeatures.insert( featureId, feat );
  return feat;
}

void KadasGlobeVectorFeatureSource::featureAdded( const QgsFeatureId &featureId )
{
  loadFeature( featureId );
}

void KadasGlobeVectorFeatureSource::featureDeleted( const QgsFeatureId &featureId )
{
  mFeatures.remove( featureId );
}

void KadasGlobeVectorFeatureSource::attributeValueChanged( const QgsFeatureId &featureId, int idx, const QVariant &value )
{
  QgsVectorLayer *layer = qobject_cast<QgsVectorLayer *>( mOptions.layer() );
  FeatureMap::iterator it = mFeatures.find( featureId );
  if ( it != mFeatures.end() )
  {
    osgEarth::Features::Feature *feature = it.value().get();
    KadasGlobeFeatureUtils::setFeatureField( feature, layer->fields().at( idx ), value );
  }
}

void KadasGlobeVectorFeatureSource::geometryChanged( const QgsFeatureId &featureId, const QgsGeometry &geometry )
{
  FeatureMap::iterator it = mFeatures.find( featureId );
  if ( it != mFeatures.end() )
  {
    osgEarth::Features::Feature *feature = it.value().get();
    feature->setGeometry( KadasGlobeFeatureUtils::geometryFromQgsGeometry( geometry.constGet() ) );
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeItemFeatureSource::KadasGlobeItemFeatureSource( const KadasGlobeFeatureOptions &options )
  : KadasGlobeFeatureSource( options )
{
  KadasItemLayer *layer = qobject_cast<KadasItemLayer *>( mOptions.layer() );

  connect( layer, &KadasItemLayer::itemAdded, this, &KadasGlobeItemFeatureSource::itemAdded );
  connect( layer, &KadasItemLayer::itemRemoved, this, &KadasGlobeItemFeatureSource::itemRemoved );

  // Populate initial cache with featureIds, features are build on-demand
  for ( auto it = layer->items().begin(), itEnd = layer->items().end(); it != itEnd; ++it )
  {
    if ( dynamic_cast<KadasGeometryItem *>( it.value() ) )
    {
      mFeatures.insert( it.key(), nullptr );
    }
  }
}

osgEarth::Features::Feature *KadasGlobeItemFeatureSource::loadFeature( osgEarth::Features::FeatureID featureId )
{
  KadasItemLayer *layer = qobject_cast<KadasItemLayer *>( mOptions.layer() );

  KadasGeometryItem *item = dynamic_cast<KadasGeometryItem *>( layer->items()[featureId] );
  if ( !item )
  {
    return nullptr;
  }
  osgEarth::Features::Feature *feat = KadasGlobeFeatureUtils::featureFromItem( layer->id(), featureId, item, mOptions.style().get() );
  mFeatures.insert( featureId, feat );
  return feat;
}

void KadasGlobeItemFeatureSource::itemAdded( KadasItemLayer::ItemId itemId )
{
  loadFeature( itemId );
}

void KadasGlobeItemFeatureSource::itemRemoved( KadasItemLayer::ItemId itemId )
{
  mFeatures.remove( itemId );
}

///////////////////////////////////////////////////////////////////////////////

class KadasGlobeFeatureSourceFactory : public osgEarth::Features::FeatureSourceDriver
{
  public:
    KadasGlobeFeatureSourceFactory()
    {
      supportsExtension( "osgearth_feature_qgis", "QGIS feature driver for osgEarth" );
    }

    osgDB::ReaderWriter::ReadResult readObject( const std::string &file_name, const osgDB::Options *options ) const override
    {
      // this function seems to be called for every plugin
      // we declare supporting the special extension "osgearth_feature_qgis"
      if ( !acceptsExtension( osgDB::getLowerCaseFileExtension( file_name ) ) )
      {
        return osgDB::ReaderWriter::ReadResult::FILE_NOT_HANDLED;
      }
      KadasGlobeFeatureOptions featureOptions = getFeatureSourceOptions( options );

      if ( dynamic_cast<QgsVectorLayer *>( featureOptions.layer() ) )
      {
        return osgDB::ReaderWriter::ReadResult( new KadasGlobeVectorFeatureSource( featureOptions ) );
      }
      else
      {
        return osgDB::ReaderWriter::ReadResult( new KadasGlobeItemFeatureSource( featureOptions ) );
      }
    }
};

REGISTER_OSGPLUGIN( osgearth_feature_qgis, KadasGlobeFeatureSourceFactory )
