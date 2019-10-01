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

#include <osgEarthFeatures/FeatureSource>
#include <osgEarth/Version>
#include <QObject>

#include <qgis/qgsfeatureid.h>
#include <qgis/qgsgeometry.h>

#include <kadas/app/globe/featuresource/kadasglobefeatureoptions.h>


class KadasGlobeFeatureSource : public QObject, public osgEarth::Features::FeatureSource
{
    Q_OBJECT
  public:
    KadasGlobeFeatureSource( const KadasGlobeFeatureOptions &options = osgEarth::Features::ConfigOptions() );

    osgEarth::Features::FeatureCursor *createFeatureCursor( const osgEarth::Symbology::Query &query, osgEarth::ProgressCallback *progress ) override;

    int getFeatureCount() const override;
    osgEarth::Features::Feature *getFeature( osgEarth::Features::FeatureID fid ) override;
    osgEarth::Features::Geometry::Type getGeometryType() const override;

    QgsVectorLayer *layer() const { return mLayer; }

    const char *className() const override { return "QGISFeatureSource"; }
    const char *libraryName() const override { return "QGIS"; }

    osgEarth::Status initialize( const osgDB::Options *dbOptions ) override;

  protected:
    const osgEarth::Features::FeatureSchema &getSchema() const override { return mSchema; }

    ~KadasGlobeFeatureSource() {}

  private:
    KadasGlobeFeatureOptions mOptions;
    QgsVectorLayer *mLayer = nullptr;
    osgEarth::Features::FeatureSchema mSchema;
    typedef std::map<osgEarth::Features::FeatureID, osg::observer_ptr<osgEarth::Features::Feature> > FeatureMap_t;
    FeatureMap_t mFeatures;

  private slots:
    void attributeValueChanged( const QgsFeatureId &featureId, int idx, const QVariant &value );
    void geometryChanged( const QgsFeatureId &featureId, const QgsGeometry &geometry );
};

#endif // KADASGLOBEFEATURESOURCE_H
