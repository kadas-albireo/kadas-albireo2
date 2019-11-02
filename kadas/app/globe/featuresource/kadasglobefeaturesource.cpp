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

#include <kadas/app/globe/featuresource/kadasglobefeaturecursor.h>
#include <kadas/app/globe/featuresource/kadasglobefeaturesource.h>
#include <kadas/app/globe/featuresource/kadasglobefeatureutils.h>


KadasGlobeFeatureSource::KadasGlobeFeatureSource( const KadasGlobeFeatureOptions &options )
  : mOptions( options )
  , mLayer( 0 )
{
}

osgEarth::Status KadasGlobeFeatureSource::initialize( const osgDB::Options *dbOptions )
{
  Q_UNUSED( dbOptions )
  mLayer = mOptions.layer();

  connect( mLayer, &QgsVectorLayer::attributeValueChanged, this, &KadasGlobeFeatureSource::attributeValueChanged );
  connect( mLayer, &QgsVectorLayer::geometryChanged, this, &KadasGlobeFeatureSource::geometryChanged );

  // create the profile
  osgEarth::SpatialReference *ref = osgEarth::SpatialReference::create( mLayer->crs().toWkt().toStdString() );
  if ( 0 == ref )
  {
    std::cout << "Cannot find the spatial reference" << std::endl;
    return osgEarth::Status( osgEarth::Status::ConfigurationError );
  }
  QgsRectangle ext = mLayer->extent();
  osgEarth::GeoExtent geoext( ref, ext.xMinimum(), ext.yMinimum(), ext.xMaximum(), ext.yMaximum() );
  mSchema = KadasGlobeFeatureUtils::schemaForFields( mLayer->fields() );
  setFeatureProfile( new osgEarth::Features::FeatureProfile( geoext ) );
  return osgEarth::Status( osgEarth::Status::NoError );
}

osgEarth::Features::FeatureCursor *KadasGlobeFeatureSource::createFeatureCursor( const osgEarth::Symbology::Query &query, osgEarth::ProgressCallback *progress )
{
  QgsFeatureRequest request;

  if ( query.expression().isSet() )
  {
    QgsDebugMsg( QString( "Ignoring query expression '%1'" ). arg( query.expression().value().c_str() ) );
  }

  if ( query.bounds().isSet() )
  {
    QgsRectangle bounds( query.bounds()->xMin(), query.bounds()->yMin(), query.bounds()->xMax(), query.bounds()->yMax() );
    request.setFilterRect( bounds );
  }

  QgsFeatureIterator it = mLayer->getFeatures( request );
  return new KadasGlobeFeatureCursor( mLayer, it, progress );
}

osgEarth::Features::Feature *KadasGlobeFeatureSource::getFeature( osgEarth::Features::FeatureID fid )
{
  QgsFeature feat;
  mLayer->getFeatures( QgsFeatureRequest().setFilterFid( fid ) ).nextFeature( feat );
  osgEarth::Features::Feature *feature = KadasGlobeFeatureUtils::featureFromQgsFeature( mLayer, feat );
  FeatureMap_t::iterator it = mFeatures.find( fid );
  if ( it == mFeatures.end() )
  {
    mFeatures.insert( std::make_pair( fid, osg::observer_ptr<osgEarth::Features::Feature>( feature ) ) );
  }
  else
  {
    it->second = osg::observer_ptr<osgEarth::Features::Feature>( feature );
  }
  return feature;
}

osgEarth::Features::Geometry::Type KadasGlobeFeatureSource::getGeometryType() const
{
  switch ( mLayer->geometryType() )
  {
    case  QgsWkbTypes::PointGeometry:
      return osgEarth::Features::Geometry::TYPE_POINTSET;

    case QgsWkbTypes::LineGeometry:
      return osgEarth::Features::Geometry::TYPE_LINESTRING;

    case QgsWkbTypes::PolygonGeometry:
      return osgEarth::Features::Geometry::TYPE_POLYGON;

    default:
      return osgEarth::Features::Geometry::TYPE_UNKNOWN;
  }

  return osgEarth::Features::Geometry::TYPE_UNKNOWN;
}

int KadasGlobeFeatureSource::getFeatureCount() const
{
  return mLayer->featureCount();
}

void KadasGlobeFeatureSource::attributeValueChanged( const QgsFeatureId &featureId, int idx, const QVariant &value )
{
  FeatureMap_t::iterator it = mFeatures.find( featureId );
  if ( it != mFeatures.end() )
  {
    osgEarth::Features::Feature *feature = it->second.get();
    KadasGlobeFeatureUtils::setFeatureField( feature, mLayer->fields().at( idx ), value );
  }
}

void KadasGlobeFeatureSource::geometryChanged( const QgsFeatureId &featureId, const QgsGeometry &geometry )
{
  FeatureMap_t::iterator it = mFeatures.find( featureId );
  if ( it != mFeatures.end() )
  {
    osgEarth::Features::Feature *feature = it->second.get();
    feature->setGeometry( KadasGlobeFeatureUtils::geometryFromQgsGeometry( geometry ) );
  }
}


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
        return osgDB::ReaderWriter::ReadResult::FILE_NOT_HANDLED;

      return osgDB::ReaderWriter::ReadResult( new KadasGlobeFeatureSource( getFeatureSourceOptions( options ) ) );
    }
};

REGISTER_OSGPLUGIN( osgearth_feature_qgis, KadasGlobeFeatureSourceFactory )
