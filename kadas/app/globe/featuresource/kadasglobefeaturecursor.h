/***************************************************************************
    kadasglobefeaturecursor.h
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

#ifndef KADASGLOBEFEATURECURSOR_H
#define KADASGLOBEFEATURECURSOR_H

#include <osgEarthFeatures/FeatureCursor>

#include <qgis/qgsfeature.h>
#include <qgis/qgsfeatureiterator.h>
#include <qgis/qgslogger.h>

#include <kadas/app/globe/featuresource/kadasglobefeatureutils.h>


class KadasGlobeFeatureCursor : public osgEarth::Features::FeatureCursor
{
  public:
    KadasGlobeFeatureCursor( QgsVectorLayer *layer, const QgsFeatureIterator &iterator, osgEarth::ProgressCallback *progress )
      : FeatureCursor( progress )
      , mIterator( iterator )
      , mLayer( layer )
    {
      mIterator.nextFeature( mFeature );
    }

    bool hasMore() const override
    {
      return mFeature.isValid();
    }

    osgEarth::Features::Feature *nextFeature() override
    {
      if ( mFeature.isValid() )
      {
        osgEarth::Features::Feature *feat = KadasGlobeFeatureUtils::featureFromQgsFeature( mLayer, mFeature );
        mIterator.nextFeature( mFeature );
        return feat;
      }
      else
      {
        QgsDebugMsg( QStringLiteral( "WARNING: Returning NULL feature to osgEarth" ) );
        return NULL;
      }
    }

  private:
    QgsFeatureIterator mIterator;
    QgsVectorLayer *mLayer = nullptr;
    // Cached feature which will be returned next.
    // Always contains the next feature which will be returned
    // (Because hasMore() needs to know if we are able to return a next feature)
    QgsFeature mFeature;
};

#endif // KADASGLOBEFEATURECURSOR_H
