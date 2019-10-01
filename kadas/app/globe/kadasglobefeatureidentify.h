/***************************************************************************
    kadasglobefeatureidentify.h
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

#ifndef KADASGLOBEFEATUREIDENTIFY_H
#define KADASGLOBEFEATUREIDENTIFY_H

#include <osgEarth/Picker>

class QgsMapCanvas;
class QgsRubberBand;

class KadasGlobeFeatureIdentifyCallback : public osgEarth::Picker::Callback
{
  public:
    KadasGlobeFeatureIdentifyCallback( QgsMapCanvas *mapCanvas );
    ~KadasGlobeFeatureIdentifyCallback();

    void onHit( osgEarth::ObjectID id ) override;
    void onMiss() override;

  private:
    QgsMapCanvas *mCanvas = nullptr;
    QgsRubberBand *mRubberBand = nullptr;
};

#endif // KADASGLOBEFEATUREIDENTIFY_H
