/***************************************************************************
    kadasglobefrustumhighlight.h
    ----------------------------
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

#ifndef KADASGLOBEFRUSTUMHIGHLIGHT_H
#define KADASGLOBEFRUSTUMHIGHLIGHT_H

#include <QColor>

#include <osg/Callback>

class QgsMapCanvas;
class QgsRubberBand;
namespace osg { class View; }
namespace osgEarth
{
  class Terrain;
  class SpatialReference;
}

struct KadasGlobeFrustumHighlightCallback : public osg::Callback
{
  public:
    KadasGlobeFrustumHighlightCallback( osg::View *view, osgEarth::Terrain *terrain, QgsMapCanvas *mapCanvas, QColor color );
    ~KadasGlobeFrustumHighlightCallback();

    bool run( osg::Object *object, osg::Object *data ) override;

  private:
    osg::View *mView = nullptr;
    osgEarth::Terrain *mTerrain = nullptr;
    QgsRubberBand *mRubberBand = nullptr;
    osgEarth::SpatialReference *mSrs = nullptr;
};

#endif // KADASGLOBEFRUSTUMHIGHLIGHT_H
