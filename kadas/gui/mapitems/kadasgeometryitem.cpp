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
#include <qgis/qgsmulticurve.h>
#include <qgis/qgsmultisurface.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgsunittypes.h>

#include "kadas/gui/mapitems/kadasgeometryitem.h"

static QFont measurementFont()
{
  QFont font;
  font.setPixelSize( 10 );
  font.setBold( true );
  return font;
}

static const int sLabelOffset = 16;

void KadasGeometryItem::registerMetaTypes()
{
  static bool registered = false;
  if ( !registered )
  {
    qRegisterMetaType<IconType>( "IconType" );
    registered = true;
  }
}

KadasGeometryItem::KadasGeometryItem()
  : KadasMapItem()
  , mPen( QPen( Qt::red, 4 ) )
  , mBrush( QColor( 255, 0, 0, 127 ) )
  , mIconSize( 10 )
  , mIconType( IconType::ICON_NONE )
  , mIconPen( Qt::red, 2 )
  , mIconBrush( Qt::white )
{
  registerMetaTypes();

  connect( this, &KadasGeometryItem::geometryChanged, this, &KadasGeometryItem::updateMeasurements );
}

KadasGeometryItem::~KadasGeometryItem()
{
  delete mGeometry;
}

void KadasGeometryItem::setCrs( const QgsCoordinateReferenceSystem &crs )
{
  KadasMapItem::setCrs( crs );
  mDa.setSourceCrs( crs, QgsProject::instance()->transformContext() );
  mDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", "NONE" ) );
}

void KadasGeometryItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  if ( !mGeometry )
  {
    return;
  }

  double dpiScale = outputDpiScale( context );

  QgsAbstractGeometry *paintGeom = mGeometry->clone();
  paintGeom->transform( context.coordinateTransform() );
  paintGeom->transform( context.mapToPixel().transform() );
  if ( QgsWkbTypes::geometryType( mGeometry->wkbType() ) != Qgis::GeometryType::Point )
  {
    // Workaround to avoid unintended tranparent outlines in PDF export:
    // render in 2 steps, brush first...
    context.painter()->setPen( Qt::NoPen );
    context.painter()->setBrush( mBrush );
    paintGeom->draw( *context.painter() );

    // ... and pen second
    context.painter()->setPen( QPen( mPen.brush(), mPen.widthF() * dpiScale, mPen.style() ) );
    context.painter()->setBrush( Qt::NoBrush );
    paintGeom->draw( *context.painter() );
  }

  if ( QgsWkbTypes::geometryType( mGeometry->wkbType() ) == Qgis::GeometryType::Polygon )
  {
    context.painter()->setBrush( mBrush );
  }
  else
  {
    context.painter()->setBrush( Qt::NoBrush );
  }
  context.painter()->setPen( QPen( mPen.brush(), mPen.widthF() * dpiScale, mPen.style() ) );

  // Draw vertices
  QgsVertexId vertexId;
  QgsPoint vertex;
  while ( paintGeom->nextVertex( vertexId, vertex ) )
  {
    drawVertex( context.painter(), dpiScale, vertex.x(), vertex.y() );
  }

  // Draw measurement labels
  int red = QgsSettings().value( "/Qgis/default_measure_color_red", 255 ).toInt();
  int green = QgsSettings().value( "/Qgis/default_measure_color_green", 0 ).toInt();
  int blue = QgsSettings().value( "/Qgis/default_measure_color_blue", 0 ).toInt();
  QColor rectColor = QColor( 255, 255, 255, 192 );

  context.painter()->setPen( QColor( red, green, blue ) );
  context.painter()->setFont( measurementFont() );
  QFontMetrics metrics = context.painter()->fontMetrics();

  for ( const MeasurementLabel &label : mMeasurementLabels )
  {
    QPointF pos = context.mapToPixel().transform( context.coordinateTransform().transform( label.pos ) ).toQPointF();
    int width = label.width + 6;
    int height = label.height + 6;
    QRectF labelRect( pos.x() - 0.5 * width, pos.y() + ( label.center ? 0 : sLabelOffset ) - 0.5 * height, width, height );
    context.painter()->fillRect( labelRect, rectColor );
    context.painter()->drawText( labelRect, Qt::AlignCenter | Qt::AlignVCenter, label.string );
  }
  delete paintGeom;
}

QString KadasGeometryItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  if ( !mGeometry )
  {
    return QString();
  }

  auto color2hex = []( const QColor &c ) { return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) ); };

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << QString( "<name>%1</name>\n" ).arg( exportName() );
  outStream << "<Style>\n";
  outStream << QString( "<LineStyle><width>%1</width><color>%2</color></LineStyle>\n<PolyStyle><fill>%3</fill><color>%4</color></PolyStyle>\n" )
                 .arg( outline().width() )
                 .arg( color2hex( outline().color() ) )
                 .arg( fill().style() != Qt::NoBrush ? 1 : 0 )
                 .arg( color2hex( fill().color() ) );
  outStream << "</Style>\n";
  outStream << "<ExtendedData>\n";
  outStream << "<SchemaData schemaUrl=\"#KadasGeometryItem\">\n";
  outStream << QString( "<SimpleData name=\"icon_type\">%1</SimpleData>\n" ).arg( static_cast<int>( mIconType ) );
  outStream << QString( "<SimpleData name=\"outline_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodePenStyle( mPen.style() ) );
  outStream << QString( "<SimpleData name=\"fill_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodeBrushStyle( mBrush.style() ) );
  outStream << "</SchemaData>\n";
  outStream << "</ExtendedData>\n";
  QgsAbstractGeometry *geom = mGeometry->segmentize();
  geom->transform( QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ) );
  outStream << geom->asKml( 6 ) << "\n";
  delete geom;
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}

void KadasGeometryItem::drawVertex( QPainter *p, double dpiScale, double x, double y ) const
{
  qreal iconSize = mIconSize * mSymbolScale * dpiScale;
  qreal s = ( iconSize - 1 ) / 2;
  p->save();
  p->setPen( QPen( mIconPen.brush(), mIconPen.widthF() * dpiScale, mIconPen.style() ) );
  p->setBrush( mIconBrush );

  switch ( mIconType )
  {
    case IconType::ICON_NONE:
      break;

    case IconType::ICON_CROSS:
      p->drawLine( QLineF( x - s, y, x + s, y ) );
      p->drawLine( QLineF( x, y - s, x, y + s ) );
      break;

    case IconType::ICON_X:
      p->drawLine( QLineF( x - s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x + s, y - s ) );
      break;

    case IconType::ICON_BOX:
      p->drawLine( QLineF( x - s, y - s, x + s, y - s ) );
      p->drawLine( QLineF( x + s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x - s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x - s, y - s ) );
      break;

    case IconType::ICON_FULL_BOX:
      p->drawRect( x - s, y - s, iconSize, iconSize );
      break;

    case IconType::ICON_CIRCLE:
      p->drawEllipse( x - s, y - s, iconSize, iconSize );
      break;

    case IconType::ICON_TRIANGLE:
      p->drawLine( QLineF( x - s, y + s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x, y - s ) );
      p->drawLine( QLineF( x, y - s, x - s, y + s ) );
      break;

    case IconType::ICON_FULL_TRIANGLE:
      p->drawPolygon( QPolygonF() << QPointF( x - s, y + s ) << QPointF( x + s, y + s ) << QPointF( x, y - s ) );
      break;
  }
  p->restore();
}

KadasMapItem::Margin KadasGeometryItem::margin() const
{
  int maxMeasureLabelWidth = 0;
  int maxMeasureLabelHeight = 0;
  if ( mMeasureGeometry )
  {
    for ( const MeasurementLabel &label : mMeasurementLabels )
    {
      maxMeasureLabelWidth = std::max( maxMeasureLabelWidth, label.width / 2 + 1 );
      maxMeasureLabelHeight = std::max( maxMeasureLabelHeight, label.height / 2 + 1 ) + sLabelOffset;
    }
  }
  int maxPainterMargin = std::ceil( std::max( mIconType != IconType::ICON_NONE ? mIconSize * mSymbolScale : 0., mPen.widthF() ) / 2. + 1 );
  int maxW = std::max( maxMeasureLabelWidth, maxPainterMargin );
  int maxH = std::max( maxMeasureLabelHeight, maxPainterMargin );
  return Margin { maxW, maxH, maxW, maxH };
}

void KadasGeometryItem::updateMeasurements()
{
  mMeasurementLabels.clear();
  mTotalMeasurement.clear();
  if ( mMeasureGeometry )
  {
    measureGeometry();
  }
  update();
}

void KadasGeometryItem::setInternalGeometry( QgsAbstractGeometry *geom )
{
  delete mGeometry;
  mGeometry = geom;
  emit geometryChanged();
}

bool KadasGeometryItem::intersects( const QgsRectangle &rect, const QgsMapSettings &settings, bool contains ) const
{
  if ( !mGeometry )
  {
    return false;
  }
  QgsRectangle r = QgsCoordinateTransform( settings.destinationCrs(), crs(), QgsProject::instance() ).transform( rect );

  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence() << QgsPoint( r.xMinimum(), r.yMinimum() ) << QgsPoint( r.xMaximum(), r.yMinimum() ) << QgsPoint( r.xMaximum(), r.yMaximum() ) << QgsPoint( r.xMinimum(), r.yMaximum() ) << QgsPoint( r.xMinimum(), r.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &filterRect );
  bool intersects = false;
  if ( ( mBrush.color().alpha() == 0 || mBrush.style() == Qt::NoBrush ) && dynamic_cast<QgsMultiSurface *>( mGeometry ) )
  {
    QgsMultiSurface *multiSurface = static_cast<QgsMultiSurface *>( mGeometry );
    QgsMultiCurve multiCurve;
    for ( int i = 0, n = multiSurface->numGeometries(); i < n; ++i )
    {
      QgsCurvePolygon *surface = dynamic_cast<QgsCurvePolygon *>( multiSurface->geometryN( i ) );
      multiCurve.addGeometry( surface->exteriorRing()->clone() );
    }
    intersects = contains ? geomEngine->contains( mGeometry ) : geomEngine->intersects( &multiCurve );
  }
  else
  {
    intersects = contains ? geomEngine->contains( mGeometry ) : geomEngine->intersects( mGeometry );
  }
  delete geomEngine;
  return intersects;
}

QPair<KadasMapPos, double> KadasGeometryItem::closestPoint( const KadasMapPos &pos, const QgsMapSettings &settings ) const
{
  double minDistSq = std::numeric_limits<double>::max();
  KadasMapPos minPos;
  QgsVertexId vidx;
  QgsPoint p;
  QgsPointXY testPosScreen = settings.mapToPixel().transform( pos );
  while ( mGeometry->nextVertex( vidx, p ) )
  {
    KadasMapPos mapPos = toMapPos( KadasItemPos::fromPoint( p ), settings );
    QgsPointXY itemPosScreen = settings.mapToPixel().transform( mapPos );
    double distSq = itemPosScreen.sqrDist( testPosScreen );
    if ( distSq < minDistSq )
    {
      minDistSq = distSq;
      minPos = mapPos;
    }
  }
  return qMakePair( minPos, std::sqrt( minDistSq ) );
}

void KadasGeometryItem::clear()
{
  delete mState;
  mState = createEmptyState();
  recomputeDerived();
}

void KadasGeometryItem::setState( const State *state )
{
  mState->assign( state );
  recomputeDerived();
}

void KadasGeometryItem::setOutline( const QPen &pen )
{
  mPen = pen;
  update();
  emit propertyChanged();
}

void KadasGeometryItem::setFill( const QBrush &brush )
{
  mBrush = brush;
  update();
  emit propertyChanged();
}

void KadasGeometryItem::setIconSize( int iconSize )
{
  mIconSize = iconSize;
  update();
  emit propertyChanged();
}

void KadasGeometryItem::setIconType( IconType iconType )
{
  mIconType = iconType;
  update();
  emit propertyChanged();
}

void KadasGeometryItem::setIconOutline( const QPen &iconPen )
{
  mIconPen = iconPen;
  update();
  emit propertyChanged();
}

void KadasGeometryItem::setIconFill( const QBrush &iconBrush )
{
  mIconBrush = iconBrush;
  update();
  emit propertyChanged();
}

QgsRectangle KadasGeometryItem::boundingBox() const
{
  QgsRectangle bbox = mGeometry ? mGeometry->boundingBox() : QgsRectangle();
  return bbox;
}

QList<KadasMapItem::Node> KadasGeometryItem::nodes( const QgsMapSettings &settings ) const
{
  QList<Node> points;
  QgsVertexId vidx;
  QgsPoint p;
  while ( mGeometry->nextVertex( vidx, p ) )
  {
    points.append( { toMapPos( KadasItemPos( p.x(), p.y() ), settings ) } );
  }
  return points;
}

void KadasGeometryItem::setMeasurementsEnabled( bool enabled, Qgis::DistanceUnit baseUnit )
{
  mMeasureGeometry = enabled;
  mBaseUnit = baseUnit;
  emit geometryChanged(); // Trigger re-measurement
}

Qgis::DistanceUnit KadasGeometryItem::distanceBaseUnit() const
{
  return mBaseUnit;
}

Qgis::AreaUnit KadasGeometryItem::areaBaseUnit() const
{
  return QgsUnitTypes::distanceToAreaUnit( mBaseUnit );
}

QString KadasGeometryItem::formatLength( double value, Qgis::DistanceUnit unit ) const
{
  int decimals = QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt();
  value = mDa.convertLengthMeasurement( value, unit );
  return QgsUnitTypes::formatDistance( value, decimals, unit );
}

QString KadasGeometryItem::formatArea( double value, Qgis::AreaUnit unit ) const
{
  int decimals = QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt();
  value = mDa.convertAreaMeasurement( value, unit );
  if ( unit == Qgis::AreaUnit::SquareMeters )
  {
    if ( value >= 1000000 )
    {
      return QString( "%1 km²" ).arg( value / 1000000., 0, 'f', decimals );
    }
    else
    {
      return QString( "%1 m²" ).arg( value, 0, 'f', decimals );
    }
  }
  else
  {
    return QgsUnitTypes::formatArea( value, decimals, unit );
  }
}

QString KadasGeometryItem::formatAngle( double value, Qgis::AngleUnit unit ) const
{
  int decimals = QgsSettings().value( "/kadas/measure_decimals", "2" ).toInt();
  value *= QgsUnitTypes::fromUnitToUnitFactor( Qgis::AngleUnit::Radians, unit );
  return QgsUnitTypes::formatAngle( value, decimals, unit );
}

void KadasGeometryItem::addMeasurements( const QStringList &measurements, const KadasItemPos &mapPos, bool center )
{
  static QFontMetrics metrics( measurementFont() );
  int width = 0;
  for ( const QString &line : measurements )
  {
    width = std::max( width, metrics.horizontalAdvance( line ) );
  }
  if ( !measurements.isEmpty() )
  {
    mMeasurementLabels.append( MeasurementLabel { measurements.join( "\n" ), mapPos, width, metrics.height() * measurements.size(), center } );
  }
}

static KadasItemPos projectPointOnSegment( const KadasItemPos &p, const KadasItemPos &s1, const KadasItemPos &s2 )
{
  double nx = s2.y() - s1.y();
  double ny = -( s2.x() - s1.x() );
  double t = ( p.x() * ny - p.y() * nx - s1.x() * ny + s1.y() * nx ) / ( ( s2.x() - s1.x() ) * ny - ( s2.y() - s1.y() ) * nx );
  return t < 0. ? s1 : t > 1. ? s2
                              : KadasItemPos( s1.x() + ( s2.x() - s1.x() ) * t, s1.y() + ( s2.y() - s1.y() ) * t );
}

QgsVertexId KadasGeometryItem::insertionPoint( const QList<QList<KadasItemPos>> &points, const KadasItemPos &testPos ) const
{
  double minDist = std::numeric_limits<double>::max();
  QgsVertexId minVtx;
  for ( int j = 0, m = points.size(); j < m; ++j )
  {
    const QList<KadasItemPos> &part = points[j];
    int n = part.size();
    for ( int i = 0; i < n - 1; ++i )
    {
      const KadasItemPos &p1 = part[i];
      const KadasItemPos &p2 = part[i + 1];
      KadasItemPos inter = projectPointOnSegment( testPos, p1, p2 );
      double dist = inter.sqrDist( testPos );
      if ( dist < minDist )
      {
        minDist = dist;
        minVtx = QgsVertexId( j, 0, i + 1 );
      }
    }
  }
  return minVtx;
}
