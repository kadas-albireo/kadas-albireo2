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
#include <osgEarth/Version>
#include <osgEarthFeatures/FeatureSource>

class QgsMapLayer;

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
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
      conf.updateNonSerializable( "layer", new RefPtr< QgsMapLayer >( mLayer ) );
#else
      conf.setNonSerializable( "layer", new RefPtr< QgsMapLayer >( mLayer ) );
#endif
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
      conf.setObj( "style", mStyle );
#else
      conf.set( "style", mStyle );
#endif
      return conf;
    }

    osgEarth::optional<std::string> &layerId() { return mLayerId; }
    const osgEarth::optional<std::string> &layerId() const { return mLayerId; }

    QgsMapLayer *layer() const { return mLayer; }
    void setLayer( QgsMapLayer *layer ) { mLayer = layer; }

    osgEarth::optional<osgEarth::Style> &style() { return mStyle; }
    const osgEarth::optional<osgEarth::Style> &style() const { return mStyle; }

  protected:
    void mergeConfig( const osgEarth::Config &conf ) override
    {
      osgEarth::Features::FeatureSourceOptions::mergeConfig( conf );
      fromConfig( conf );
    }

  private:
    void fromConfig( const osgEarth::Config &conf )
    {
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
      conf.getIfSet( "layerId", mLayerId );
#else
      conf.get( "layerId", mLayerId );
#endif
      RefPtr< QgsMapLayer > *layer_ptr = conf.getNonSerializable< RefPtr< QgsMapLayer > >( "layer" );
      mLayer = layer_ptr ? layer_ptr->ptr() : nullptr;
#if OSGEARTH_VERSION_LESS_THAN(2, 10, 0)
      conf.getObjIfSet( "style", mStyle );
#else
      conf.get( "style", mStyle );
#endif
    }

    osgEarth::optional<std::string> mLayerId;
    QgsMapLayer *mLayer = nullptr;
    osgEarth::optional<osgEarth::Style> mStyle;
};

#endif // KADASGLOBEFEATUREOPTIONS_H
