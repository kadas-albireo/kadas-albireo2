/***************************************************************************
    kadasglobetilesource.h
    ----------------------
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

#ifndef KADASGLOBETILESOURCE_H
#define KADASGLOBETILESOURCE_H

#include <osg/ImageStream>
#include <osgEarth/TileSource>
#include <osgEarth/Version>

#include <QImage>
#include <QStringList>
#include <QLabel>
#include <QMutex>

#include <qgis/qgsrectangle.h>

//#define GLOBE_SHOW_TILE_STATS

class QgsCoordinateTransform;
class QgsMapCanvas;
class QgsMapLayer;
class QgsMapRenderer;
class QgsMapSettings;
class QgsMapRendererParallelJob;
class KadasGlobeTileSource;


class KadasGlobeTileStatistics : public QObject
{
    Q_OBJECT
  public:
    KadasGlobeTileStatistics();
    ~KadasGlobeTileStatistics() { s_instance = 0; }
    static KadasGlobeTileStatistics *instance() { return s_instance; }
    void updateTileCount( int change );
    void updateQueueTileCount( int change );
  signals:
    void changed( int queued, int tot );
  private:
    static KadasGlobeTileStatistics *s_instance;
    QMutex mMutex;
    int mTileCount;
    int mQueueTileCount;
};


class KadasGlobeTileImage : public osg::Image
{
  public:
    KadasGlobeTileImage( KadasGlobeTileSource *tileSource, const QgsRectangle &tileExtent, int tileSize, int tileLod );
    ~KadasGlobeTileImage();
    bool requiresUpdateCall() const { return !mUpdatedImage.isNull(); }
    QgsMapSettings createSettings( int dpi, const QList<QgsMapLayer *> &layers ) const;
    void setUpdatedImage( const QImage &image ) { mUpdatedImage = image; }
    int dpi() const { return mDpi; }
    const QgsRectangle &extent() { return mTileExtent; }

    void update( osg::NodeVisitor * );

    static bool lodSort( const KadasGlobeTileImage *lhs, const KadasGlobeTileImage *rhs ) { return lhs->mLod > rhs->mLod; }

  private:
    osg::ref_ptr<KadasGlobeTileSource> mTileSource;
    QgsRectangle mTileExtent;
    int mTileSize;
    unsigned char *mTileData;
    int mLod;
    int mDpi;
    QImage mUpdatedImage;
};


class KadasGlobeTileUpdateManager : public QObject
{
    Q_OBJECT
  public:
    KadasGlobeTileUpdateManager( QObject *parent = nullptr );
    ~KadasGlobeTileUpdateManager();
    void updateLayerSet( const QList<QgsMapLayer *> &layers ) { mLayers = layers; }
    void addTile( KadasGlobeTileImage *tile );
    void removeTile( KadasGlobeTileImage *tile );
    void waitForFinished() const;

  signals:
    void startRendering();
    void cancelRendering();

  private:
    QList<QgsMapLayer *> mLayers;
    QList<KadasGlobeTileImage *> mTileQueue;
    KadasGlobeTileImage *mCurrentTile = nullptr;
    QgsMapRendererParallelJob *mRenderer = nullptr;

  private slots:
    void start();
    void cancel();
    void renderingFinished();
};

class KadasGlobeTileSource : public osgEarth::TileSource
{
  public:
    KadasGlobeTileSource( QgsMapCanvas *canvas, const osgEarth::TileSourceOptions &options = osgEarth::TileSourceOptions() );
    osgEarth::Status initialize( const osgDB::Options *dbOptions ) override;
    osg::Image *createImage( const osgEarth::TileKey &key, osgEarth::ProgressCallback *progress ) override;
    osg::HeightField *createHeightField( const osgEarth::TileKey &/*key*/, osgEarth::ProgressCallback * /*progress*/ ) override { return 0; }

    bool isDynamic() const override { return true; }
    osgEarth::CachePolicy getCachePolicyHint( const osgEarth::Profile * /*profile*/ ) const override { return osgEarth::CachePolicy::NO_CACHE; }

    void refresh( const QgsRectangle &dirtyExtent );
    void setLayers( const QList<QgsMapLayer *> &layers ) { mLayers = layers; }
    const QList<QgsMapLayer *> &layers() const { return mLayers; }

    void waitForFinished() const
    {
      mTileUpdateManager.waitForFinished();
    }

  private:
    friend class KadasGlobeTileImage;

    QMutex mTileListLock;
    QList<KadasGlobeTileImage *> mTiles;
    QgsMapCanvas *mCanvas = nullptr;
    QList<QgsMapLayer *> mLayers;
    KadasGlobeTileUpdateManager mTileUpdateManager;

    void addTile( KadasGlobeTileImage *tile );
    void removeTile( KadasGlobeTileImage *tile );
};

#endif // KADASGLOBETILESOURCE_H
