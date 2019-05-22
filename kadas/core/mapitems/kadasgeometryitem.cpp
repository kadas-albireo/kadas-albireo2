/***************************************************************************
    kadasgeometryitem.cpp
    ---------------------
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

#include <QPainter>

#include <qgis/qgsabstractgeometry.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgscompoundcurve.h>
#include <qgis/qgscurve.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsgeometrycollection.h>
#include <qgis/qgsgeometryengine.h>
#include <qgis/qgsgeometryutils.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>

#include <kadas/core/mapitems/kadasgeometryitem.h>

static QFont measurementFont() {
  QFont font;
  font.setPixelSize(10);
  font.setBold(true);
  return font;
}

KadasGeometryItem::KadasGeometryItem(const QgsCoordinateReferenceSystem &crs, QObject *parent)
    : KadasMapItem(crs, parent)
    , mPen( Qt::red )
    , mBrush( Qt::red )
    , mIconSize( 5 )
    , mIconType( ICON_NONE )
    , mIconPen( Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin )
    , mIconBrush( Qt::transparent )
{
  mDa.setSourceCrs(crs, QgsProject::instance()->transformContext());
  mDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
  connect(this, &KadasGeometryItem::geometryChanged, this, &KadasGeometryItem::updateMeasurements);
}

KadasGeometryItem::~KadasGeometryItem()
{
  delete mGeometry;
}

void KadasGeometryItem::render( QgsRenderContext &context ) const
{
  if ( !mGeometry )
  {
    return;
  }

  context.painter()->save();

  if ( QgsWkbTypes::geometryType(mGeometry->wkbType()) == QgsWkbTypes::PolygonGeometry )
  {
    context.painter()->setBrush( mBrush );
  }
  else
  {
    context.painter()->setBrush( Qt::NoBrush );
  }
  context.painter()->setPen( mPen );

  QgsAbstractGeometry* paintGeom = mGeometry->clone();
  paintGeom->transform( QgsCoordinateTransform(mCrs, context.coordinateTransform().destinationCrs(), context.transformContext()) );
  paintGeom->transform( context.mapToPixel().transform() );
  paintGeom->draw( *context.painter() );

  // Draw vertices
  QgsVertexId vertexId;
  QgsPoint vertex;
  while ( paintGeom->nextVertex( vertexId, vertex ) )
  {
    drawVertex( context.painter(), vertex.x(), vertex.y() );
  }

  // Draw measurement labels
  int red = QSettings().value( "/Qgis/default_measure_color_red", 255 ).toInt();
  int green = QSettings().value( "/Qgis/default_measure_color_green", 0 ).toInt();
  int blue = QSettings().value( "/Qgis/default_measure_color_blue", 0 ).toInt();
  QColor rectColor = QColor(255, 255, 255, 192);

  context.painter()->setPen(QColor(red, green, blue));
  context.painter()->setFont(measurementFont());
  QFontMetrics metrics = context.painter()->fontMetrics();

  for(const MeasurementLabel& label : mMeasurementLabels) {
    QPointF pos = context.mapToPixel().transform(label.mapPos).toQPointF();
    int width = label.width + 6;
    int heigth = label.height + 6;
    QRectF labelRect(pos.x() - 0.5 * width, pos.y() + (label.center ? 0 : 16) - 0.5 * heigth, width, heigth);
    context.painter()->fillRect(labelRect, rectColor);
    context.painter()->drawText(labelRect, Qt::AlignCenter|Qt::AlignVCenter, label.string);
  }
  delete paintGeom;
  context.painter()->restore();
}

void KadasGeometryItem::drawVertex( QPainter* p, double x, double y ) const
{
  qreal s = ( mIconSize - 1 ) / 2;
  p->save();
  p->setPen( mIconPen );
  p->setBrush( mIconBrush );

  switch ( mIconType )
  {
    case ICON_NONE:
      break;

    case ICON_CROSS:
      p->drawLine( QLineF( x - s, y, x + s, y ) );
      p->drawLine( QLineF( x, y - s, x, y + s ) );
      break;

    case ICON_X:
      p->drawLine( QLineF( x - s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x + s, y - s ) );
      break;

    case ICON_BOX:
      p->drawLine( QLineF( x - s, y - s, x + s, y - s ) );
      p->drawLine( QLineF( x + s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x - s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x - s, y - s ) );
      break;

    case ICON_FULL_BOX:
      p->drawRect( x - s, y - s, mIconSize, mIconSize );
      break;

    case ICON_CIRCLE:
      p->drawEllipse( x - s, y - s, mIconSize, mIconSize );
      break;

    case ICON_TRIANGLE:
      p->drawLine( QLineF( x - s, y + s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x, y - s ) );
      p->drawLine( QLineF( x, y - s, x - s, y + s ) );
      break;

    case ICON_FULL_TRIANGLE:
      p->drawPolygon( QPolygonF() <<
                      QPointF( x - s, y + s ) <<
                      QPointF( x + s, y + s ) <<
                      QPointF( x, y - s ) );
      break;
  }
  p->restore();
}

int KadasGeometryItem::margin() const {
  int measurementMargin = 0;
  for(const MeasurementLabel label : mMeasurementLabels) {
    measurementMargin = qMax(measurementMargin, label.width/2 + 4);
  }
  return qMax(measurementMargin, qMax(mIconSize, mPen.width()));
}

void KadasGeometryItem::updateMeasurements()
{
  mMeasurementLabels.clear();
  mTotalMeasurement.clear();
  if ( mMeasureGeometry )
  {
    measureGeometry();
  }
  emit changed();
}

void KadasGeometryItem::setGeometry(QgsAbstractGeometry* geom )
{
  delete mGeometry;
  mGeometry = geom;
  emit geometryChanged();
}

bool KadasGeometryItem::intersects(const QgsRectangle &rect) const
{
  if ( !mGeometry )
  {
    return false;
  }

  QgsPolygon filterRect;
  QgsLineString* exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine* geomEngine = QgsGeometry::createGeometryEngine( mGeometry );
  bool intersects = geomEngine->intersects( &filterRect );
  delete geomEngine;
  return intersects;
}

void KadasGeometryItem::setFillColor( const QColor& c )
{
  mBrush.setColor( c );
  emit changed();
}

QColor KadasGeometryItem::fillColor() const
{
  return mBrush.color();
}

void KadasGeometryItem::setOutlineColor( const QColor& c )
{
  mPen.setColor( c );
  emit changed();
}

QColor KadasGeometryItem::outlineColor() const
{
  return mPen.color();
}

void KadasGeometryItem::setOutlineWidth( int width )
{
  mPen.setWidth( width );
  emit changed();
}

int KadasGeometryItem::outlineWidth() const
{
  return mPen.width();
}

void KadasGeometryItem::setLineStyle( Qt::PenStyle penStyle )
{
  mPen.setStyle( penStyle );
  emit changed();
}

Qt::PenStyle KadasGeometryItem::lineStyle() const
{
  return mPen.style();
}

void KadasGeometryItem::setBrushStyle( Qt::BrushStyle brushStyle )
{
  mBrush.setStyle( brushStyle );
  emit changed();
}

Qt::BrushStyle KadasGeometryItem::brushStyle() const
{
  return mBrush.style();
}

void KadasGeometryItem::setIconSize( int iconSize )
{
  mIconSize = iconSize;
  emit changed();
}

void KadasGeometryItem::setIconFillColor( const QColor& c )
{
  mIconBrush.setColor( c );
  emit changed();
}

void KadasGeometryItem::setIconOutlineColor( const QColor& c )
{
  mIconPen.setColor( c );
  emit changed();
}

void KadasGeometryItem::setIconOutlineWidth( int width )
{
  mIconPen.setWidth( width );
  emit changed();
}

void KadasGeometryItem::setIconLineStyle( Qt::PenStyle penStyle )
{
  mIconPen.setStyle( penStyle );
  emit changed();
}

void KadasGeometryItem::setIconBrushStyle( Qt::BrushStyle brushStyle )
{
  mIconBrush.setStyle( brushStyle );
  emit changed();
}

QgsRectangle KadasGeometryItem::boundingBox() const
{
  return mGeometry ? mGeometry->boundingBox() : QgsRectangle();
}

QList<QgsPointXY> KadasGeometryItem::nodes() const
{
  QList<QgsPointXY> points;
  QgsVertexId vidx;
  QgsPoint p;
  while(mGeometry->nextVertex(vidx, p)) {
    points.append(p);
  }
  return points;
}

void KadasGeometryItem::setMeasurementsEnabled(bool enabled, QgsUnitTypes::DistanceUnit baseUnit)
{
  mMeasureGeometry = enabled;
  mBaseUnit = baseUnit;
  emit geometryChanged(); // Trigger re-measurement
}

QgsUnitTypes::DistanceUnit KadasGeometryItem::distanceBaseUnit() const
{
  return mBaseUnit;
}

QgsUnitTypes::AreaUnit KadasGeometryItem::areaBaseUnit() const
{
  return QgsUnitTypes::distanceToAreaUnit(mBaseUnit);
}

QString KadasGeometryItem::formatLength( double value, QgsUnitTypes::DistanceUnit unit ) const
{
  int decimals = QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt();
  value = mDa.convertLengthMeasurement(value, unit);
  return QgsUnitTypes::formatDistance(value, decimals, unit);
}

QString KadasGeometryItem::formatArea( double value, QgsUnitTypes::AreaUnit unit ) const
{
  int decimals = QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt();
  value = mDa.convertAreaMeasurement(value, unit);
  return QgsUnitTypes::formatArea(value, decimals, unit);
}

QString KadasGeometryItem::formatAngle( double value, QgsUnitTypes::AngleUnit unit ) const
{
  int decimals = QSettings().value( "/Qgis/measure/decimalplaces", "2" ).toInt();
  value *= QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::AngleRadians, unit);
  return QgsUnitTypes::formatAngle(value, decimals, unit);
}

void KadasGeometryItem::addMeasurements(const QStringList& measurements, const QgsPointXY &mapPos, bool center )
{
  static QFontMetrics metrics(measurementFont());
  int width = 0;
  for(const QString& line : measurements) {
    width = qMax(width, metrics.width(line));
  }
  if(!measurements.isEmpty()) {
    mMeasurementLabels.append(MeasurementLabel{
      measurements.join("\n"),
      mapPos,
      width,
      metrics.height() * measurements.size(),
      center
    });
  }
}
