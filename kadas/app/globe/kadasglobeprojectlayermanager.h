/***************************************************************************
    kadasglobeprojectlayermanager.h
    -------------------------------
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

#ifndef KADASGLOBEPROJECTLAYERMANAGER_H
#define KADASGLOBEPROJECTLAYERMANAGER_H

#include <QObject>

#include <osg/ref_ptr>
#include <osgEarth/MapNode>
#include <osgEarth/ImageLayer>

#include <kadas/app/globe/kadasglobetilesource.h>


class KadasGlobeProjectLayerManager : public QObject
{
    Q_OBJECT

  public:
    KadasGlobeProjectLayerManager( QObject *parent ) : QObject( parent ) {}
    void init( osg::ref_ptr<osgEarth::MapNode> mapNode, const QStringList &visibleLayerIds );
    void reset();
    osg::ref_ptr<osgEarth::ImageLayer> drapedLayer() const { return mDrapedLayer; }

  public slots:
    void updateLayers( const QStringList &visibleLayerIds );

  private:
    osg::ref_ptr<osgEarth::MapNode> mMapNode;
    osg::ref_ptr<osgEarth::ImageLayer> mDrapedLayer;
    osg::ref_ptr<KadasGlobeTileSource> mTileSource;
    QObject *mLayerSignalScope = nullptr;
    QStringList mCurrentVisibleLayerIds;

    void addModelLayer( const QString &layerId );
    void updateLayer( const QString &layerId );
};

#endif // KADASGLOBEPROJECTLAYERMANAGER_H
