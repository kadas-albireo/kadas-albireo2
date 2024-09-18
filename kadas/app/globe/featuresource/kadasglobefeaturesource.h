/***************************************************************************
    kadasglobefeaturesource.h
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

#ifndef KADASGLOBEFEATURESOURCE_H
#define KADASGLOBEFEATURESOURCE_H

#include <osgEarthFeatures/FeatureCursor>
#include <osgEarthFeatures/FeatureSource>
#include <osgEarth/Version>
#include <QObject>

#include <qgis/qgsfeatureid.h>
#include <qgis/qgsgeometry.h>

#include <kadas/gui/kadasitemlayer.h>
#include <globe/featuresource/kadasglobefeatureoptions.h>

class KadasGlobeFeatureSource : public QObject, public osgEarth::Features::FeatureSource
{
    Q_OBJECT
  public:
    typedef QMap<osgEarth::Features::FeatureID, osg::ref_ptr<osgEarth::Features::Feature> > FeatureMap;

    KadasGlobeFeatureSource( const KadasGlobeFeatureOptions &options = osgEarth::Features::ConfigOptions() ) : mOptions( options ) {}

    osgEarth::Status initialize( const osgDB::Options *dbOptions ) override;
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
    osgEarth::Features::FeatureCursor *createFeatureCursor( const osgEarth::Symbology::Query &query ) override;
#else
    osgEarth::Features::FeatureCursor *createFeatureCursor( const osgEarth::Symbology::Query &query, osgEarth::ProgressCallback *progress ) override;
#endif

    const char *libraryName() const override { return "Kadas"; }
    int getFeatureCount() const override { return mFeatures.size(); }
    osgEarth::Features::Feature *getFeature( osgEarth::Features::FeatureID fid ) override { return mFeatures.value( fid ); }
    osgEarth::Features::Geometry::Type getGeometryType() const override { return mGeomType; }
    const osgEarth::Features::FeatureSchema &getSchema() const override { return mSchema; }

    const FeatureMap &features() const { return mFeatures; }

    virtual osgEarth::Features::Feature *loadFeature( osgEarth::Features::FeatureID featureId ) = 0;

  protected:
    KadasGlobeFeatureOptions mOptions;
    osgEarth::Features::Geometry::Type mGeomType = osgEarth::Features::Geometry::TYPE_UNKNOWN;
    osgEarth::Features::FeatureSchema mSchema;
    FeatureMap mFeatures;
};


class KadasGlobeFeatureCursor : public osgEarth::Features::FeatureCursor
{
  public:
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
    KadasGlobeFeatureCursor( KadasGlobeFeatureSource *source )
      : mSource( source )
#else
    KadasGlobeFeatureCursor( KadasGlobeFeatureSource * source, osgEarth::ProgressCallback * progress )
      : FeatureCursor( progress ), mSource( source )
#endif
    {
      mIterator = mSource->features().begin();
#if OSGEARTH_VERSION_GREATER_OR_EQUAL(2, 10, 0)
      progress->reportProgress( mCounter, mSource->getFeatureCount() );
#endif
    }

    bool hasMore() const override
    {
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
      return mIterator != mSource->features().end();
#else
      return mIterator != mSource->features().end() && !_progress->isCanceled();
#endif
    }

    osgEarth::Features::Feature *nextFeature() override
    {
      osgEarth::Features::Feature *feat = mIterator.value();
      if ( !feat )
      {
        feat = mSource->loadFeature( mIterator.key() );
      }
#if OSGEARTH_VERSION_GREATER_OR_EQUAL(2, 10, 0)
      _progress->reportProgress( ++mCounter, mSource->getFeatureCount() );
#endif
      ++mIterator;
      return feat;
    }

  private:
    KadasGlobeFeatureSource *mSource;
    KadasGlobeFeatureSource::FeatureMap::const_iterator mIterator;
    int mCounter = 0;
};


class KadasGlobeVectorFeatureSource : public KadasGlobeFeatureSource
{
    Q_OBJECT
  public:
    KadasGlobeVectorFeatureSource( const KadasGlobeFeatureOptions &options = osgEarth::Features::ConfigOptions() );
    const char *className() const override { return "KadasVectorFeatureSource"; }

    osgEarth::Features::Feature *loadFeature( osgEarth::Features::FeatureID featureId ) override;

  private slots:
    void featureAdded( const QgsFeatureId &featureId );
    void featureDeleted( const QgsFeatureId &featureId );
    void attributeValueChanged( const QgsFeatureId &featureId, int idx, const QVariant &value );
    void geometryChanged( const QgsFeatureId &featureId, const QgsGeometry &geometry );
};


class KadasGlobeItemFeatureSource : public KadasGlobeFeatureSource
{
    Q_OBJECT
  public:
    KadasGlobeItemFeatureSource( const KadasGlobeFeatureOptions &options = osgEarth::Features::ConfigOptions() );
    const char *className() const override { return "KadasVectorFeatureSource"; }

    // Per-feature styles
    bool hasEmbeddedStyles() const override { return true; }

    osgEarth::Features::Feature *loadFeature( osgEarth::Features::FeatureID featureId ) override;

  private slots:
    void itemAdded( KadasItemLayer::ItemId itemId );
    void itemRemoved( KadasItemLayer::ItemId itemId );
};

#endif // KADASGLOBEFEATURESOURCE_H
