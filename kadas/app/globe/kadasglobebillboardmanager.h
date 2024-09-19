/***************************************************************************
    kadasglobebillboardmanager.h
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

#ifndef KADASGLOBEBILLBOARDMANAGER_H
#define KADASGLOBEBILLBOARDMANAGER_H

#include <QMap>
#include <osgEarthAnnotation/PlaceNode>

#include "kadas/gui/kadasitemlayer.h"

class KadasGlobeBillboardManager : public QObject
{
    Q_OBJECT

  public:
    KadasGlobeBillboardManager( QObject *parent = nullptr );
    void init( osg::ref_ptr<osgEarth::MapNode> mapNode, const QStringList &visibleLayerIds );
    void reset();
    void updateLayers( const QStringList &layerIds );

  private slots:
    void addCanvasBillboard( const KadasMapItem *item );
    void removeCanvasBillboard( const KadasMapItem *item );
    void updateCanvasBillboard();

  private:
    void addLayerBillboard( const QString &layerId, KadasItemLayer::ItemId itemId );
    void removeLayerBillboard( const QString &layerId, KadasItemLayer::ItemId itemId );

    osg::ref_ptr<osgEarth::Annotation::PlaceNode> createBillboard( const KadasMapItem *item );

    typedef QMap<QString, QMap<KadasItemLayer::ItemId, osg::ref_ptr<osgEarth::Annotation::PlaceNode>>> BillboardRegistry;
    BillboardRegistry mRegistry;
    QMap<const KadasMapItem *, osg::ref_ptr<osgEarth::Annotation::PlaceNode>> mCanvasItemsRegistry;
    osg::ref_ptr<osgEarth::MapNode> mMapNode;
    osg::ref_ptr<osg::Group> mGroup;
    QObject *mSignalScope = nullptr;
    QStringList mCurrentLayers;
};

#endif // KADASGLOBEBILLBOARDMANAGER_H
