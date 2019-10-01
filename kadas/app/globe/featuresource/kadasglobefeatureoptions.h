/***************************************************************************
    kadasglobefeatureoptions.h
    --------------------------
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

#ifndef KADASGLOBEFEATUREOPTIONS_H
#define KADASGLOBEFEATUREOPTIONS_H

#include <osgEarth/Common>
#include <osgEarthFeatures/FeatureSource>

class QgsVectorLayer;

class KadasGlobeFeatureOptions : public osgEarth::Features::FeatureSourceOptions
{
  private:
    template <class T>
    class RefPtr : public osg::Referenced
    {
      public:
        RefPtr( T *ptr ) : mPtr( ptr ) {}
        T *ptr() { return mPtr; }

      private:
        T *mPtr = nullptr;
    };

  public:
    KadasGlobeFeatureOptions( const ConfigOptions &opt = ConfigOptions() )
      : osgEarth::Features::FeatureSourceOptions( opt )
    {
      // Call the driver declared as "osgearth_feature_qgis"
      setDriver( "qgis" );
      fromConfig( _conf );
    }

    osgEarth::Config getConfig() const override
    {
      osgEarth::Config conf = osgEarth::Features::FeatureSourceOptions::getConfig();
      conf.set( "layerId", mLayerId );
      conf.setNonSerializable( "layer", new RefPtr< QgsVectorLayer >( mLayer ) );
      return conf;
    }

    osgEarth::optional<std::string> &layerId() { return mLayerId; }
    const osgEarth::optional<std::string> &layerId() const { return mLayerId; }

    QgsVectorLayer *layer() const { return mLayer; }
    void setLayer( QgsVectorLayer *layer ) { mLayer = layer; }

  protected:
    void mergeConfig( const osgEarth::Config &conf ) override
    {
      osgEarth::Features::FeatureSourceOptions::mergeConfig( conf );
      fromConfig( conf );
    }

  private:
    void fromConfig( const osgEarth::Config &conf )
    {
      conf.get( "layerId", mLayerId );
      RefPtr< QgsVectorLayer > *layer_ptr = conf.getNonSerializable< RefPtr< QgsVectorLayer > >( "layer" );
      mLayer = layer_ptr ? layer_ptr->ptr() : 0;
    }

    osgEarth::optional<std::string> mLayerId;
    QgsVectorLayer *mLayer = nullptr;
};

#endif // KADASGLOBEFEATUREOPTIONS_H
