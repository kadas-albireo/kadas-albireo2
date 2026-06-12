/***************************************************************************
    kadasmaptoolmeasure.cpp
    -----------------------
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

#include <cmath>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QToolButton>

#include <qgis/qgsabstractgeometry.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapcanvasitem.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include "kadas/gui/kadasbottombar.h"
#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/maptools/kadasmaptoolmeasure.h"


/**
 * Transient canvas overlay drawing the per-segment / per-part measurement
 * labels on top of the capture rubber band, replicating the label style of
 * the legacy KadasGeometryItem measurement rendering.
 */
class KadasMeasureLabelsOverlay : public QgsMapCanvasItem
{
  public:
    struct Label
    {
        QgsPointXY pos; // canvas CRS
        QString text;
        bool center = true; // false: offset below the anchor (e.g. total at last vertex)
    };

    KadasMeasureLabelsOverlay( QgsMapCanvas *canvas )
      : QgsMapCanvasItem( canvas )
    {
      setZValue( 100 );
      updatePosition();
    }

    void setLabels( const QList<Label> &labels )
    {
      mLabels = labels;
      updatePosition();
    }

    void updatePosition() override { setRect( mMapCanvas->extent() ); }

    void paint( QPainter *painter ) override
    {
      if ( mLabels.isEmpty() )
        return;

      const int red = QgsSettings().value( "/Qgis/default_measure_color_red", 255 ).toInt();
      const int green = QgsSettings().value( "/Qgis/default_measure_color_green", 0 ).toInt();
      const int blue = QgsSettings().value( "/Qgis/default_measure_color_blue", 0 ).toInt();

      QFont font = painter->font();
      font.setPixelSize( 10 );
      font.setBold( true );
      painter->setFont( font );
      painter->setPen( QColor( red, green, blue ) );
      const QFontMetrics metrics( font );

      static const int sLabelOffset = 16;
      const QColor rectColor( 255, 255, 255, 192 );
      for ( const Label &label : mLabels )
      {
        const QPointF anchor = toCanvasCoordinates( label.pos ) - pos();
        const QStringList lines = label.text.split( '\n' );
        int width = 0;
        for ( const QString &line : lines )
          width = std::max( width, metrics.horizontalAdvance( line ) );
        width += 6;
        const int height = metrics.height() * lines.size() + 6;
        const QRectF labelRect( anchor.x() - 0.5 * width, anchor.y() + ( label.center ? 0 : sLabelOffset ) - 0.5 * height, width, height );
        painter->fillRect( labelRect, rectColor );
        painter->drawText( labelRect, Qt::AlignCenter | Qt::AlignVCenter, label.text );
      }
    }

  private:
    QList<Label> mLabels;
};


static QVector<QVector<QgsPointXY>> extractPolylines( const QgsGeometry &g )
{
  QVector<QVector<QgsPointXY>> polylines;
  const QgsAbstractGeometry *ag = g.constGet();
  if ( !ag )
    return polylines;
  if ( ag->wkbType() == Qgis::WkbType::LineString || ag->wkbType() == Qgis::WkbType::LineString25D )
  {
    polylines.append( g.asPolyline().toVector() );
  }
  else
  {
    for ( const QgsPolylineXY &pl : g.asMultiPolyline() )
      polylines.append( pl.toVector() );
  }
  return polylines;
}


static KadasShapeCaptureMapTool::Shape shapeFor( KadasMapToolMeasure::MeasureMode m )
{
  switch ( m )
  {
    case KadasMapToolMeasure::MeasureMode::MeasureLine:
      return KadasShapeCaptureMapTool::Shape::Polyline;
    case KadasMapToolMeasure::MeasureMode::MeasurePolygon:
      return KadasShapeCaptureMapTool::Shape::Polygon;
    case KadasMapToolMeasure::MeasureMode::MeasureCircle:
      return KadasShapeCaptureMapTool::Shape::Circle;
  }
  return KadasShapeCaptureMapTool::Shape::Polyline;
}


KadasMapToolMeasure::KadasMapToolMeasure( QgsMapCanvas *canvas, MeasureMode measureMode )
  : KadasShapeCaptureMapTool( canvas, shapeFor( measureMode ) )
  , mMeasureMode( measureMode )
{
  setCursor( Qt::ArrowCursor );
  if ( measureMode == MeasureMode::MeasureLine )
  {
    // Draw long line segments as densified great circles, as the measurements are geodesic
    setGeodesicPreview( true );
  }
  connect( this, &KadasShapeCaptureMapTool::shapeCaptured, this, &KadasMapToolMeasure::onShapeCaptured );
  connect( this, &KadasShapeCaptureMapTool::previewChanged, this, &KadasMapToolMeasure::recomputeReadout );
  connect( this, &KadasShapeCaptureMapTool::cleared, this, &KadasMapToolMeasure::recomputeReadout );
}

KadasMapToolMeasure::~KadasMapToolMeasure()
{
  delete mBottomBar;
  delete mLabelsOverlay;
}

void KadasMapToolMeasure::activate()
{
  KadasShapeCaptureMapTool::activate();

  mDa.setSourceCrs( canvas()->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
  mDa.setEllipsoid( QgsProject::instance()->ellipsoid() );

  mLabelsOverlay = new KadasMeasureLabelsOverlay( canvas() );

  mBottomBar = new KadasBottomBar( canvas() );
  QHBoxLayout *layout = new QHBoxLayout( mBottomBar );
  layout->setContentsMargins( 8, 4, 8, 4 );
  layout->setSpacing( 2 );

  QGridLayout *gridLayout = new QGridLayout();
  gridLayout->setContentsMargins( 0, 0, 0, 0 );
  gridLayout->setSpacing( 2 );
  QWidget *grid = new QWidget();
  grid->setLayout( gridLayout );
  layout->addWidget( grid );

  QString toolLabel;
  switch ( mMeasureMode )
  {
    case MeasureMode::MeasureLine:
      toolLabel = tr( "Measure line" );
      break;
    case MeasureMode::MeasurePolygon:
      toolLabel = tr( "Measure polygon" );
      break;
    case MeasureMode::MeasureCircle:
      toolLabel = tr( "Measure circle" );
      break;
  }
  mTitleLabel = new QLabel( QString( "<b>%1</b>" ).arg( toolLabel ) );
  gridLayout->addWidget( mTitleLabel, 0, 0 );

  mReadoutLabel = new QLabel();
  mReadoutLabel->setTextInteractionFlags( Qt::TextSelectableByMouse );
  mReadoutLabel->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
  gridLayout->addWidget( mReadoutLabel, 0, 1 );

  mUnitComboBox = new QComboBox();
  mUnitComboBox->addItem( tr( "Metric" ), static_cast<int>( Qgis::DistanceUnit::Meters ) );
  mUnitComboBox->addItem( tr( "Imperial" ), static_cast<int>( Qgis::DistanceUnit::Feet ) );
  mUnitComboBox->addItem( tr( "Nautical" ), static_cast<int>( Qgis::DistanceUnit::NauticalMiles ) );
  const int defUnit = QgsSettings().value( "/kadas/last_measure_unit", static_cast<int>( Qgis::DistanceUnit::Meters ) ).toInt();
  mUnitComboBox->setCurrentIndex( mUnitComboBox->findData( defUnit ) );
  connect( mUnitComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, [this]( int ) {
    QgsSettings().setValue( "/kadas/last_measure_unit", mUnitComboBox->currentData().toInt() );
    recomputeReadout();
  } );
  gridLayout->addWidget( mUnitComboBox, 0, 2 );

  if ( mMeasureMode == MeasureMode::MeasureLine )
  {
    mAzimuthCheckbox = new QCheckBox( tr( "Azimuth" ) );
    mAzimuthCheckbox->setChecked( QgsSettings().value( "kadas/last_azimuth_enabled", true ).toBool() );
    connect( mAzimuthCheckbox, &QCheckBox::toggled, this, [this]( bool enabled ) {
      QgsSettings().setValue( "/kadas/last_azimuth_enabled", enabled );
      mNorthComboBox->setEnabled( enabled );
      mAngleUnitComboBox->setEnabled( enabled );
      recomputeReadout();
    } );
    gridLayout->addWidget( mAzimuthCheckbox, 1, 0 );

    mNorthComboBox = new QComboBox();
    mNorthComboBox->addItem( tr( "Geographic north" ), QVariant::fromValue( AzimuthNorth::AzimuthGeoNorth ) );
    mNorthComboBox->addItem( tr( "Map north" ), QVariant::fromValue( AzimuthNorth::AzimuthMapNorth ) );
    mNorthComboBox->setCurrentIndex( mNorthComboBox->findData( QVariant::fromValue( settingsLastAzimuthNorth->value() ) ) );
    mNorthComboBox->setEnabled( mAzimuthCheckbox->isChecked() );
    connect( mNorthComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, [this]( int ) {
      settingsLastAzimuthNorth->setValue( mNorthComboBox->currentData().value<AzimuthNorth>() );
      recomputeReadout();
    } );
    gridLayout->addWidget( mNorthComboBox, 1, 1 );

    mAngleUnitComboBox = new QComboBox();
    mAngleUnitComboBox->addItem( tr( "Degrees" ), static_cast<int>( Qgis::AngleUnit::Degrees ) );
    mAngleUnitComboBox->addItem( tr( "Radians" ), static_cast<int>( Qgis::AngleUnit::Radians ) );
    mAngleUnitComboBox->addItem( tr( "Gradians" ), static_cast<int>( Qgis::AngleUnit::Gon ) );
    mAngleUnitComboBox->addItem( tr( "Angular Mil" ), static_cast<int>( Qgis::AngleUnit::MilNATO ) );
    const int defAngleUnit = std::max( 0, QgsSettings().value( "/kadas/last_azimuth_unit", static_cast<int>( Qgis::AngleUnit::MilNATO ) ).toInt() );
    mAngleUnitComboBox->setCurrentIndex( mAngleUnitComboBox->findData( defAngleUnit ) );
    mAngleUnitComboBox->setEnabled( mAzimuthCheckbox->isChecked() );
    connect( mAngleUnitComboBox, qOverload<int>( &QComboBox::currentIndexChanged ), this, [this]( int ) {
      QgsSettings().setValue( "/kadas/last_azimuth_unit", mAngleUnitComboBox->currentData().toInt() );
      recomputeReadout();
    } );
    gridLayout->addWidget( mAngleUnitComboBox, 1, 2 );
  }

  QToolButton *pickButton = new QToolButton();
  pickButton->setIcon( QIcon( ":/kadas/icons/select" ) );
  pickButton->setToolTip( tr( "Pick existing geometry" ) );
  connect( pickButton, &QToolButton::clicked, this, &KadasMapToolMeasure::requestPick );
  layout->addWidget( pickButton );

  QToolButton *clearButton = new QToolButton();
  clearButton->setIcon( QIcon( ":/kadas/icons/clear" ) );
  clearButton->setToolTip( tr( "Clear" ) );
  connect( clearButton, &QToolButton::clicked, this, [this] {
    mParts.clear();
    clear();
    recomputeReadout();
  } );
  layout->addWidget( clearButton );

  QPushButton *closeButton = new QPushButton();
  closeButton->setIcon( QIcon( ":/kadas/icons/close" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool( this ); } );
  layout->addWidget( closeButton );

  mBottomBar->setLayout( layout );
  mBottomBar->adjustSize();
  mBottomBar->show();

  recomputeReadout();
}

void KadasMapToolMeasure::deactivate()
{
  delete mBottomBar;
  mBottomBar = nullptr;
  delete mLabelsOverlay;
  mLabelsOverlay = nullptr;
  mTitleLabel = nullptr;
  mReadoutLabel = nullptr;
  mUnitComboBox = nullptr;
  mAngleUnitComboBox = nullptr;
  mNorthComboBox = nullptr;
  mAzimuthCheckbox = nullptr;
  mParts.clear();
  KadasShapeCaptureMapTool::deactivate();
}

void KadasMapToolMeasure::canvasPressEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
    KadasShapeCaptureMapTool::canvasPressEvent( e );
}

void KadasMapToolMeasure::canvasReleaseEvent( QgsMapMouseEvent *e )
{
  if ( !mPickFeature )
  {
    KadasShapeCaptureMapTool::canvasReleaseEvent( e );
    return;
  }
  KadasFeaturePicker::PickResult pickResult
    = KadasFeaturePicker::pick( canvas(), toMapCoordinates( e->pos() ), mMeasureMode == MeasureMode::MeasureLine ? Qgis::GeometryType::Line : Qgis::GeometryType::Polygon );
  if ( pickResult.geom )
  {
    QgsGeometry g( pickResult.geom->clone() );
    if ( pickResult.crs.isValid() )
    {
      QgsCoordinateTransform ct( pickResult.crs, canvas()->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
      try
      {
        g.transform( ct );
      }
      catch ( ... )
      {}
    }
    onShapeCaptured( g, canvas()->mapSettings().destinationCrs() );
  }
  mPickFeature = false;
  setCursor( Qt::ArrowCursor );
}

void KadasMapToolMeasure::keyReleaseEvent( QKeyEvent *e )
{
  if ( mPickFeature && e->key() == Qt::Key_Escape )
  {
    mPickFeature = false;
    setCursor( Qt::ArrowCursor );
    return;
  }
  KadasShapeCaptureMapTool::keyReleaseEvent( e );
}

void KadasMapToolMeasure::requestPick()
{
  mPickFeature = true;
  setCursor( QCursor( Qt::CrossCursor ) );
}

void KadasMapToolMeasure::onShapeCaptured( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &crs )
{
  Q_UNUSED( crs ); // helper always emits in canvas CRS
  if ( geometry.isEmpty() )
    return;
  Part p;
  p.geometry = geometry;
  if ( mMeasureMode == MeasureMode::MeasureCircle )
  {
    p.circleCenter = circleCenter();
    p.circleRadius = circleRadius();
  }
  mParts.append( p );
  recomputeReadout();
}

QString KadasMapToolMeasure::formatLength( double meters, Qgis::DistanceUnit unit ) const
{
  const int decimals = QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt();
  const double v = meters * QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, unit );
  return QgsUnitTypes::formatDistance( v, decimals, unit );
}

QString KadasMapToolMeasure::formatArea( double sqMeters, Qgis::AreaUnit unit ) const
{
  const int decimals = QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt();
  const double v = sqMeters * QgsUnitTypes::fromUnitToUnitFactor( Qgis::AreaUnit::SquareMeters, unit );
  if ( unit == Qgis::AreaUnit::SquareMeters )
  {
    if ( v >= 1000000 )
      return QString( "%1 km²" ).arg( v / 1000000., 0, 'f', decimals );
    return QString( "%1 m²" ).arg( v, 0, 'f', decimals );
  }
  return QgsUnitTypes::formatArea( v, decimals, unit );
}

QString KadasMapToolMeasure::formatAngle( double radians, Qgis::AngleUnit unit ) const
{
  const int decimals = QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt();
  const double v = radians * QgsUnitTypes::fromUnitToUnitFactor( Qgis::AngleUnit::Radians, unit );
  return QgsUnitTypes::formatAngle( v, decimals, unit );
}

double KadasMapToolMeasure::computeSegmentAzimuth( const QgsPointXY &p1, const QgsPointXY &p2, bool geoNorth ) const
{
  double angle = geoNorth ? mDa.bearing( p1, p2 ) : std::atan2( p2.x() - p1.x(), p2.y() - p1.y() );
  angle = std::round( angle * 1000. ) / 1000.;
  if ( angle < 0 )
    angle += 2 * M_PI;
  if ( angle >= 2 * M_PI )
    angle -= 2 * M_PI;
  return angle;
}

QString KadasMapToolMeasure::lineReadout( const QgsGeometry &g, double &totalLength ) const
{
  const Qgis::DistanceUnit distUnit = mUnitComboBox ? static_cast<Qgis::DistanceUnit>( mUnitComboBox->currentData().toInt() ) : Qgis::DistanceUnit::Meters;
  const bool azimuthEnabled = mAzimuthCheckbox && mAzimuthCheckbox->isChecked();
  const Qgis::AngleUnit angleUnit = mAngleUnitComboBox ? static_cast<Qgis::AngleUnit>( mAngleUnitComboBox->currentData().toInt() ) : Qgis::AngleUnit::MilNATO;
  const bool geoNorth = !mNorthComboBox || mNorthComboBox->currentData().value<AzimuthNorth>() == AzimuthNorth::AzimuthGeoNorth;

  QStringList lines;
  double partTotal = 0.0;

  const QVector<QVector<QgsPointXY>> polylines = extractPolylines( g );

  for ( const QVector<QgsPointXY> &part : polylines )
  {
    if ( part.size() < 2 )
      continue;
    double partLen = 0.0;
    for ( int i = 1, n = part.size(); i < n; ++i )
    {
      const QgsPointXY &p1 = part[i - 1];
      const QgsPointXY &p2 = part[i];
      const double segLen = mDa.measureLine( p1, p2 );
      partLen += segLen;
      QString line = QStringLiteral( "  %1: %2" ).arg( i ).arg( formatLength( segLen, distUnit ) );
      if ( azimuthEnabled )
      {
        const double az = computeSegmentAzimuth( p1, p2, geoNorth );
        line += QStringLiteral( " — %1" ).arg( formatAngle( az, angleUnit ) );
      }
      lines.append( line );
    }
    partTotal += partLen;
  }
  totalLength += partTotal;
  lines.prepend( tr( "Length: %1" ).arg( formatLength( partTotal, distUnit ) ) );
  return lines.join( QStringLiteral( "\n" ) );
}

QString KadasMapToolMeasure::polygonReadout( const QgsGeometry &g, double &totalArea ) const
{
  const Qgis::AreaUnit areaUnit = QgsUnitTypes::distanceToAreaUnit( mUnitComboBox ? static_cast<Qgis::DistanceUnit>( mUnitComboBox->currentData().toInt() ) : Qgis::DistanceUnit::Meters );
  const double area = mDa.measureArea( g );
  totalArea += area;
  return tr( "Area: %1" ).arg( formatArea( area, areaUnit ) );
}

QString KadasMapToolMeasure::circleReadout( const QgsGeometry &g, double &totalArea ) const
{
  Q_UNUSED( g );
  // Caller passes empty g; we rely on the Part's center/radius instead via recomputeReadout.
  Q_UNUSED( totalArea );
  return QString();
}

void KadasMapToolMeasure::recomputeReadout()
{
  if ( !mReadoutLabel )
    return;

  const Qgis::DistanceUnit distUnit = mUnitComboBox ? static_cast<Qgis::DistanceUnit>( mUnitComboBox->currentData().toInt() ) : Qgis::DistanceUnit::Meters;
  const Qgis::AreaUnit areaUnit = QgsUnitTypes::distanceToAreaUnit( distUnit );

  // Include the live capture preview as a transient extra part
  QList<Part> parts = mParts;
  if ( isCapturing() )
  {
    const QgsGeometry preview = previewGeometry();
    if ( !preview.isEmpty() )
    {
      Part p;
      p.geometry = preview;
      if ( mMeasureMode == MeasureMode::MeasureCircle )
      {
        p.circleCenter = circleCenter();
        p.circleRadius = circleRadius();
      }
      parts.append( p );
    }
  }

  if ( parts.isEmpty() )
  {
    mReadoutLabel->setText( QStringLiteral( "<b>—</b>" ) );
    if ( mLabelsOverlay )
      mLabelsOverlay->setLabels( {} );
    return;
  }

  QStringList partTexts;
  double total = 0.0;
  for ( int i = 0, n = parts.size(); i < n; ++i )
  {
    const Part &p = parts[i];
    const QString hdr = ( parts.size() > 1 ) ? tr( "Part %1\n" ).arg( i + 1 ) : QString();
    switch ( mMeasureMode )
    {
      case MeasureMode::MeasureLine:
        partTexts.append( hdr + lineReadout( p.geometry, total ) );
        break;
      case MeasureMode::MeasurePolygon:
        partTexts.append( hdr + polygonReadout( p.geometry, total ) );
        break;
      case MeasureMode::MeasureCircle:
      {
        const double radiusM = mDa.measureLine( p.circleCenter, QgsPointXY( p.circleCenter.x() + p.circleRadius, p.circleCenter.y() ) );
        const double area = M_PI * radiusM * radiusM;
        total += area;
        partTexts.append( hdr + tr( "Area: %1\nRadius: %2" ).arg( formatArea( area, areaUnit ), formatLength( radiusM, distUnit ) ) );
        break;
      }
    }
  }

  QString totalLine;
  switch ( mMeasureMode )
  {
    case MeasureMode::MeasureLine:
      totalLine = ( parts.size() > 1 ) ? tr( "Total: %1" ).arg( formatLength( total, distUnit ) ) : QString();
      break;
    case MeasureMode::MeasurePolygon:
    case MeasureMode::MeasureCircle:
      totalLine = ( parts.size() > 1 ) ? tr( "Total: %1" ).arg( formatArea( total, areaUnit ) ) : QString();
      break;
  }

  QString text = QStringLiteral( "<pre style='margin:0'>" ) + partTexts.join( QStringLiteral( "\n" ) ).toHtmlEscaped();
  if ( !totalLine.isEmpty() )
    text += QStringLiteral( "\n<b>" ) + totalLine.toHtmlEscaped() + QStringLiteral( "</b>" );
  text += QStringLiteral( "</pre>" );
  mReadoutLabel->setText( text );
  if ( mBottomBar )
    mBottomBar->adjustSize();

  updateCanvasLabels( parts );
}

void KadasMapToolMeasure::updateCanvasLabels( const QList<Part> &parts )
{
  if ( !mLabelsOverlay )
    return;

  const Qgis::DistanceUnit distUnit = mUnitComboBox ? static_cast<Qgis::DistanceUnit>( mUnitComboBox->currentData().toInt() ) : Qgis::DistanceUnit::Meters;
  const Qgis::AreaUnit areaUnit = QgsUnitTypes::distanceToAreaUnit( distUnit );
  const bool azimuthEnabled = mAzimuthCheckbox && mAzimuthCheckbox->isChecked();
  const Qgis::AngleUnit angleUnit = mAngleUnitComboBox ? static_cast<Qgis::AngleUnit>( mAngleUnitComboBox->currentData().toInt() ) : Qgis::AngleUnit::MilNATO;
  const bool geoNorth = !mNorthComboBox || mNorthComboBox->currentData().value<AzimuthNorth>() == AzimuthNorth::AzimuthGeoNorth;

  QList<KadasMeasureLabelsOverlay::Label> labels;
  for ( const Part &p : parts )
  {
    switch ( mMeasureMode )
    {
      case MeasureMode::MeasureLine:
      {
        for ( const QVector<QgsPointXY> &part : extractPolylines( p.geometry ) )
        {
          if ( part.size() < 2 )
            continue;
          double partLen = 0.0;
          for ( int i = 1, n = part.size(); i < n; ++i )
          {
            const QgsPointXY &p1 = part[i - 1];
            const QgsPointXY &p2 = part[i];
            const double segLen = mDa.measureLine( p1, p2 );
            partLen += segLen;
            QString text = formatLength( segLen, distUnit );
            if ( azimuthEnabled )
              text += QStringLiteral( "\n" ) + formatAngle( computeSegmentAzimuth( p1, p2, geoNorth ), angleUnit );
            labels.append( { QgsPointXY( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ), text, true } );
          }
          labels.append( { part.last(), tr( "Tot.: %1" ).arg( formatLength( partLen, distUnit ) ), false } );
        }
        break;
      }

      case MeasureMode::MeasurePolygon:
      {
        if ( p.geometry.isEmpty() )
          break;
        const double area = mDa.measureArea( p.geometry );
        const QgsGeometry centroid = p.geometry.centroid();
        if ( !centroid.isNull() )
          labels.append( { centroid.asPoint(), formatArea( area, areaUnit ), true } );
        break;
      }

      case MeasureMode::MeasureCircle:
      {
        const double radiusM = mDa.measureLine( p.circleCenter, QgsPointXY( p.circleCenter.x() + p.circleRadius, p.circleCenter.y() ) );
        const double area = M_PI * radiusM * radiusM;
        // Empty middle line keeps the center vertex marker from covering the text
        const QString text = formatArea( area, areaUnit ) + QStringLiteral( "\n\n" ) + tr( "Radius: %1" ).arg( formatLength( radiusM, distUnit ) );
        labels.append( { p.circleCenter, text, true } );
        break;
      }
    }
  }
  mLabelsOverlay->setLabels( labels );
}
