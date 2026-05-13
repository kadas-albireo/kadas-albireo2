/***************************************************************************
    kadasmaptoolminmax.cpp
    ------------------------
    copyright            : (C) 2022 by Sandro Mani
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

#include <gdal.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsgeometrycollection.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrubberband.h>
#include <qgis/qgssettings.h>

#include "kadas/core/kadas.h"
#include "kadas/core/kadascoordinateformat.h"
#include "kadas/analysis/kadasninecellfilter.h"
#include "kadas/gui/annotationitems/kadasannotationlayerregistry.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"
#include "kadas/gui/mapitems/kadascircleitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadasrectangleitem.h"
#include "kadas/gui/maptools/kadasmaptoolminmax.h"
#include "kadas/gui/kadasfeaturepicker.h"


KadasMapItem *KadasMapToolMinMaxItemInterface::createItem() const
{
  switch ( mFilterType )
  {
    case KadasMapToolMinMax::FilterType::FilterRect:
      return new KadasRectangleItem( mCanvas->mapSettings().destinationCrs() );
    case KadasMapToolMinMax::FilterType::FilterPoly:
      return new KadasPolygonItem( mCanvas->mapSettings().destinationCrs() );
    case KadasMapToolMinMax::FilterType::FilterCircle:
      return new KadasCircleItem( mCanvas->mapSettings().destinationCrs() );
  }
  return nullptr;
}

void KadasMapToolMinMaxItemInterface::setFilterType( KadasMapToolMinMax::FilterType filterType )
{
  mFilterType = filterType;
}

KadasMapToolMinMax::KadasMapToolMinMax( QgsMapCanvas *mapCanvas, QAction *actionViewshed, QAction *actionProfile )
  : KadasMapToolCreateItem( mapCanvas, std::move( std::make_unique<KadasMapToolMinMaxItemInterface>( KadasMapToolMinMaxItemInterface( mapCanvas ) ) ) )
  , mFilterType( FilterType::FilterRect )
  , mActionViewshed( actionViewshed )
  , mActionProfile( actionProfile )
{
  setCursor( Qt::ArrowCursor );
  setToolLabel( tr( "Compute min/max" ) );
  setUndoRedoVisible( false );
  connect( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolMinMax::drawFinished );

  QWidget *filterTypeWidget = new QWidget();
  filterTypeWidget->setLayout( new QHBoxLayout() );
  filterTypeWidget->layout()->addWidget( new QLabel( tr( "Select area by:" ) ) );
  filterTypeWidget->layout()->setContentsMargins( 0, 0, 0, 0 );

  mFilterTypeCombo = new QComboBox();
  mFilterTypeCombo->addItem( tr( "Rectangle" ), QVariant::fromValue( KadasMapToolMinMax::FilterType::FilterRect ) );
  mFilterTypeCombo->addItem( tr( "Polygon" ), QVariant::fromValue( KadasMapToolMinMax::FilterType::FilterPoly ) );
  mFilterTypeCombo->addItem( tr( "Circle" ), QVariant::fromValue( KadasMapToolMinMax::FilterType::FilterCircle ) );
  mFilterTypeCombo->setCurrentIndex( 0 );
  connect( mFilterTypeCombo, qOverload<int>( &QComboBox::currentIndexChanged ), this, [this]( int ) { setFilterType( static_cast<FilterType>( mFilterTypeCombo->currentData().toInt() ) ); } );
  filterTypeWidget->layout()->addWidget( mFilterTypeCombo );

  QToolButton *pickButton = new QToolButton();
  pickButton->setIcon( QIcon( ":/kadas/icons/select" ) );
  pickButton->setToolTip( tr( "Pick existing geometry" ) );
  connect( pickButton, &QToolButton::clicked, this, &KadasMapToolMinMax::requestPick );
  filterTypeWidget->layout()->addWidget( pickButton );

  setExtraBottomBarContents( filterTypeWidget );
}

KadasMapToolMinMax::~KadasMapToolMinMax()
{
  delete mPinMinBand;
  delete mPinMaxBand;
}

void KadasMapToolMinMax::setFilterType( FilterType filterType )
{
  mFilterType = filterType;
  static_cast<KadasMapToolMinMaxItemInterface *>( mInterface.get() )->setFilterType( filterType );
  mFilterTypeCombo->blockSignals( true );
  mFilterTypeCombo->setCurrentIndex( mFilterTypeCombo->findData( QVariant::fromValue( filterType ) ) );
  mFilterTypeCombo->blockSignals( false );
}
static inline double pixelToGeoX( double gtrans[6], double px, double py )
{
  return gtrans[0] + px * gtrans[1] + py * gtrans[2];
}

static inline double pixelToGeoY( double gtrans[6], double px, double py )
{
  return gtrans[3] + px * gtrans[4] + py * gtrans[5];
}

void KadasMapToolMinMax::drawFinished()
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != Qgis::LayerType::Raster )
  {
    emit messageEmitted( tr( "No heightmap is defined in the project. Right-click a raster layer in the layer tree and select it to be used as heightmap." ), Qgis::Warning );
    clear();
    return;
  }

  GDALDatasetH inputDataset = Kadas::gdalOpenForLayer( static_cast<QgsRasterLayer *>( layer ) );
  double gtrans[6] = {};
  if ( inputDataset == NULL || GDALGetRasterCount( inputDataset ) < 1 || GDALGetGeoTransform( inputDataset, &gtrans[0] ) != CE_None )
  {
    GDALClose( inputDataset );
    clear();
    return;
  }
  GDALRasterBandH band = GDALGetRasterBand( inputDataset, 1 );
  QgsCoordinateReferenceSystem inputCrs( QString( GDALGetProjectionRef( inputDataset ) ) );

  QgsCoordinateTransform crst( inputCrs, mCanvas->mapSettings().destinationCrs(), layer->transformContext() );

  const KadasGeometryItem *item = static_cast<const KadasGeometryItem *>( currentItem() );
  QgsAbstractGeometry *geom = nullptr;
  if ( mFilterType == FilterType::FilterCircle )
  {
    geom = item->geometry()->segmentize( M_PI / 22.5 );
  }
  else
  {
    geom = item->geometry()->clone();
  }
  QgsRectangle bbox = crst.transformBoundingBox( geom->boundingBox(), Qgis::TransformDirection::Reverse );
  QPolygonF filterGeom = QgsGeometry( geom ).asQPolygonF();

  if ( bbox.isEmpty() )
  {
    clear();
    return;
  }

  //determine the window
  int rowStart, rowEnd, colStart, colEnd;
  if ( !KadasNineCellFilter::computeWindow( inputDataset, inputCrs, bbox, inputCrs, rowStart, rowEnd, colStart, colEnd ) )
  {
    GDALClose( inputDataset );
    clear();
    return;
  }

  double pixValues[256 * 256] = {};
  double valMin = std::numeric_limits<double>::max();
  double valMax = 0;
  double xMin = 0, xMax = 0;
  double yMin = 0, yMax = 0;
  // Read in 256x256 chunks
  for ( int x = colStart; x <= colEnd; x += 256 )
  {
    for ( int y = rowStart; y <= rowEnd; y += 256 )
    {
      if ( CE_None == GDALRasterIO( band, GF_Read, x, y, 256, 256, &pixValues[0], 256, 256, GDT_Float64, 0, 0 ) )
      {
        int maxx = std::min( 256, colEnd - x + 1 );
        int maxy = std::min( 256, rowEnd - y + 1 );
#pragma omp parallel for schedule( static )
        for ( int bx = 0; bx < maxx; ++bx )
        {
          double localValMin = std::numeric_limits<double>::max();
          double localValMax = 0;
          double localxMin = 0, localxMax = 0;
          double localyMin = 0, localyMax = 0;

          for ( int by = 0; by < maxy; ++by )
          {
            QgsPointXY p( crst.transform( pixelToGeoX( gtrans, x + bx, y + by ), pixelToGeoY( gtrans, x + bx, y + by ) ) );
            if ( mFilterType != FilterType::FilterRect && !filterGeom.containsPoint( p.toQPointF(), Qt::WindingFill ) )
            {
              continue;
            }

            double val = pixValues[by * 256 + bx];
            if ( val < localValMin )
            {
              localValMin = val;
              localxMin = p.x();
              localyMin = p.y();
            }
            if ( val > localValMax )
            {
              localValMax = val;
              localxMax = p.x();
              localyMax = p.y();
            }
          }

#pragma omp critical
          {
            if ( localValMin < valMin )
            {
              valMin = localValMin;
              xMin = localxMin;
              yMin = localyMin;
            }
            if ( localValMax > valMax )
            {
              valMax = localValMax;
              xMax = localxMax;
              yMax = localyMax;
            }
          }
        }
      }
    }
  }

  QgsPointXY pMin( xMin, yMin );
  QgsPointXY pMax( xMax, yMax );

  // SVG natural size: 55x43. tri_up anchored at top-center -> drawOffset (-W/2, 0)
  // so the icon hangs below the geographic point. tri_down anchored at
  // bottom-center -> drawOffset (-W/2, -H) so it floats above.
  if ( !mPinMinBand )
  {
    mPinMinBand = new QgsRubberBand( mCanvas, Qgis::GeometryType::Point );
    mPinMinBand->setIcon( QgsRubberBand::ICON_SVG );
    mPinMinBand->setSvgIcon( QStringLiteral( ":/kadas/icons/tri_up" ), QPoint( -27, 0 ) );
  }
  mPinMinBand->reset( Qgis::GeometryType::Point );
  mPinMinBand->addPoint( pMin, true );
  mPinMinPos = pMin;

  if ( !mPinMaxBand )
  {
    mPinMaxBand = new QgsRubberBand( mCanvas, Qgis::GeometryType::Point );
    mPinMaxBand->setIcon( QgsRubberBand::ICON_SVG );
    mPinMaxBand->setSvgIcon( QStringLiteral( ":/kadas/icons/tri_down" ), QPoint( -27, -43 ) );
  }
  mPinMaxBand->reset( Qgis::GeometryType::Point );
  mPinMaxBand->addPoint( pMax, true );
  mPinMaxPos = pMax;
  mPinsValid = true;

  clear();
}

void KadasMapToolMinMax::requestPick()
{
  mPickFeature = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void KadasMapToolMinMax::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( mPickFeature )
  {
    return;
  }
  if ( currentItem() && currentItem()->drawStatus() != KadasMapItem::DrawStatus::Drawing && mPinsValid )
  {
    // Hit test in canvas pixel space against the SVG marker bounding box.
    // The tri_up / tri_down SVGs are 55x43; pick a generous half-extent.
    const QgsMapToPixel &m2p = mCanvas->mapSettings().mapToPixel();
    const QPointF clickPx( e->pos().x(), e->pos().y() );
    const QPointF maxPx = m2p.transform( QgsPointXY( mPinMaxPos ) ).toQPointF();
    const QPointF minPx = m2p.transform( QgsPointXY( mPinMinPos ) ).toQPointF();
    constexpr double tolHX = 28.0; // half SVG width + slack
    constexpr double tolH = 45.0;  // SVG height + slack
    auto inBox = [&]( const QPointF &px, double yOffset ) {
      // yOffset: -tolH for tri_down (above pos), 0..tolH for tri_up (below pos).
      return std::abs( clickPx.x() - px.x() ) <= tolHX && ( clickPx.y() - px.y() ) >= yOffset && ( clickPx.y() - px.y() ) <= yOffset + tolH;
    };
    if ( inBox( maxPx, -tolH ) )
    {
      showContextMenu( mPinMaxPos );
      return;
    }
    else if ( inBox( minPx, 0 ) )
    {
      showContextMenu( mPinMinPos );
      return;
    }
  }
  KadasMapToolCreateItem::canvasPressEvent( e );
}

void KadasMapToolMinMax::showContextMenu( const QgsPointXY &mapPos ) const
{
  QMenu menu;
  menu.addAction( QIcon( ":/kadas/icons/copy_coordinates" ), tr( "Copy coordinates" ), [this, mapPos] {
    const QgsCoordinateReferenceSystem &mapCrs = mCanvas->mapSettings().destinationCrs();
    QString posStr = KadasCoordinateFormat::instance()->getDisplayString( mapPos, mapCrs );
    QString text = QString( "%1\n%2" ).arg( posStr ).arg( KadasCoordinateFormat::instance()->getHeightAtPos( mapPos, mapCrs ) );
    QApplication::clipboard()->setText( text );
  } );
  menu.addAction( QIcon( ":/kadas/icons/viewshed_color" ), tr( "Viewshed" ), [this, mapPos] {
    mActionViewshed->trigger();
    QgsMapTool *tool = mCanvas->mapTool();
    if ( dynamic_cast<KadasMapToolCreateItem *>( tool ) )
    {
      static_cast<KadasMapToolCreateItem *>( tool )->addPoint( KadasMapPos::fromPoint( mapPos ) );
    }
  } );
  menu.addAction( QIcon( ":/kadas/icons/measure_height_profile" ), tr( "Line of sight" ), [this, mapPos] {
    mActionProfile->trigger();
    QgsMapTool *tool = mCanvas->mapTool();
    if ( dynamic_cast<KadasMapToolCreateItem *>( tool ) )
    {
      static_cast<KadasMapToolCreateItem *>( tool )->addPoint( KadasMapPos::fromPoint( mapPos ) );
    }
  } );
  menu.addAction( QIcon( ":/kadas/icons/pin_red" ), tr( "Add pin" ), [this, mapPos] {
    QgsAnnotationLayer *layer = KadasAnnotationLayerRegistry::getOrCreateAnnotationLayer( KadasAnnotationLayerRegistry::StandardLayer::PinsLayer );
    if ( !layer )
      return;
    const QgsPointXY layerPos = QgsCoordinateTransform( mCanvas->mapSettings().destinationCrs(), layer->crs(), QgsProject::instance()->transformContext() ).transform( mapPos );
    auto *pin = new KadasPinAnnotationItem( QgsPoint( layerPos.x(), layerPos.y() ) );
    layer->addItem( pin );
    layer->triggerRepaint();
  } );
  menu.exec( mCanvas->mapToGlobal( toCanvasCoordinates( mapPos ) ) );
}

void KadasMapToolMinMax::canvasMoveEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    KadasMapToolCreateItem::canvasMoveEvent( e );
  }
}

void KadasMapToolMinMax::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    KadasMapToolCreateItem::canvasReleaseEvent( e );
  }
  else
  {
    KadasFeaturePicker::PickResult pickResult = KadasFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), Qgis::GeometryType::Polygon );
    if ( pickResult.geom && pickResult.geom->partCount() == 1 )
    {
      QgsAbstractGeometry *geom = dynamic_cast<QgsGeometryCollection *>( pickResult.geom ) ? static_cast<QgsGeometryCollection *>( pickResult.geom )->geometryN( 0 ) : pickResult.geom;
      if ( QgsWkbTypes::flatType( geom->wkbType() ) == Qgis::WkbType::CurvePolygon )
      {
        setFilterType( FilterType::FilterCircle );
      }
      else
      {
        setFilterType( FilterType::FilterPoly );
      }
      addPartFromGeometry( *pickResult.geom, pickResult.crs );
    }
    mPickFeature = false;
    setCursor( Qt::ArrowCursor );
  }
}
