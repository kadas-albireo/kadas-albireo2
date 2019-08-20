/***************************************************************************
    kadasheightprofiledialog.cpp
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

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

#include <gdal.h>

#include <qgis/qgsdistancearea.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvector.h>

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/gui/kadasheightprofiledialog.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/maptools/kadasmaptoolheightprofile.h>


class KadasHeightProfileDialog::ScaleDraw : public QwtScaleDraw
{
public:
  ScaleDraw ( double maxVal, int nSamples ) : mFactor ( maxVal / nSamples )
  {
  }
  QwtText label ( double value ) const override
  {
    return QLocale::system().toString ( value * mFactor );
  }
private:
  double mFactor;
};

class PaddedPlotMarker : public QwtPlotMarker
{
public:
  PaddedPlotMarker() : QwtPlotMarker() {}
  void getCanvasMarginHint ( const QwtScaleMap& /*xMap*/, const QwtScaleMap& /*yMap*/, const QRectF& /*canvasSize*/, double& left, double& top, double& right, double& bottom ) const override
  {
    left = 15;
    right = 15;
    top = 15;
    bottom = 5;
  }
};

KadasHeightProfileDialog::KadasHeightProfileDialog ( KadasMapToolHeightProfile* tool, QWidget* parent, Qt::WindowFlags f )
  : QDialog ( parent, f ), mTool ( tool ), mLineOfSightMarker ( 0 ), mNSamples ( 1000 )
{
  setWindowTitle ( tr ( "Height profile" ) );
  setAttribute ( Qt::WA_ShowWithoutActivating );
  QVBoxLayout* vboxLayout = new QVBoxLayout ( this );

  QPushButton* pickButton = new QPushButton ( QIcon ( ":/images/themes/default/mActionSelect.svg" ), tr ( "Measure along existing line" ), this );
  connect ( pickButton, &QPushButton::clicked, mTool, &KadasMapToolHeightProfile::pickLine );
  vboxLayout->addWidget ( pickButton );

  QgsUnitTypes::DistanceUnit heightDisplayUnit = KadasCoordinateFormat::instance()->getHeightDisplayUnit();

  mPlot = new QwtPlot ( this );
  mPlot->canvas()->setCursor ( Qt::ArrowCursor );
  mPlot->setCanvasBackground ( Qt::white );
  mPlot->enableAxis ( QwtPlot::yLeft );
  mPlot->enableAxis ( QwtPlot::xBottom );
  mPlot->setAxisTitle ( QwtPlot::yLeft, heightDisplayUnit == QgsUnitTypes::DistanceFeet ? tr ( "Height [ft AMSL]" ) : tr ( "Height [m AMSL]" ) );
  mPlot->setAxisTitle ( QwtPlot::xBottom, tr ( "Distance [m]" ) );
  mPlot->setMinimumHeight ( 200 );
  mPlot->setSizePolicy ( mPlot->sizePolicy().horizontalPolicy(), QSizePolicy::MinimumExpanding );
  vboxLayout->addWidget ( mPlot );

  QwtPlotGrid* grid = new QwtPlotGrid();
  grid->setMajorPen ( QPen ( Qt::gray ) );
  grid->attach ( mPlot );

  mPlotCurve = new QwtPlotCurve ( tr ( "Height profile" ) );
  mPlotCurve->setRenderHint ( QwtPlotItem::RenderAntialiased );
  QPen curvePen;
  curvePen.setColor ( Qt::red );
  curvePen.setJoinStyle ( Qt::RoundJoin );
  mPlotCurve->setPen ( curvePen );
  mPlotCurve->setBaseline ( 0 );
  mPlotCurve->setBrush ( QColor ( 255, 127, 127 ) );
  mPlotCurve->attach ( mPlot );
  mPlotCurve->setData ( new QwtPointSeriesData() );

  mPlotMarker = new PaddedPlotMarker();
  mPlotMarker->setSymbol ( new QwtSymbol ( QwtSymbol::Ellipse, QBrush ( Qt::blue ), QPen ( Qt::blue ), QSize ( 5, 5 ) ) );
  mPlotMarker->setItemAttribute ( QwtPlotItem::Margins );
  mPlotMarker->attach ( mPlot );
  mPlotMarker->setLabelAlignment ( Qt::AlignTop | Qt::AlignHCenter );

  mLineOfSightGroupBoxgroupBox = new QGroupBox ( this );
  mLineOfSightGroupBoxgroupBox->setTitle ( tr ( "Line of sight" ) );
  mLineOfSightGroupBoxgroupBox->setCheckable ( true );
  connect ( mLineOfSightGroupBoxgroupBox, &QGroupBox::clicked, this, &KadasHeightProfileDialog::updateLineOfSight );

  QGridLayout* layoutLOS = new QGridLayout();
  layoutLOS->setContentsMargins ( 2, 2, 2, 2 );
  mLineOfSightGroupBoxgroupBox->setLayout ( layoutLOS );
  layoutLOS->addWidget ( new QLabel ( tr ( "Observer height:" ) ), 0, 0 );

  mObserverHeightSpinBox = new QDoubleSpinBox();
  mObserverHeightSpinBox->setRange ( 0, 8000 );
  mObserverHeightSpinBox->setDecimals ( 1 );
  mObserverHeightSpinBox->setSuffix ( heightDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );
  mObserverHeightSpinBox->setValue ( QgsSettings().value ( "/Qgis/measure/heightprofile/observerheight", 2.0 ).toDouble() );
  mObserverHeightSpinBox->setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Preferred );
  connect ( mObserverHeightSpinBox, qOverload<double> ( &QDoubleSpinBox::valueChanged ), this, &KadasHeightProfileDialog::updateLineOfSight );

  layoutLOS->addWidget ( mObserverHeightSpinBox, 0, 1 );
  layoutLOS->addWidget ( new QLabel ( tr ( "Target height:" ) ), 0, 2 );

  mTargetHeightSpinBox = new QDoubleSpinBox();
  mTargetHeightSpinBox->setRange ( 0, 8000 );
  mTargetHeightSpinBox->setDecimals ( 1 );
  mTargetHeightSpinBox->setSuffix ( heightDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );
  mTargetHeightSpinBox->setValue ( QgsSettings().value ( "/Qgis/measure/heightprofile/targetheight", 2.0 ).toDouble() );
  mTargetHeightSpinBox->setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Preferred );
  connect ( mTargetHeightSpinBox, qOverload<double> ( &QDoubleSpinBox::valueChanged ), this, &KadasHeightProfileDialog::updateLineOfSight );

  layoutLOS->addWidget ( mTargetHeightSpinBox, 0, 3 );
  layoutLOS->addWidget ( new QLabel ( tr ( "Heights relative to:" ) ), 0, 4 );

  mHeightModeCombo = new QComboBox();
  mHeightModeCombo->addItem ( tr ( "Ground" ), static_cast<int> ( HeightRelToGround ) );
  mHeightModeCombo->addItem ( tr ( "Sea level" ), static_cast<int> ( HeightRelToSeaLevel ) );
  connect ( mHeightModeCombo, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasHeightProfileDialog::updateLineOfSight );
  layoutLOS->addWidget ( mHeightModeCombo, 0, 5 );

  vboxLayout->addWidget ( mLineOfSightGroupBoxgroupBox );

  QDialogButtonBox* bbox = new QDialogButtonBox ( QDialogButtonBox::Close, Qt::Horizontal, this );
  QPushButton* copyButton = bbox->addButton ( tr ( "Copy to clipboard" ), QDialogButtonBox::ActionRole );
  QPushButton* addButton = bbox->addButton ( tr ( "Add to map" ), QDialogButtonBox::ActionRole );
  vboxLayout->addWidget ( bbox );
  connect ( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect ( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  connect ( copyButton, &QPushButton::clicked, this, &KadasHeightProfileDialog::copyToClipboard );
  connect ( addButton, &QPushButton::clicked, this, &KadasHeightProfileDialog::addToCanvas );
  connect ( this, &QDialog::finished, this, &KadasHeightProfileDialog::finish );

  connect ( KadasCoordinateFormat::instance(), &KadasCoordinateFormat::heightDisplayUnitChanged, this, &KadasHeightProfileDialog::replot );

  restoreGeometry ( QgsSettings().value ( "/Windows/MeasureHeightProfile/geometry" ).toByteArray() );
}

void KadasHeightProfileDialog::setPoints ( const QList<QgsPointXY>& points, const QgsCoordinateReferenceSystem& crs )
{
  mPoints = points;
  mPointsCrs = crs;
  mTotLength = 0;
  mSegmentLengths.clear();
  QgsDistanceArea da;
  da.setEllipsoid ( QgsProject::instance()->readEntry ( "Measure", "/Ellipsoid", GEO_NONE ) );
  da.setSourceCrs ( crs, QgsProject::instance()->transformContext() );
  for ( int i = 0, n = mPoints.size() - 1; i < n; ++i ) {
    mSegmentLengths.append ( da.measureLine ( mPoints[i + 1], mPoints[i] ) );
    mTotLength += mSegmentLengths.back();
  }
  mLineOfSightGroupBoxgroupBox->setEnabled ( points.size() == 2 );
  replot();
}

void KadasHeightProfileDialog::setMarkerPos ( int segment, const QgsPointXY& p )
{
  double x = qSqrt ( p.sqrDist ( mPoints[segment] ) );
  for ( int i = 0; i < segment; ++i ) {
    x += mSegmentLengths[i];
  }
  int idx = qMin ( int ( x / mTotLength * mNSamples ), mNSamples - 1 );
  QPointF sample = mPlotCurve->data()->sample ( idx );
  mPlotMarker->setValue ( sample );
  mPlotMarker->setLabel ( QString::number ( qRound ( sample.y() ) ) );
  mPlot->replot();
}

void KadasHeightProfileDialog::clear()
{
  static_cast<QwtPointSeriesData*> ( mPlotCurve->data() )->setSamples ( QVector<QPointF>() );
  mPlotMarker->setValue ( 0, 0 );
  qDeleteAll ( mLinesOfSight );
  mLinesOfSight.clear();
  qDeleteAll ( mLinesOfSightRB );
  mLinesOfSightRB.clear();
  delete mLineOfSightMarker;
  mLineOfSightMarker = 0;
  qDeleteAll ( mNodeMarkers );
  mNodeMarkers.clear();
  mPlot->replot();
  mLineOfSightGroupBoxgroupBox->setEnabled ( false );
}

void KadasHeightProfileDialog::finish()
{
  QgsSettings().setValue ( "/Windows/MeasureHeightProfile/geometry", saveGeometry() );
  mTool->canvas()->unsetMapTool ( mTool );
  clear();
}

void KadasHeightProfileDialog::replot()
{
  QgsUnitTypes::DistanceUnit vertDisplayUnit = KadasCoordinateFormat::instance()->getHeightDisplayUnit();
  mPlot->setAxisTitle ( QwtPlot::yLeft, vertDisplayUnit == QgsUnitTypes::DistanceFeet ? tr ( "Height [ft AMSL]" ) : tr ( "Height [m AMSL]" ) );
  mObserverHeightSpinBox->setSuffix ( vertDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );
  mTargetHeightSpinBox->setSuffix ( vertDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );

  if ( mPoints.isEmpty() ) {
    return;
  }

  QString layerid = QgsProject::instance()->readEntry ( "Heightmap", "layer" );
  QgsMapLayer* layer = QgsProject::instance()->mapLayer ( layerid );
  if ( !layer || layer->type() != QgsMapLayerType::RasterLayer ) {
    emit mTool->messageEmitted ( tr ( "No heightmap is defined in the project." ), Qgis::Warning );
    return;
  }
  QString rasterFile = layer->source();
  GDALDatasetH raster = GDALOpen ( rasterFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( !raster ) {
    QMessageBox::warning ( 0, tr ( "Error" ), tr ( "Failed to open raster file: %1" ).arg ( rasterFile ) );
    return;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform ( raster, &gtrans[0] ) != CE_None ) {
    QgsDebugMsg ( "Failed to get raster geotransform" );
    GDALClose ( raster );
    return;
  }

  QString proj ( GDALGetProjectionRef ( raster ) );
  QgsCoordinateReferenceSystem rasterCrs ( proj );
  if ( !rasterCrs.isValid() ) {
    QgsDebugMsg ( "Failed to get raster CRS" );
    GDALClose ( raster );
    return;
  }

  GDALRasterBandH band = GDALGetRasterBand ( raster, 1 );
  if ( !band ) {
    QgsDebugMsg ( "Failed to open raster band 0" );
    GDALClose ( raster );
    return;
  }

  // Get vertical unit
  QgsUnitTypes::DistanceUnit vertUnit = strcmp ( GDALGetRasterUnitType ( band ), "ft" ) == 0 ? QgsUnitTypes::DistanceFeet : QgsUnitTypes::DistanceMeters;
  double heightConversion = QgsUnitTypes::fromUnitToUnitFactor ( vertUnit, vertDisplayUnit );
  mPlot->setAxisTitle ( QwtPlot::yLeft, vertDisplayUnit == QgsUnitTypes::DistanceFeet ? tr ( "Height [ft AMSL]" ) : tr ( "Height [m AMSL]" ) );
  mObserverHeightSpinBox->setSuffix ( vertDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );
  mTargetHeightSpinBox->setSuffix ( vertDisplayUnit == QgsUnitTypes::DistanceFeet ? " ft" : " m" );

  QVector<QPointF> samples;
  double x = 0;
  for ( int i = 0, n = mPoints.size() - 1; i < n; ++i ) {
    if ( x >= mSegmentLengths[i] ) {
      continue;
    }

    QgsVector dir = QgsVector ( mPoints[i + 1] - mPoints[i] ).normalized();
    while ( x < mSegmentLengths[i] ) {
      QgsPointXY p = mPoints[i] + dir * x;
      // Transform geo position to raster CRS
      QgsPointXY pRaster = QgsCoordinateTransform ( mPointsCrs, rasterCrs, QgsProject::instance() ).transform ( p );


      // Transform raster geo position to pixel coordinates
      double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
      double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );

      double pixValues[4] = {};
      if ( CE_None != GDALRasterIO ( band, GF_Read,
                                     qFloor ( col ), qFloor ( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) ) {
        QgsDebugMsg ( "Failed to read pixel values" );
        samples.append ( QPointF ( samples.size(), 0 ) );
      } else {
        // Interpolate values
        double lambdaR = row - qFloor ( row );
        double lambdaC = col - qFloor ( col );

        double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                       + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
        samples.append ( QPointF ( samples.size(), value * heightConversion ) );
      }
      x += mTotLength / mNSamples;
    }
    x -= mSegmentLengths[i];
  }

  static_cast<QwtPointSeriesData*> ( mPlotCurve->data() )->setSamples ( samples );
  int nSamples = samples.size();
  mPlotMarker->setValue ( 0, 0 );
  mPlot->setAxisScaleDraw ( QwtPlot::xBottom, new ScaleDraw ( mTotLength, nSamples ) );
  double step = qPow ( 10, qFloor ( log10 ( mTotLength ) ) ) / ( mTotLength ) * nSamples;
  while ( nSamples / step < 10 ) { step /= 2.; }
  while ( nSamples / step > 10 ) { step *= 2.; }
  mPlot->setAxisScale ( QwtPlot::xBottom, 0, nSamples, step );

  GDALClose ( raster );

  // Node markers
  x = 0;
  for ( int i = 0, n = mPoints.size() - 2; i < n; ++i ) {
    x += mSegmentLengths[i];
    QwtPlotMarker* nodeMarker = new QwtPlotMarker();
    nodeMarker->setLinePen ( QPen ( Qt::black, 1, Qt::DashLine ) );
    nodeMarker->setLineStyle ( QwtPlotMarker::VLine );
    int idx = qMin ( int ( x / mTotLength * mNSamples ), mNSamples - 1 );
    QPointF sample = mPlotCurve->data()->sample ( idx );
    nodeMarker->setValue ( sample );
    nodeMarker->attach ( mPlot );
    mNodeMarkers.append ( nodeMarker );
  }

  updateLineOfSight( );
}

void KadasHeightProfileDialog::updateLineOfSight( )
{
  qDeleteAll ( mLinesOfSight );
  mLinesOfSight.clear();
  qDeleteAll ( mLinesOfSightRB );
  mLinesOfSightRB.clear();
  delete mLineOfSightMarker;
  mLineOfSightMarker = 0;

  if ( !mLineOfSightGroupBoxgroupBox->isEnabled() || !mLineOfSightGroupBoxgroupBox->isChecked() ) {
    mPlot->replot();
    return;
  }

  int nSamples = mPlotCurve->dataSize();
  QVector<QPointF> samples = static_cast<QwtPointSeriesData*> ( mPlotCurve->data() )->samples();

  QVector< QVector<QPointF> > losSampleSet;
  losSampleSet.append ( QVector<QPointF>() );
  double targetHeight = mTargetHeightSpinBox->value();
  bool heightRelToGround = static_cast<HeightMode> ( mHeightModeCombo->itemData ( mHeightModeCombo->currentIndex() ).toInt() ) == HeightRelToGround;

  QVector<double> pX;
  pX.reserve ( nSamples );
  QVector<double> pY;
  pY.reserve ( nSamples );
  double earthRadius = 6370000;
  double meterToDisplayUnit = QgsUnitTypes::fromUnitToUnitFactor ( QgsUnitTypes::DistanceMeters, KadasCoordinateFormat::instance()->getHeightDisplayUnit() );
  for ( const QPointF& p : samples ) {
    pX.append ( p.x() / mNSamples * mTotLength );
    double hCorr = 0.87 * pX.last() * pX.last() / ( 2 * earthRadius ) * meterToDisplayUnit;
    pY.append ( p.y() - hCorr );
  }

  // Visibility of first point
  double y = pY.front() + targetHeight - ( heightRelToGround ? 0 : samples.front().y() );
  if ( y > pY.front() ) {
    losSampleSet.back().append ( samples.front() );
  } else {
    losSampleSet.append ( QVector<QPointF>() );
  }

  QPointF p1 ( pX.front(), ( heightRelToGround ? pY.front() : 0 ) + mObserverHeightSpinBox->value() );
  for ( int i = 1; i < nSamples; ++i ) {
    QPointF p2 ( pX[i], pY[i] + targetHeight );
    if ( !heightRelToGround ) {
      p2.ry() -= samples[i].y();
    }
    // For each sample position along line [p1, p2], check if y is below terrain
    // X = p1.x() + d * (p2.x() - p1.x())
    // Y = p1.y() + d * (p2.y() - p1.y())
    // => d = (X - p1.x()) / (p2.x() - p1.x())
    // => Y = p1.y() + (X - p1.x()) / (p2.x() - p1.x()) * (p2.y() - p1.y())
    bool visible = true;
    for ( int j = 1; j < i; ++j ) {
      double Y = p1.y() + ( pX[j] - p1.x() ) / ( p2.x() - p1.x() ) * ( p2.y() - p1.y() );
      if ( Y < pY[j] ) {
        visible = false;
        break;
      }
    }
    if ( ( visible && losSampleSet.size() % 2 == 0 ) || ( !visible && losSampleSet.size() % 2 == 1 ) ) {
      losSampleSet.append ( QVector<QPointF>() );
    }
    losSampleSet.back().append ( samples[i] );
  }

  Qt::GlobalColor colors[] = {Qt::green, Qt::red};
  int iColor = 0;
  for ( const QVector<QPointF>& losSamples : losSampleSet ) {
    QwtPlotCurve* curve = new QwtPlotCurve ( tr ( "Line of sight" ) );
    curve->setRenderHint ( QwtPlotItem::RenderAntialiased );
    QPen losPen;
    losPen.setColor ( colors[iColor] );
    losPen.setWidth ( 5 );
    curve->setPen ( losPen );
    curve->setData ( new QwtPointSeriesData() );
    static_cast<QwtPointSeriesData*> ( curve->data() )->setSamples ( losSamples );
    curve->attach ( mPlot );
    mLinesOfSight.append ( curve );

    KadasLineItem* line = new KadasLineItem ( mTool->canvas()->mapSettings().destinationCrs() );
    double lambda1 = losSamples.front().x() / mNSamples;
    double lambda2 = losSamples.back().x() / mNSamples;
    QgsPoint p1 ( mPoints[0] + ( mPoints[1] - mPoints[0] ) * lambda1 );
    QgsPoint p2 ( mPoints[0] + ( mPoints[1] - mPoints[0] ) * lambda2 );
    QgsLineString geom ( QgsPointSequence() << p1 << p2 );
    line->addPartFromGeometry ( &geom );
    line->setOutlineColor ( colors[iColor] );
    line->setOutlineWidth ( 5 );
    line->setZIndex ( 10 );
    KadasMapCanvasItemManager::instance()->addItem ( line );
    mLinesOfSightRB.append ( line );

    iColor = ( iColor + 1 ) % 2;
  }

  double observerHeight = mObserverHeightSpinBox->value();
  if ( heightRelToGround ) {
    observerHeight += samples.front().y();
  }
  mLineOfSightMarker = new QwtPlotMarker();
  QwtSymbol* observerMarkerSymbol = new QwtSymbol ( QwtSymbol::Pixmap );
  observerMarkerSymbol->setPixmap ( QPixmap ( ":/kadas/icons/observer" ) );
  observerMarkerSymbol->setPinPoint ( QPointF ( 0, 4 ) );
  mLineOfSightMarker->setSymbol ( observerMarkerSymbol );
  mLineOfSightMarker->setValue ( QPointF ( samples.front().x(), observerHeight ) );
  mLineOfSightMarker->attach ( mPlot );

  mPlot->replot();
}

void KadasHeightProfileDialog::copyToClipboard()
{
  QImage image ( mPlot->size(), QImage::Format_ARGB32 );
  mPlotMarker->setVisible ( false );
  mPlot->replot();
  mPlot->render ( &image );
  mPlotMarker->setVisible ( true );
  mPlot->replot();
  QApplication::clipboard()->setImage ( image );
}

void KadasHeightProfileDialog::addToCanvas()
{
  QImage image ( mPlot->size(), QImage::Format_ARGB32 );
  mPlotMarker->setVisible ( false );
  mPlot->replot();
  mPlot->render ( &image );
  mPlotMarker->setVisible ( true );
  mPlot->replot();
  // TODO
//  QgsImageAnnotationItem* item = new QgsImageAnnotationItem( mTool->canvas() );
//  item->setImage( image );
//  item->setMapPosition( mTool->canvas()->mapSettings().extent().center() );
}

void KadasHeightProfileDialog::keyPressEvent ( QKeyEvent* ev )
{
  if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
    ev->ignore();
  } else {
    QDialog::keyPressEvent ( ev );
  }
}
