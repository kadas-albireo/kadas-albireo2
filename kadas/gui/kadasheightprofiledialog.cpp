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
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>
#include <qwt_text.h>

#include <gdal.h>

#include <qgis/qgsapplication.h>
#include <qgis/qgsdistancearea.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssettings.h>
#include <qgis/qgsvector.h>

#include <kadas/core/kadas.h>
#include <kadas/core/kadascoordinateformat.h>
#include <kadas/gui/kadasheightprofiledialog.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/maptools/kadasmaptoolheightprofile.h>


class KadasHeightProfileDialog::ScaleDraw : public QwtScaleDraw
{
  public:
    ScaleDraw( double maxVal, int nSamples ) : mFactor( maxVal / nSamples )
    {
    }
    QwtText label( double value ) const override
    {
      return QLocale::system().toString( value * mFactor );
    }
  private:
    double mFactor;
};

class PaddedPlotMarker : public QwtPlotMarker
{
  public:
    PaddedPlotMarker() : QwtPlotMarker() {}
    void getCanvasMarginHint( const QwtScaleMap & /*xMap*/, const QwtScaleMap & /*yMap*/, const QRectF & /*canvasSize*/, double &left, double &top, double &right, double &bottom ) const override
    {
      left = 15;
      right = 15;
      top = 15;
      bottom = 5;
    }
};

class PlotMousePicker : public QwtPlotPicker
{
  public:
    typedef std::function<void( const QPoint & )> PickCallback_t;
    PlotMousePicker( QWidget *canvas, const PickCallback_t &callback )
      : QwtPlotPicker( QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::VLineRubberBand, QwtPicker::AlwaysOn, canvas )
      , mCallback( callback )
    {
      setStateMachine( new QwtPickerTrackerMachine() );
    }
  signals:
    void plotPicked( const QPoint &pos );

  protected:
    QwtText trackerText( const QPoint &pos ) const override
    {
      mCallback( pos );
      return QwtText();
    }

  private:
    std::function<void( const QPoint & )> mCallback;
};

KadasHeightProfileDialog::KadasHeightProfileDialog( KadasMapToolHeightProfile *tool, QWidget *parent, Qt::WindowFlags f )
  : QDialog( parent, f ), mTool( tool )
{
  setWindowTitle( tr( "Height profile" ) );
  setAttribute( Qt::WA_ShowWithoutActivating );
  QVBoxLayout *vboxLayout = new QVBoxLayout( this );

  QPushButton *pickButton = new QPushButton( QgsApplication::getThemeIcon( "/mActionSelect.svg" ), tr( "Measure along existing line" ), this );
  connect( pickButton, &QPushButton::clicked, mTool, &KadasMapToolHeightProfile::pickLine );
  vboxLayout->addWidget( pickButton );

  Qgis::DistanceUnit heightDisplayUnit = KadasCoordinateFormat::instance()->getHeightDisplayUnit();

  mPlot = new QwtPlot( this );
  mPlot->canvas()->setCursor( Qt::ArrowCursor );
  mPlot->setCanvasBackground( Qt::white );
  mPlot->enableAxis( QwtPlot::yLeft );
  mPlot->enableAxis( QwtPlot::xBottom );
  mPlot->setAxisTitle( QwtPlot::yLeft, heightDisplayUnit == Qgis::DistanceUnit::Feet ? tr( "Height [ft AMSL]" ) : tr( "Height [m AMSL]" ) );
  mPlot->setAxisTitle( QwtPlot::xBottom, tr( "Distance [m]" ) );
  mPlot->setMinimumHeight( 200 );
  mPlot->setSizePolicy( mPlot->sizePolicy().horizontalPolicy(), QSizePolicy::MinimumExpanding );
  vboxLayout->addWidget( mPlot );

  QwtPlotGrid *grid = new QwtPlotGrid();
  grid->setMajorPen( QPen( Qt::gray ) );
  grid->attach( mPlot );

  mPlotMarker = new PaddedPlotMarker();
  mPlotMarker->setSymbol( new QwtSymbol( QwtSymbol::Ellipse, QBrush( Qt::blue ), QPen( Qt::blue ), QSize( 5, 5 ) ) );
  mPlotMarker->setItemAttribute( QwtPlotItem::Margins );
  mPlotMarker->attach( mPlot );
  mPlotMarker->setLabelAlignment( Qt::AlignTop | Qt::AlignHCenter );

  mPlotPicker = new PlotMousePicker( mPlot->canvas(), [this]( const QPoint & pos ) { setMarkerPlotPos( pos ); } );

  mNodeMarkersCheckbox = new QCheckBox( tr( "Show vertex lines" ) );
  mNodeMarkersCheckbox->setChecked( QgsSettings().value( "/kadas/heightprofile_nodemarkers", true ).toBool() );
  connect( mNodeMarkersCheckbox, &QCheckBox::toggled, this, &KadasHeightProfileDialog::toggleNodeMarkers );
  vboxLayout->addWidget( mNodeMarkersCheckbox );

  mLineOfSightGroupBoxgroupBox = new QGroupBox( this );
  mLineOfSightGroupBoxgroupBox->setTitle( tr( "Line of sight" ) );
  mLineOfSightGroupBoxgroupBox->setCheckable( true );
  connect( mLineOfSightGroupBoxgroupBox, &QGroupBox::clicked, this, &KadasHeightProfileDialog::updateLineOfSight );

  QGridLayout *layoutLOS = new QGridLayout();
  layoutLOS->setContentsMargins( 2, 2, 2, 2 );
  mLineOfSightGroupBoxgroupBox->setLayout( layoutLOS );
  layoutLOS->addWidget( new QLabel( tr( "Observer height:" ) ), 0, 0 );

  mObserverHeightSpinBox = new QDoubleSpinBox();
  mObserverHeightSpinBox->setRange( 0, 8000 );
  mObserverHeightSpinBox->setDecimals( 1 );
  mObserverHeightSpinBox->setSuffix( heightDisplayUnit == Qgis::DistanceUnit::Feet ? " ft" : " m" );
  mObserverHeightSpinBox->setValue( QgsSettings().value( "/kadas/heightprofile_observerheight", 2.0 ).toDouble() );
  mObserverHeightSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  connect( mObserverHeightSpinBox, qOverload<double> ( &QDoubleSpinBox::valueChanged ), this, &KadasHeightProfileDialog::updateLineOfSight );

  layoutLOS->addWidget( mObserverHeightSpinBox, 0, 1 );
  layoutLOS->addWidget( new QLabel( tr( "Target height:" ) ), 0, 2 );

  mTargetHeightSpinBox = new QDoubleSpinBox();
  mTargetHeightSpinBox->setRange( 0, 8000 );
  mTargetHeightSpinBox->setDecimals( 1 );
  mTargetHeightSpinBox->setSuffix( heightDisplayUnit == Qgis::DistanceUnit::Feet ? " ft" : " m" );
  mTargetHeightSpinBox->setValue( QgsSettings().value( "/kadas/heightprofile_targetheight", 2.0 ).toDouble() );
  mTargetHeightSpinBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  connect( mTargetHeightSpinBox, qOverload<double> ( &QDoubleSpinBox::valueChanged ), this, &KadasHeightProfileDialog::updateLineOfSight );

  layoutLOS->addWidget( mTargetHeightSpinBox, 0, 3 );
  layoutLOS->addWidget( new QLabel( tr( "Heights relative to:" ) ), 0, 4 );

  mHeightModeCombo = new QComboBox();
  mHeightModeCombo->addItem( tr( "Ground" ), static_cast<int>( HeightRelToGround ) );
  mHeightModeCombo->addItem( tr( "Sea level" ), static_cast<int>( HeightRelToSeaLevel ) );
  connect( mHeightModeCombo, qOverload<int> ( &QComboBox::currentIndexChanged ), this, &KadasHeightProfileDialog::updateLineOfSight );
  layoutLOS->addWidget( mHeightModeCombo, 0, 5 );

  vboxLayout->addWidget( mLineOfSightGroupBoxgroupBox );

  QHBoxLayout *hboxLayout = new QHBoxLayout();

  mProgressBar = new QProgressBar();
  mProgressBar->hide();
  hboxLayout->addWidget( mProgressBar );

  mCancelButton = new QPushButton();
  mCancelButton->setCheckable( true );
  mCancelButton->setIcon( QgsApplication::getThemeIcon( "/mTaskCancel.svg" ) );
  mCancelButton->hide();
  hboxLayout->addWidget( mCancelButton );

  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  QPushButton *copyButton = bbox->addButton( tr( "Copy to clipboard" ), QDialogButtonBox::ActionRole );
  QPushButton *addButton = bbox->addButton( tr( "Add to map" ), QDialogButtonBox::ActionRole );
  hboxLayout->addWidget( bbox );

  vboxLayout->addLayout( hboxLayout );
  connect( bbox, &QDialogButtonBox::accepted, this, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, this, &QDialog::reject );
  connect( copyButton, &QPushButton::clicked, this, &KadasHeightProfileDialog::copyToClipboard );
  connect( addButton, &QPushButton::clicked, this, &KadasHeightProfileDialog::addToCanvas );
  connect( this, &QDialog::finished, this, &KadasHeightProfileDialog::finish );

  connect( KadasCoordinateFormat::instance(), &KadasCoordinateFormat::heightDisplayUnitChanged, this, &KadasHeightProfileDialog::replot );

  restoreGeometry( QgsSettings().value( "/Windows/MeasureHeightProfile/geometry" ).toByteArray() );
}

void KadasHeightProfileDialog::setPoints( const QList<QgsPointXY> &points, const QgsCoordinateReferenceSystem &crs )
{
  mPoints = points;
  mPointsCrs = crs;
  mTotLength = 0;
  mTotLengthMeters = 0;
  mSegmentLengths.clear();

  QgsDistanceArea da;
  da.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", "NONE" ) );
  da.setSourceCrs( crs, QgsProject::instance()->transformContext() );

  for ( int i = 0, n = mPoints.size() - 1; i < n; ++i )
  {
    mSegmentLengths.append( std::sqrt( mPoints[i + 1].sqrDist( mPoints[i] ) ) );
    mTotLength += mSegmentLengths.back();
    mTotLengthMeters += da.measureLine( mPoints[i], mPoints[i + 1] );
  }
  mLineOfSightGroupBoxgroupBox->setEnabled( points.size() == 2 );

  // At least heightprofile_samples samples or 1 sample per 10m, whichever is larger
  mNSamples = std::max( QgsSettings().value( "/kadas/heightprofile_samples", 1000 ).toInt(), int( mTotLengthMeters / 10 ) );
  replot();
}

void KadasHeightProfileDialog::setMarkerPos( int segment, const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs )
{
  if ( isBusy() || mSegmentLengths.isEmpty() )
  {
    return;
  }

  QgsPointXY pos = QgsCoordinateTransform( crs, mPointsCrs, QgsProject::instance() ).transform( p );
  double x = std::sqrt( pos.sqrDist( mPoints[segment] ) );
  for ( int i = 0; i < segment; ++i )
  {
    x += mSegmentLengths[i];
  }
  int idx = std::min( int ( x / mTotLength * mNSamples ), mNSamples - 1 );
  QPointF sample = mPlotSamples.at( idx );
  mPlotMarker->setValue( sample );
  mPlotMarker->setLabel( sample.y() == mNoDataValue ? "" : QString::number( qRound( sample.y() ) ) );
  mPlot->replot();
}

void KadasHeightProfileDialog::setMarkerPlotPos( const QPoint &pos )
{
  if ( isBusy() || mSegmentLengths.isEmpty() )
  {
    return;
  }

  int idx = mPlot->invTransform( QwtPlot::xBottom, pos.x() );
  QPointF sample = mPlotSamples.at( idx );
  mPlotMarker->setValue( sample );
  mPlotMarker->setLabel( sample.y() == mNoDataValue ? "" : QString::number( qRound( sample.y() ) ) );
  mPlot->replot();
  double distance = sample.x() / double( mNSamples ) * mTotLength;
  mTool->setMarkerPos( distance );
}

void KadasHeightProfileDialog::clear()
{
  mSegmentLengths.clear();
  mPlotSamples.clear();
  mPlotMarker->setValue( 0, 0 );
  qDeleteAll( mPlotCurves );
  mPlotCurves.clear();
  qDeleteAll( mLinesOfSight );
  mLinesOfSight.clear();
  qDeleteAll( mLinesOfSightRB );
  mLinesOfSightRB.clear();
  delete mLineOfSightMarker;
  mLineOfSightMarker = nullptr;
  qDeleteAll( mNodeMarkers );
  mNodeMarkers.clear();
  mPlot->replot();
  mLineOfSightGroupBoxgroupBox->setEnabled( false );
}

bool KadasHeightProfileDialog::isBusy() const
{
  return mCancelButton->isVisible();
}

void KadasHeightProfileDialog::accept()
{
  if ( !isBusy() )
  {
    QDialog::accept();
  }
}

void KadasHeightProfileDialog::reject()
{
  if ( !isBusy() )
  {
    QDialog::reject();
  }
}

void KadasHeightProfileDialog::finish()
{
  QgsSettings().setValue( "/Windows/MeasureHeightProfile/geometry", saveGeometry() );
  mTool->canvas()->unsetMapTool( mTool );
  clear();
}

void KadasHeightProfileDialog::replot()
{
  Qgis::DistanceUnit vertDisplayUnit = KadasCoordinateFormat::instance()->getHeightDisplayUnit();
  mPlot->setAxisTitle( QwtPlot::yLeft, vertDisplayUnit == Qgis::DistanceUnit::Feet ? tr( "Height [ft AMSL]" ) : tr( "Height [m AMSL]" ) );
  mObserverHeightSpinBox->setSuffix( vertDisplayUnit == Qgis::DistanceUnit::Feet ? " ft" : " m" );
  mTargetHeightSpinBox->setSuffix( vertDisplayUnit == Qgis::DistanceUnit::Feet ? " ft" : " m" );

  if ( mPoints.isEmpty() )
  {
    return;
  }

  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != Qgis::LayerType::Raster )
  {
    emit mTool->messageEmitted( tr( "No heightmap is defined in the project. Right-click a raster layer in the layer tree and select it to be used as heightmap." ), Qgis::Warning );
    return;
  }

  QString errMsg;
  GDALDatasetH raster = Kadas::gdalOpenForLayer( static_cast<QgsRasterLayer *>( layer ), &errMsg );
  if ( !raster )
  {
    emit mTool->messageEmitted( tr( "Error: Unable to open heightmap." ), Qgis::Warning );
    return;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    QgsDebugMsgLevel( "Failed to get raster geotransform" , 2 );
    GDALClose( raster );
    return;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs( proj );
  if ( !rasterCrs.isValid() )
  {
    QgsDebugMsgLevel( "Failed to get raster CRS" , 2 );
    GDALClose( raster );
    return;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !band )
  {
    QgsDebugMsgLevel( "Failed to open raster band 0" , 2 );
    GDALClose( raster );
    return;
  }

  // Get vertical unit
  Qgis::DistanceUnit vertUnit = strcmp( GDALGetRasterUnitType( band ), "ft" ) == 0 ? Qgis::DistanceUnit::Feet : Qgis::DistanceUnit::Meters;
  double heightConversion = QgsUnitTypes::fromUnitToUnitFactor( vertUnit, vertDisplayUnit );
  mNoDataValue = GDALGetRasterNoDataValue( band, NULL );
  mPlot->setAxisTitle( QwtPlot::yLeft, vertDisplayUnit == Qgis::DistanceUnit::Feet ? tr( "Height [ft AMSL]" ) : tr( "Height [m AMSL]" ) );
  mObserverHeightSpinBox->setSuffix( vertDisplayUnit == Qgis::DistanceUnit::Feet ? " ft" : " m" );
  mTargetHeightSpinBox->setSuffix( vertDisplayUnit == Qgis::DistanceUnit::Feet ? " ft" : " m" );

  mProgressBar->setValue( 0 );
  mProgressBar->setRange( 0, mNSamples );
  mProgressBar->show();
  mCancelButton->show();
  mPlotSamples.clear();

  double x = 0;
  for ( int i = 0, n = mPoints.size() - 1; i < n; ++i )
  {
    if ( x >= mSegmentLengths[i] )
    {
      continue;
    }

    QgsVector dir = QgsVector( mPoints[i + 1] - mPoints[i] ).normalized();
    while ( x < mSegmentLengths[i] )
    {
      mProgressBar->setValue( mProgressBar->value() + 1 );
      QApplication::processEvents();
      if ( mCancelButton->isChecked() )
      {
        break;
      }
      QgsPointXY p = mPoints[i] + dir * x;
      // Transform geo position to raster CRS
      QgsPointXY pRaster = QgsCoordinateTransform( mPointsCrs, rasterCrs, QgsProject::instance() ).transform( p );


      // Transform raster geo position to pixel coordinates
      double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );
      double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );

      double pixValues[4] = {};
      if ( CE_None != GDALRasterIO( band, GF_Read,
                                    std::floor( col ), std::floor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
      {
        QgsDebugMsgLevel( "Failed to read pixel values" , 2 );
        mPlotSamples.append( QPointF( mPlotSamples.size(), 0 ) );
      }
      else
      {
        bool nodata = false;
        for ( int j = 0; j < 4; ++j )
        {
          if ( pixValues[j] == mNoDataValue )
          {
            nodata = true;
          }
        }
        if ( nodata )
        {
          mPlotSamples.append( QPointF( mPlotSamples.size(), mNoDataValue ) );
        }
        else
        {
          // Interpolate values
          double lambdaR = row - std::floor( row );
          double lambdaC = col - std::floor( col );

          double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                         + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
          mPlotSamples.append( QPointF( mPlotSamples.size(), value * heightConversion ) );
        }
      }
      x += mTotLength / mNSamples;
    }
    x -= mSegmentLengths[i];
    if ( mCancelButton->isChecked() )
    {
      break;
    }
  }
  bool cancelled = mCancelButton->isChecked();
  mCancelButton->setChecked( false );
  mCancelButton->hide();
  mProgressBar->hide();
  GDALClose( raster );
  if ( cancelled )
  {
    clear();
    return;
  }

  // Add separate plot curves for each contiguous set of samples without NODATA values
  QVector<QPointF> sampleSet;
  for ( const QPointF &p : mPlotSamples )
  {
    if ( p.y() == mNoDataValue )
    {
      if ( !sampleSet.isEmpty() )
      {
        QwtPlotCurve *plotCurve = new QwtPlotCurve( tr( "Height profile" ) );
        plotCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
        QPen curvePen;
        curvePen.setColor( Qt::red );
        curvePen.setJoinStyle( Qt::RoundJoin );
        plotCurve->setPen( curvePen );
        plotCurve->setBaseline( 0 );
        plotCurve->setBrush( QColor( 255, 127, 127 ) );
        plotCurve->attach( mPlot );
        plotCurve->setData( new QwtPointSeriesData( sampleSet ) );
        mPlotCurves.append( plotCurve );
        sampleSet.clear();
      }
    }
    else
    {
      sampleSet.append( p );
    }
  }
  if ( !sampleSet.isEmpty() )
  {
    QwtPlotCurve *plotCurve = new QwtPlotCurve( tr( "Height profile" ) );
    plotCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
    QPen curvePen;
    curvePen.setColor( Qt::red );
    curvePen.setJoinStyle( Qt::RoundJoin );
    plotCurve->setPen( curvePen );
    plotCurve->setBaseline( 0 );
    plotCurve->setBrush( QColor( 255, 127, 127 ) );
    plotCurve->attach( mPlot );
    plotCurve->setData( new QwtPointSeriesData( sampleSet ) );
    mPlotCurves.append( plotCurve );
    sampleSet.clear();
  }

  int nSamples = mPlotSamples.size();
  mPlotMarker->setValue( 0, 0 );
  mPlot->setAxisScaleDraw( QwtPlot::xBottom, new ScaleDraw( mTotLengthMeters, nSamples ) );
  double step = std::pow( 10, std::floor( log10( mTotLengthMeters ) ) ) / ( mTotLengthMeters ) * nSamples;
  while ( nSamples / step < 10 ) { step /= 2.; }
  while ( nSamples / step > 10 ) { step *= 2.; }
  mPlot->setAxisScale( QwtPlot::xBottom, 0, nSamples, step );


  // Node markers
  if ( mNodeMarkersCheckbox->isChecked() )
  {
    x = 0;
    for ( int i = 0, n = mPoints.size() - 2; i < n; ++i )
    {
      x += mSegmentLengths[i];
      QwtPlotMarker *nodeMarker = new QwtPlotMarker();
      nodeMarker->setLinePen( QPen( Qt::black, 1, Qt::DashLine ) );
      nodeMarker->setLineStyle( QwtPlotMarker::VLine );
      int idx = std::min( int ( x / mTotLength * mNSamples ), mNSamples - 1 );
      if ( idx < mPlotSamples.length() )
      {
        QPointF sample = mPlotSamples.at( idx );
        nodeMarker->setValue( sample );
        nodeMarker->attach( mPlot );
        mNodeMarkers.append( nodeMarker );
      }
    }
  }

  updateLineOfSight( );
}

void KadasHeightProfileDialog::updateLineOfSight( )
{
  QgsSettings().setValue( "/kadas/heightprofile_observerheight", mObserverHeightSpinBox->value() );
  QgsSettings().setValue( "/kadas/heightprofile_targetheight", mTargetHeightSpinBox->value() );

  qDeleteAll( mLinesOfSight );
  mLinesOfSight.clear();
  qDeleteAll( mLinesOfSightRB );
  mLinesOfSightRB.clear();
  delete mLineOfSightMarker;
  mLineOfSightMarker = 0;

  if ( !mLineOfSightGroupBoxgroupBox->isEnabled() || !mLineOfSightGroupBoxgroupBox->isChecked() )
  {
    mPlot->replot();
    return;
  }

  int nSamples = mPlotSamples.size();
  if ( nSamples < 2 )
  {
    mPlot->replot();
    return;
  }

  // If first point is noData, don't compute any line of sight
  if ( mPlotSamples.first().y() == mNoDataValue )
  {
    mPlot->replot();
    return;
  }

  QVector< QVector<QPointF> > losSampleSet;
  losSampleSet.append( QVector<QPointF>() );
  double targetHeight = mTargetHeightSpinBox->value();
  bool heightRelToGround = static_cast<HeightMode>( mHeightModeCombo->itemData( mHeightModeCombo->currentIndex() ).toInt() ) == HeightRelToGround;

  QVector<double> pX;
  pX.reserve( nSamples );
  QVector<double> pY;
  pY.reserve( nSamples );
  double earthRadius = 6370000;
  double meterToDisplayUnit = QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, KadasCoordinateFormat::instance()->getHeightDisplayUnit() );
  for ( const QPointF &p : mPlotSamples )
  {
    pX.append( p.x() / mNSamples * mTotLengthMeters );
    double hCorr = 0.87 * pX.last() * pX.last() / ( 2 * earthRadius ) * meterToDisplayUnit;
    pY.append( p.y() - hCorr );
  }

  // Visibility of first point
  double y = pY.front() + targetHeight - ( heightRelToGround ? 0 : mPlotSamples.front().y() );
  if ( y > pY.front() )
  {
    if ( mPlotSamples.front().y() != mNoDataValue )
    {
      losSampleSet.back().append( mPlotSamples.front() );
    }
  }
  else
  {
    losSampleSet.append( QVector<QPointF>() );
  }

  QPointF p1( pX.front(), ( heightRelToGround ? pY.front() : 0 ) + mObserverHeightSpinBox->value() );
  for ( int i = 1; i < nSamples; ++i )
  {
    QPointF p2( pX[i], pY[i] + targetHeight );
    if ( !heightRelToGround )
    {
      p2.ry() -= mPlotSamples[i].y();
    }
    // For each sample position along line [p1, p2], check if y is below terrain
    // X = p1.x() + d * (p2.x() - p1.x())
    // Y = p1.y() + d * (p2.y() - p1.y())
    // => d = (X - p1.x()) / (p2.x() - p1.x())
    // => Y = p1.y() + (X - p1.x()) / (p2.x() - p1.x()) * (p2.y() - p1.y())
    bool visible = true;
    for ( int j = 1; j < i; ++j )
    {
      double Y = p1.y() + ( pX[j] - p1.x() ) / ( p2.x() - p1.x() ) * ( p2.y() - p1.y() );
      if ( Y < pY[j] )
      {
        visible = false;
        break;
      }
    }
    if ( ( visible && losSampleSet.size() % 2 == 0 ) || ( !visible && losSampleSet.size() % 2 == 1 ) )
    {
      losSampleSet.append( QVector<QPointF>() );
    }
    if ( mPlotSamples[i].y() != mNoDataValue )
    {
      losSampleSet.back().append( mPlotSamples[i] );
    }
  }

  Qt::GlobalColor colors[] = {Qt::green, Qt::red};
  int iColor = 0;
  for ( const QVector<QPointF> &losSamples : losSampleSet )
  {
    if ( losSamples.isEmpty() )
    {
      continue;
    }
    QwtPlotCurve *curve = new QwtPlotCurve( tr( "Line of sight" ) );
    curve->setRenderHint( QwtPlotItem::RenderAntialiased );
    QPen losPen;
    losPen.setColor( colors[iColor] );
    losPen.setWidth( 5 );
    curve->setPen( losPen );
    curve->setData( new QwtPointSeriesData() );
    static_cast<QwtPointSeriesData *>( curve->data() )->setSamples( losSamples );
    curve->attach( mPlot );
    mLinesOfSight.append( curve );

    KadasLineItem *line = new KadasLineItem( mTool->canvas()->mapSettings().destinationCrs() );
    double lambda1 = losSamples.front().x() / mNSamples;
    double lambda2 = losSamples.back().x() / mNSamples;
    QgsPoint p1( mPoints[0] + ( mPoints[1] - mPoints[0] ) * lambda1 );
    QgsPoint p2( mPoints[0] + ( mPoints[1] - mPoints[0] ) * lambda2 );
    QgsLineString geom( QgsPointSequence() << p1 << p2 );
    line->addPartFromGeometry( geom );
    line->setOutline( QPen( colors[iColor], 5 ) );
    line->setZIndex( 10 );
    KadasMapCanvasItemManager::instance()->addItem( line );
    mLinesOfSightRB.append( line );

    iColor = ( iColor + 1 ) % 2;
  }

  double observerHeight = mObserverHeightSpinBox->value();
  if ( heightRelToGround )
  {
    observerHeight += mPlotSamples.front().y();
  }
  mLineOfSightMarker = new QwtPlotMarker();
  QwtSymbol *observerMarkerSymbol = new QwtSymbol( QwtSymbol::Pixmap );
  observerMarkerSymbol->setPixmap( QPixmap( ":/kadas/icons/observer" ) );
  observerMarkerSymbol->setPinPoint( QPointF( 0, 4 ) );
  mLineOfSightMarker->setSymbol( observerMarkerSymbol );
  mLineOfSightMarker->setValue( QPointF( mPlotSamples.front().x(), observerHeight ) );
  mLineOfSightMarker->attach( mPlot );

  mPlot->replot();
}

void KadasHeightProfileDialog::copyToClipboard()
{
  QImage image( mPlot->size(), QImage::Format_ARGB32 );
  mPlotMarker->setVisible( false );
  mPlot->replot();
  mPlot->render( &image );
  mPlotMarker->setVisible( true );
  mPlot->replot();
  QApplication::clipboard()->setImage( image );
}

void KadasHeightProfileDialog::addToCanvas()
{
  QImage image( mPlot->size(), QImage::Format_ARGB32 );
  mPlotMarker->setVisible( false );
  mPlot->replot();
  mPlot->render( &image );
  mPlotMarker->setVisible( true );
  mPlot->replot();

  QString filename = QgsProject::instance()->createAttachedFile( "heightProfile.png" );
  image.save( filename );

  KadasSymbolItem *item = new KadasSymbolItem( mTool->canvas()->mapSettings().destinationCrs() );
  item->setFilePath( filename );
  item->setPosition( KadasItemPos::fromPoint( mTool->canvas()->extent().center() ) );
  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::SymbolsLayer )->addItem( item );
  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::SymbolsLayer )->triggerRepaint();
}

void KadasHeightProfileDialog::keyPressEvent( QKeyEvent *ev )
{
  if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return )
  {
    ev->ignore();
  }
  else
  {
    QDialog::keyPressEvent( ev );
  }
}

void KadasHeightProfileDialog::toggleNodeMarkers()
{
  QgsSettings().setValue( "/kadas/heightprofile_nodemarkers", mNodeMarkersCheckbox->isChecked() );
  qDeleteAll( mNodeMarkers );
  mNodeMarkers.clear();
  replot();
}

#include "moc_kadasheightprofiledialog.cpp"
