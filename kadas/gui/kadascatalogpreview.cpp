/***************************************************************************
    kadascatalogpreview.cpp
    -----------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QThread>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaprendererparalleljob.h>
#include <qgis/qgsmessagebar.h>
#include <qgis/qgsmessagebaritem.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectortilelayer.h>

#include "kadas/gui/kadascatalogpreview.h"
#include "kadas/gui/kadascatalogpreviewcanvasitem.h"

//! How long the shutdown waits for a build before leaving the thread behind.
static const int sWorkerShutdownTimeout = 3000;

void KadasCatalogPreviewBuilder::build( int generation, const KadasCatalogLayerSource &source, const QgsCoordinateTransformContext &transformContext )
{
  // Runs on the worker thread, so nothing here may reach for the GUI: no
  // default style (which is read through the style database) and no CRS
  // validation (which can put up a dialog asking the user to pick a CRS).
  KadasCatalogPreviewLayer layer;
  switch ( source.type )
  {
    case KadasCatalogLayerSource::Type::Raster:
    {
      QgsRasterLayer::LayerOptions options( false, transformContext );
      options.skipCrsValidation = true;
      layer.reset( new QgsRasterLayer( source.uri, source.name, source.providerKey, options ) );
      break;
    }

    case KadasCatalogLayerSource::Type::Vector:
    {
      QgsVectorLayer::LayerOptions options( false, false );
      options.transformContext = transformContext;
      options.skipCrsValidation = true;
      layer.reset( new QgsVectorLayer( source.uri, source.name, source.providerKey, options ) );
      break;
    }

    case KadasCatalogLayerSource::Type::VectorTile:
    {
      const QgsVectorTileLayer::LayerOptions options( transformContext );
      layer.reset( new QgsVectorTileLayer( source.uri, source.name, options ) );
      break;
    }

    case KadasCatalogLayerSource::Type::Unknown:
      break;
  }

  if ( layer )
  {
    // Give up the thread affinity so the GUI thread can take the layer over; a
    // QObject without affinity is the one thing another thread may pull to
    // itself. If nobody is listening any more, the layer goes down with the
    // queued signal.
    layer->moveToThread( nullptr );
  }
  emit built( generation, layer );
}

KadasCatalogPreview::KadasCatalogPreview( QgsMapCanvas *canvas, QgsMessageBar *messageBar, QObject *parent )
  : QObject( parent )
  , mCanvas( canvas )
  , mMessageBar( messageBar )
{
  qRegisterMetaType<KadasCatalogPreviewLayer>();

  mItem = new KadasCatalogPreviewCanvasItem( canvas );

  // Same look as the canvas' own "Loading..." label, in the free bottom left
  // corner (that one sits bottom right).
  mBadge = new QLabel( canvas );
  mBadge->setStyleSheet( QStringLiteral( "QLabel { color: white; background: rgba(0,0,0,127); border-radius: 5px; padding: 5px; }" ) );
  mBadge->hide();
  canvas->installEventFilter( this );

  // The canvas destroys its scene, and with it the overlay item, in its own
  // destructor body -- before the base QObject destructor gets to its
  // children. Whichever way round this object is destroyed, dropping the
  // pointers here keeps the destructor from freeing the item a second time.
  connect( canvas, &QObject::destroyed, this, [this] {
    mCanvas = nullptr;
    mItem = nullptr;
  } );

  mWorkerThread = new QThread();
  mWorkerThread->setObjectName( QStringLiteral( "CatalogPreview" ) );
  mBuilder = new KadasCatalogPreviewBuilder();
  mBuilder->moveToThread( mWorkerThread );
  // A queued connection is what makes the handover safe: Qt drops it when this
  // object goes away, so a build finishing late cannot reach a dangling
  // pointer, and the layer it carries is released with the pending call.
  connect( mBuilder, &KadasCatalogPreviewBuilder::built, this, &KadasCatalogPreview::adoptLayer, Qt::QueuedConnection );
  connect( mWorkerThread, &QThread::finished, mBuilder, &QObject::deleteLater );
  mWorkerThread->start();

  connect( canvas, &QgsMapCanvas::extentsChanged, this, &KadasCatalogPreview::refresh );
  connect( canvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasCatalogPreview::refresh );
}

KadasCatalogPreview::~KadasCatalogPreview()
{
  cancelRenderJob();

  mWorkerThread->quit();
  if ( mWorkerThread->wait( sWorkerShutdownTimeout ) )
  {
    delete mWorkerThread;
  }
  else
  {
    // A provider is still waiting on the network. Blocking the shutdown until
    // it times out would look like a hung application, so let the thread run
    // out on its own and clean itself up.
    connect( mWorkerThread, &QThread::finished, mWorkerThread, &QObject::deleteLater );
  }

  // Null once the canvas has taken the item down with its scene.
  delete mItem;
}

void KadasCatalogPreview::setSource( const KadasCatalogLayerSource &source )
{
  // Whatever is on the canvas belongs to the previous entry. Building the new
  // layer can take as long as the server needs, and leaving the old image up
  // meanwhile would show it as if it were the entry just selected. clear()
  // also invalidates any build still in flight.
  clear();

  if ( !source.isValid() )
  {
    return;
  }

  mSourceName = source.name;
  mPending = true;
  updateBadge();

  const int generation = mGeneration;
  const QgsCoordinateTransformContext transformContext = QgsProject::instance()->transformContext();
  QMetaObject::invokeMethod( mBuilder, [builder = mBuilder, generation, source, transformContext] { builder->build( generation, source, transformContext ); }, Qt::QueuedConnection );
}

void KadasCatalogPreview::clear()
{
  ++mGeneration;
  cancelRenderJob();
  removeMessage();
  mLayer.reset();
  if ( mItem )
  {
    mItem->clear();
  }
  mSourceName.clear();
  mPending = false;
  updateBadge();
}

void KadasCatalogPreview::adoptLayer( int generation, KadasCatalogPreviewLayer layer )
{
  if ( layer )
  {
    layer->moveToThread( QThread::currentThread() );
  }

  if ( generation != mGeneration )
  {
    // Superseded while it was being built.
    return;
  }

  if ( !layer || !layer->isValid() )
  {
    const QString name = mSourceName;
    clear();
    showMessage( tr( "“%1” cannot be previewed." ).arg( name ), Qgis::MessageLevel::Warning );
    return;
  }

  mLayer = layer;
  checkLayerInView( layer.get() );
  refresh();
}

void KadasCatalogPreview::refresh()
{
  if ( !mLayer || !mCanvas )
  {
    return;
  }

  QgsMapSettings settings( mCanvas->mapSettings() );
  if ( settings.outputSize().isEmpty() )
  {
    return;
  }
  settings.setLayers( { mLayer.get() } );
  // Only the preview is rendered here; the map itself shows through.
  settings.setBackgroundColor( QColor( Qt::transparent ) );

  cancelRenderJob();
  const QgsRectangle extent = mCanvas->extent();
  mRenderJob = new QgsMapRendererParallelJob( settings );
  connect( mRenderJob, &QgsMapRendererJob::finished, this, [this, extent] {
    showImage( mRenderJob->renderedImage(), extent );
    mRenderJob->deleteLater();
    mRenderJob = nullptr;
  } );
  // Show the layers which are already done rather than waiting for all of them.
  connect( mRenderJob, &QgsMapRendererJob::renderingLayersFinished, this, [this, extent] { showImage( mRenderJob->renderedImage(), extent ); } );
  mRenderJob->start();
}

void KadasCatalogPreview::showImage( const QImage &image, const QgsRectangle &extent )
{
  if ( !mItem )
  {
    return;
  }
  mItem->setImage( image, extent );
  if ( mPending )
  {
    // The preview is on screen now; only its first image is announced, so
    // panning and zooming does not flicker the badge.
    mPending = false;
    updateBadge();
  }
}

void KadasCatalogPreview::updateBadge()
{
  if ( !mBadge || !mCanvas )
  {
    return;
  }

  if ( mSourceName.isEmpty() )
  {
    mBadge->hide();
    return;
  }

  // Catalog entry names get long; keep the badge from running across the map.
  const QString name = mBadge->fontMetrics().elidedText( mSourceName, Qt::ElideRight, std::max( 80, mCanvas->width() / 3 ) );
  mBadge->setText( mPending ? tr( "Preview: %1 (loading...)" ).arg( name ) : tr( "Preview: %1" ).arg( name ) );
  positionBadge();
  mBadge->show();
  mBadge->raise();
}

void KadasCatalogPreview::positionBadge()
{
  const int margin = 5;

  mBadge->adjustSize();
  mBadge->move( margin, mCanvas->height() - margin - mBadge->height() );
}

bool KadasCatalogPreview::eventFilter( QObject *watched, QEvent *event )
{
  if ( watched == mCanvas && event->type() == QEvent::Resize && !mSourceName.isEmpty() )
  {
    // Re-elide as well as reposition: the room for the name changed.
    updateBadge();
  }
  return QObject::eventFilter( watched, event );
}

void KadasCatalogPreview::cancelRenderJob()
{
  if ( !mRenderJob )
  {
    return;
  }

  mRenderJob->disconnect( this );
  if ( mRenderJob->isActive() )
  {
    connect( mRenderJob, &QgsMapRendererJob::finished, mRenderJob, &QObject::deleteLater );
    mRenderJob->cancelWithoutBlocking();
  }
  else
  {
    mRenderJob->deleteLater();
  }
  mRenderJob = nullptr;
}

void KadasCatalogPreview::checkLayerInView( QgsMapLayer *layer )
{
  if ( !mCanvas )
  {
    return;
  }

  const QgsRectangle layerExtent = layer->extent();
  if ( layerExtent.isNull() || layerExtent.isEmpty() )
  {
    // Nothing to compare against; the render will tell the user soon enough.
    return;
  }

  QgsRectangle extent;
  try
  {
    QgsCoordinateTransform transform( layer->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
    transform.setBallparkTransformsAreAppropriate( true );
    extent = transform.transformBoundingBox( layerExtent );
  }
  catch ( const QgsCsException & )
  {
    return;
  }

  if ( extent.intersects( mCanvas->extent() ) )
  {
    return;
  }

  // Without this the preview would simply render empty, which reads as broken.
  showMessage( tr( "“%1” lies outside the current view." ).arg( layer->name() ), Qgis::MessageLevel::Info, extent );
}

void KadasCatalogPreview::showMessage( const QString &text, Qgis::MessageLevel level, const QgsRectangle &zoomTo )
{
  removeMessage();
  if ( !mMessageBar )
  {
    return;
  }

  QPushButton *button = nullptr;
  if ( !zoomTo.isEmpty() )
  {
    button = new QPushButton( tr( "Zoom to layer" ) );
    button->setFlat( true );
    connect( button, &QPushButton::clicked, this, [this, zoomTo] {
      if ( !mCanvas )
      {
        return;
      }
      QgsRectangle extent = zoomTo;
      removeMessage();
      mCanvas->zoomToFeatureExtent( extent );
    } );
  }

  // Duration 0: the message belongs to the preview and goes when it does.
  QgsMessageBarItem *item = new QgsMessageBarItem( tr( "Catalog preview" ), text, button, level, 0 );
  mMessageItem = item;
  mMessageBar->pushItem( item );
}

void KadasCatalogPreview::removeMessage()
{
  if ( mMessageItem && mMessageBar )
  {
    mMessageBar->popWidget( mMessageItem );
  }
  mMessageItem = nullptr;
}
