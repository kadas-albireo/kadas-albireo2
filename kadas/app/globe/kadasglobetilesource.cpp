/***************************************************************************
    kadasglobetilesource.cpp
    ------------------------
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

#include <osgEarth/Registry>
#include <osgEarth/ImageUtils>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgsmaprenderercustompainterjob.h>
#include <qgis/qgsmaprendererparalleljob.h>
#include <qgis/qgsproject.h>

#include <kadas/app/globe/kadasglobetilesource.h>


KadasGlobeTileStatistics *KadasGlobeTileStatistics::s_instance = 0;

KadasGlobeTileStatistics::KadasGlobeTileStatistics() : mTileCount( 0 ), mQueueTileCount( 0 )
{
  s_instance =  this;
}

void KadasGlobeTileStatistics::updateTileCount( int change )
{
  mMutex.lock();
  mTileCount += change;
  emit changed( mQueueTileCount, mTileCount );
  mMutex.unlock();
}

void KadasGlobeTileStatistics::updateQueueTileCount( int num )
{
  mMutex.lock();
  mQueueTileCount = num;
  emit changed( mQueueTileCount, mTileCount );
  mMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeTileImage::KadasGlobeTileImage( KadasGlobeTileSource *tileSource, const QgsRectangle &tileExtent, int tileSize, int tileLod )
  : osg::Image()
  , mTileSource( tileSource )
  , mTileExtent( tileExtent )
  , mTileSize( tileSize )
  , mLod( tileLod )
{
  mTileSource->addTile( this );
#ifdef GLOBE_SHOW_TILE_STATS
  KadasGlobeTileStatistics::instance()->updateTileCount( + 1 );
#endif
  mTileData = new unsigned char[mTileSize * mTileSize * 4];
  std::memset( mTileData, 0, mTileSize * mTileSize * 4 );
#if 0
  setImage( mTileSize, mTileSize, 1, 4, // width, height, depth, internal_format
            GL_BGRA, GL_UNSIGNED_BYTE,
            mTileData, osg::Image::NO_DELETE );

  mTileSource->mTileUpdateManager.addTile( const_cast<KadasGlobeTileImage *>( this ) );
  mDpi = 72;
#else
  QList<QgsMapLayer *> layers;
  for ( const QString &layerId : mTileSource->layers() )
  {
    QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
    if ( layer )
    {
      layers.append( layer );
    }
  }
  QImage qImage( mTileData, mTileSize, mTileSize, QImage::Format_ARGB32_Premultiplied );
  QPainter painter( &qImage );
  QgsMapRendererCustomPainterJob job( createSettings( qImage.logicalDpiX(), layers ), &painter );
  job.renderSynchronously();

  setImage( mTileSize, mTileSize, 1, 4, // width, height, depth, internal_format
            GL_BGRA, GL_UNSIGNED_BYTE,
            mTileData, osg::Image::NO_DELETE );
  flipVertical();
  mDpi = qImage.logicalDpiX();
#endif
}

KadasGlobeTileImage::~KadasGlobeTileImage()
{
  mTileSource->removeTile( this );
  mTileSource->mTileUpdateManager.removeTile( this );
  delete[] mTileData;
#ifdef GLOBE_SHOW_TILE_STATS
  KadasGlobeTileStatistics::instance()->updateTileCount( -1 );
#endif
}

QgsMapSettings KadasGlobeTileImage::createSettings( int dpi, const QList<QgsMapLayer *> &layers ) const
{
  QgsMapSettings settings;
  settings.setBackgroundColor( QColor( Qt::transparent ) );
  settings.setDestinationCrs( QgsCoordinateReferenceSystem::fromOgcWmsCrs( GEO_EPSG_CRS_AUTHID ) );
  settings.setExtent( mTileExtent );
  settings.setLayers( layers );
  settings.setFlag( QgsMapSettings::DrawEditingInfo, false );
  settings.setFlag( QgsMapSettings::DrawLabeling, false );
  settings.setFlag( QgsMapSettings::DrawSelection, false );
  settings.setOutputSize( QSize( mTileSize, mTileSize ) );
  settings.setOutputImageFormat( QImage::Format_ARGB32_Premultiplied );
  settings.setOutputDpi( dpi );
  settings.setCustomRenderFlags( "globe" );
  return settings;
}

void KadasGlobeTileImage::update( osg::NodeVisitor * )
{
  if ( !mUpdatedImage.isNull() )
  {
    QgsDebugMsg( QString( "Updating earth tile image: %1" ).arg( mTileExtent.toString( 5 ) ) );
    std::memcpy( mTileData, mUpdatedImage.bits(), mTileSize * mTileSize * 4 );
    setImage( mTileSize, mTileSize, 1, 4, // width, height, depth, internal_format
              GL_BGRA, GL_UNSIGNED_BYTE,
              mTileData, osg::Image::NO_DELETE );
    flipVertical();
    mUpdatedImage = QImage();
  }
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeTileUpdateManager::KadasGlobeTileUpdateManager( QObject *parent )
  : QObject( parent )
{
  connect( this, &KadasGlobeTileUpdateManager::startRendering, this, &KadasGlobeTileUpdateManager::start );
  connect( this, &KadasGlobeTileUpdateManager::cancelRendering, this, &KadasGlobeTileUpdateManager::cancel );
}

KadasGlobeTileUpdateManager::~KadasGlobeTileUpdateManager()
{
#ifdef GLOBE_SHOW_TILE_STATS
  KadasGlobeTileStatistics::instance()->updateQueueTileCount( 0 );
#endif
  mTileQueue.clear();
  mCurrentTile = nullptr;
  if ( mRenderer )
  {
    mRenderer->cancel();
  }
}

void KadasGlobeTileUpdateManager::addTile( KadasGlobeTileImage *tile )
{
  if ( !mTileQueue.contains( tile ) )
  {
    mTileQueue.append( tile );
#ifdef GLOBE_SHOW_TILE_STATS
    KadasGlobeTileStatistics::instance()->updateQueueTileCount( mTileQueue.size() );
#endif
    std::sort( mTileQueue.begin(), mTileQueue.end(), KadasGlobeTileImage::lodSort );
  }
  emit startRendering();
}

void KadasGlobeTileUpdateManager::removeTile( KadasGlobeTileImage *tile )
{
  if ( mCurrentTile == tile )
  {
    mCurrentTile = nullptr;
    if ( mRenderer )
      emit cancelRendering();
  }
  else if ( mTileQueue.contains( tile ) )
  {
    mTileQueue.removeAll( tile );
#ifdef GLOBE_SHOW_TILE_STATS
    KadasGlobeTileStatistics::instance()->updateQueueTileCount( mTileQueue.size() );
#endif
  }
}

void KadasGlobeTileUpdateManager::waitForFinished() const
{
  if ( mRenderer )
  {
    mRenderer->waitForFinished();
  }
}

void KadasGlobeTileUpdateManager::start()
{
  if ( mRenderer == nullptr && !mTileQueue.isEmpty() )
  {
    mCurrentTile = mTileQueue.takeFirst();
#ifdef GLOBE_SHOW_TILE_STATS
    KadasGlobeTileStatistics::instance()->updateQueueTileCount( mTileQueue.size() );
#endif
    QList<QgsMapLayer *> layers;
    for ( const QString &layerId : mLayerIds )
    {
      QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
      if ( layer )
      {
        layers.append( layer );
      }
    }
    mRenderer = new QgsMapRendererParallelJob( mCurrentTile->createSettings( mCurrentTile->dpi(), layers ) );
    connect( mRenderer, &QgsMapRendererParallelJob::finished, this, &KadasGlobeTileUpdateManager::renderingFinished );
    mRenderer->start();
  }
}

void KadasGlobeTileUpdateManager::cancel()
{
  if ( mRenderer )
    mRenderer->cancel();
}

void KadasGlobeTileUpdateManager::renderingFinished()
{
  if ( mCurrentTile )
  {
    QImage image = mRenderer->renderedImage();
    mCurrentTile->setUpdatedImage( image );
    mCurrentTile = nullptr;
  }
  mRenderer->deleteLater();
  mRenderer = nullptr;
  start();
}

///////////////////////////////////////////////////////////////////////////////

KadasGlobeTileSource::KadasGlobeTileSource( const osgEarth::TileSourceOptions &options )
  : TileSource( options )
{
  osgEarth::GeoExtent geoextent( osgEarth::SpatialReference::get( "wgs84" ), -180., -90., 180., 90. );
  osgEarth::DataExtentList extents;
  extents.push_back( geoextent );
  getDataExtents() = extents;
}

osgEarth::Status KadasGlobeTileSource::initialize( const osgDB::Options * /*dbOptions*/ )
{
  setProfile( osgEarth::Registry::instance()->getGlobalGeodeticProfile() );
  return osgEarth::Status( osgEarth::Status::NoError );
}

osg::Image *KadasGlobeTileSource::createImage( const osgEarth::TileKey &key, osgEarth::ProgressCallback *progress )
{
  Q_UNUSED( progress )

  int tileSize = getPixelsPerTile();
  if ( tileSize <= 0 )
  {
    return osgEarth::ImageUtils::createEmptyImage();
  }

  double xmin, ymin, xmax, ymax;
  key.getExtent().getBounds( xmin, ymin, xmax, ymax );
  QgsRectangle tileExtent( xmin, ymin, xmax, ymax );

  QgsDebugMsg( QString( "Create earth tile image: %1" ).arg( tileExtent.toString( 5 ) ) );
  return new KadasGlobeTileImage( this, tileExtent, getPixelsPerTile(), key.getLOD() );
}

void KadasGlobeTileSource::refresh( const QgsRectangle &dirtyExtent )
{
  mTileListLock.lock();
  for ( KadasGlobeTileImage *tile : mTiles )
  {
    if ( tile->extent().intersects( dirtyExtent ) )
    {
      mTileUpdateManager.addTile( tile );
    }
  }
  mTileListLock.unlock();
}

void KadasGlobeTileSource::setLayers( const QList<QString> &layerIds )
{
  // Compute damaged extent
  QgsRectangle dirtyRect;
  QgsCoordinateReferenceSystem crs84( "EPSG:4326" );
  // Damage extent of draped layer: removed and added layers
  QSet<QString> oldLayers( mLayerIds.begin(), mLayerIds.end() );
  QSet<QString> newLayers( layerIds.begin(), layerIds.end() );
  QSet<QString> changedLayers = QSet<QString>( oldLayers ).subtract( newLayers ).unite( QSet<QString>( newLayers ).subtract( oldLayers ) );
  for ( const QString &layerId : changedLayers )
  {
    QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
    if ( layer )
    {
      QgsRectangle layerExtent = QgsCoordinateTransform( layer->crs(), crs84, QgsProject::instance() ).transformBoundingBox( layer->extent() );
      if ( dirtyRect.isNull() )
        dirtyRect = layerExtent;
      else
        dirtyRect.combineExtentWith( layerExtent );
    }
  }

  // Update layers and refresh
  mLayerIds = layerIds;
  mTileUpdateManager.updateLayerSet( layerIds );
  refresh( dirtyRect );
}

void KadasGlobeTileSource::addTile( KadasGlobeTileImage *tile )
{
  mTileListLock.lock();
  mTiles.append( tile );
  mTileListLock.unlock();
}

void KadasGlobeTileSource::removeTile( KadasGlobeTileImage *tile )
{
  mTileListLock.lock();
  mTiles.removeOne( tile );
  mTileListLock.unlock();
}
