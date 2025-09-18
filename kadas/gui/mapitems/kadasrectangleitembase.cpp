/***************************************************************************
    kadasrectangleitembase.cpp
    --------------------
    copyright            : (C) 2024 Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDesktopServices>
#include <QGenericMatrix>
#include <QImageReader>
#include <QJsonArray>
#include <QMenu>

#include <array>
#include <exiv2/exiv2.hpp>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>

#include <quazip/quazipfile.h>

#include "kadas/core/kadascoordinateutils.h"
#include "kadas/analysis/kadaslineofsight.h"
#include "kadas/gui/mapitems/kadasrectangleitembase.h"


QJsonObject KadasRectangleItemBase::State::serialize() const
{
  QJsonArray pos;
  pos.append( mPos.x() );
  pos.append( mPos.y() );
  QJsonArray size;
  size.append( mSize.width() );
  size.append( mSize.height() );
  QJsonArray footprint;
  for ( const KadasItemPos &footprintPos : mFootprint )
  {
    QJsonArray fp;
    fp.append( footprintPos.x() );
    fp.append( footprintPos.y() );
    footprint.append( fp );
  }
  QJsonArray rectCenterPoint;
  rectCenterPoint.append( mRectangleCenterPoint.x() );
  rectCenterPoint.append( mRectangleCenterPoint.y() );

  QJsonObject json;
  json["status"] = static_cast<int>( drawStatus );
  json["pos"] = pos;
  json["angle"] = mAngle;
  json["offsetX"] = mOffsetX;
  json["offsetY"] = mOffsetY;
  json["size"] = size;
  json["footprint"] = footprint;
  json["rectangle-center"] = rectCenterPoint;
  json["frame"] = mFrame;
  return json;
}

bool KadasRectangleItemBase::State::deserialize( const QJsonObject &json )
{
  // frame is enable by default, but for openng of older projects with texts (that had no frame), we disable it
  mFrame = json.contains( "frame" ) ? json["frame"].toBool() : false;

  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  QJsonArray p = json["pos"].toArray();
  mPos = KadasItemPos( p.at( 0 ).toDouble(), p.at( 1 ).toDouble() );
  mAngle = json["angle"].toDouble();
  mOffsetX = json["offsetX"].toDouble();
  mOffsetY = json["offsetY"].toDouble();
  QJsonArray s = json["size"].toArray();
  mSize = QSize( s.at( 0 ).toDouble(), s.at( 1 ).toDouble() );
  QJsonArray f = json["footprint"].toArray();
  mFootprint.clear();
  for ( const QJsonValue &v : f )
  {
    QJsonArray fp = v.toArray();
    mFootprint.append( KadasItemPos( fp.at( 0 ).toDouble(), fp.at( 1 ).toDouble() ) );
  }
  QJsonArray t = json["rectangle-center"].toArray();
  mRectangleCenterPoint = KadasItemPos( t.at( 0 ).toDouble(), t.at( 1 ).toDouble() );
  return true;
}


KadasRectangleItemBase::KadasRectangleItemBase()
  : KadasMapItem()
{
  mIsPointSymbol = true;
  clear();
}

KadasRectangleItemBase::~KadasRectangleItemBase()
{
}


void KadasRectangleItemBase::setFrameVisible( bool frame )
{
  state()->mFrame = frame;
  if ( !frame )
  {
    state()->mOffsetX = 0;
    state()->mOffsetY = 0;
  }
  update();
  emit propertyChanged();
}

void KadasRectangleItemBase::setPositionLocked( bool locked )
{
  mPosLocked = locked;
  update();
  emit propertyChanged();
}

void KadasRectangleItemBase::setPosition( const KadasItemPos &pos )
{
  if ( !mPosLocked )
  {
    state()->mPos = pos;
    state()->drawStatus = State::DrawStatus::Finished;
    update();
  }
}

void KadasRectangleItemBase::setState( const KadasMapItem::State *state )
{
  KadasMapItem::setState( state );
}

QgsRectangle KadasRectangleItemBase::boundingBox() const
{
  double xmin = constState()->mPos.x(), xmax = constState()->mPos.x();
  double ymin = constState()->mPos.y(), ymax = constState()->mPos.y();
  for ( const KadasItemPos &p : constState()->mFootprint )
  {
    xmin = std::min( xmin, p.x() );
    xmax = std::max( xmax, p.x() );
    ymin = std::min( ymin, p.y() );
    ymax = std::max( ymax, p.y() );
  }
  return QgsRectangle( xmin, ymin, xmax, ymax );
}

KadasMapItem::Margin KadasRectangleItemBase::margin() const
{
  double framePadding = constState()->frame() ? sFramePadding : 0;
  return Margin {
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->mSize.width() - constState()->mOffsetX + framePadding ) * mSymbolScale ) ),
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->mSize.height() + constState()->mOffsetY + framePadding ) * mSymbolScale ) ),
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->mSize.width() + constState()->mOffsetX + framePadding ) * mSymbolScale ) ),
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->mSize.height() - constState()->mOffsetY + framePadding ) * mSymbolScale ) )
  };
}

QList<KadasMapPos> KadasRectangleItemBase::cornerPoints( const QgsMapSettings &settings ) const
{
  KadasMapPos mapPos = toMapPos( constState()->mPos, settings );
  double halfW = 0.5 * constState()->mSize.width();
  double halfH = 0.5 * constState()->mSize.height();
  double scale = settings.mapUnitsPerPixel() * mSymbolScale;

  KadasMapPos p1( mapPos.x() + ( constState()->mOffsetX - halfW ) * scale, mapPos.y() + ( constState()->mOffsetY - halfH ) * scale );
  KadasMapPos p2( mapPos.x() + ( constState()->mOffsetX + halfW ) * scale, mapPos.y() + ( constState()->mOffsetY - halfH ) * scale );
  KadasMapPos p3( mapPos.x() + ( constState()->mOffsetX + halfW ) * scale, mapPos.y() + ( constState()->mOffsetY + halfH ) * scale );
  KadasMapPos p4( mapPos.x() + ( constState()->mOffsetX - halfW ) * scale, mapPos.y() + ( constState()->mOffsetY + halfH ) * scale );

  return QList<KadasMapPos>() << p1 << p2 << p3 << p4;
}

QList<KadasMapItem::Node> KadasRectangleItemBase::nodes( const QgsMapSettings &settings ) const
{
  QList<KadasMapPos> points = cornerPoints( settings );
  QList<Node> nodes;
  nodes.append( { points[0] } );
  nodes.append( { points[1] } );
  nodes.append( { points[2] } );
  nodes.append( { points[3] } );
  nodes.append( { toMapPos( constState()->mPos, settings ), anchorNodeRenderer } );
  return nodes;
}

bool KadasRectangleItemBase::intersects( const QgsRectangle &rect, const QgsMapSettings &settings, bool contains ) const
{
  if ( constState()->mSize.isEmpty() )
  {
    return false;
  }

  QList<KadasMapPos> points = cornerPoints( settings );
  QgsPolygon imageRect;
  imageRect.setExteriorRing( new QgsLineString( QgsPointSequence() << QgsPoint( points[0] ) << QgsPoint( points[1] ) << QgsPoint( points[2] ) << QgsPoint( points[3] ) << QgsPoint( points[0] ) ) );

  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence() << QgsPoint( rect.xMinimum(), rect.yMinimum() ) << QgsPoint( rect.xMaximum(), rect.yMinimum() ) << QgsPoint( rect.xMaximum(), rect.yMaximum() ) << QgsPoint( rect.xMinimum(), rect.yMaximum() ) << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &filterRect );
  bool intersects = contains ? geomEngine->contains( &imageRect ) : geomEngine->intersects( &imageRect );
  delete geomEngine;
  return intersects;
}

void KadasRectangleItemBase::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  QgsPoint pos( constState()->mPos );
  pos.transform( context.coordinateTransform() );
  pos.transform( context.mapToPixel().transform() );

  // Draw footprint TODO move to picture rendering
  QPolygonF poly;
  if ( constState()->mFootprint.size() == 4 )
  {
    QgsPoint target( constState()->mRectangleCenterPoint );
    target.transform( context.coordinateTransform() );
    target.transform( context.mapToPixel().transform() );

    for ( const KadasItemPos &fp : constState()->mFootprint )
    {
      QgsPointXY p = fp;
      p = context.coordinateTransform().transform( p );
      poly.append( context.mapToPixel().transform( p ).toQPointF() );
    }
    poly.append( poly.front() );
    QPainterPath path;
    path.addPolygon( poly );

    QLinearGradient strokeGradient( pos.toQPointF(), target.toQPointF() );
    strokeGradient.setColorAt( 0, QColor( 255, 0, 0 ) );
    strokeGradient.setColorAt( 0.75, QColor( 255, 0, 0 ) );
    strokeGradient.setColorAt( 1., QColor( 255, 0, 0, 12 ) );
    context.painter()->setPen( QPen( strokeGradient, 2, Qt::SolidLine ) );

    QLinearGradient fillGradient( pos.toQPointF(), target.toQPointF() );
    fillGradient.setColorAt( 0, QColor( 255, 0, 0, 127 ) );
    fillGradient.setColorAt( 0.75, QColor( 255, 0, 0, 127 ) );
    fillGradient.setColorAt( 1., QColor( 255, 0, 0, 12 ) );
    context.painter()->setBrush( fillGradient );
    context.painter()->drawPath( path );
    context.painter()->setPen( QPen( Qt::blue, 2, Qt::SolidLine ) );
    context.painter()->drawLine( pos.x(), pos.y(), poly.at( 0 ).x(), poly.at( 0 ).y() );
    context.painter()->drawLine( pos.x(), pos.y(), poly.at( 1 ).x(), poly.at( 1 ).y() );
  }

  context.painter()->translate( pos.x(), pos.y() );
  context.painter()->scale( mSymbolScale, mSymbolScale );
  double dpiScale = outputDpiScale( context );
  double arrowWidth = sArrowWidth * dpiScale;

  double w = constState()->mSize.width() * dpiScale;
  double h = constState()->mSize.height() * dpiScale;
  double offsetX = constState()->mOffsetX * dpiScale;
  double offsetY = constState()->mOffsetY * dpiScale;


  // Draw frame
  if ( constState()->frame() )
  {
    context.painter()->setPen( QPen( Qt::black, 1 ) );
    context.painter()->setBrush( Qt::white );

    double framew = w + 2 * sFramePadding * dpiScale;
    double frameh = h + 2 * sFramePadding * dpiScale;
    QRectF frameRectangle( offsetX - 0.5 * framew, -offsetY - 0.5 * frameh, framew, frameh );

    QPolygonF poly;
    poly.append( frameRectangle.bottomLeft() );
    poly.append( frameRectangle.topLeft() );
    poly.append( frameRectangle.topRight() );
    poly.append( frameRectangle.bottomRight() );
    poly.append( frameRectangle.bottomLeft() );

    // Draw frame triangle
    if ( qAbs( offsetX ) > qAbs( 0.5 * framew ) || qAbs( offsetY ) > qAbs( 0.5 * frameh ) )
    {
      static const int QUADRANT_LEFT = 0;
      static const int QUADRANT_TOP = 1;
      static const int QUADRANT_RIGHT = 2;
      static const int QUADRANT_BOTTOM = 3;

      // Determine nearest corner
      QPointF nearestCorner = frameRectangle.bottomRight();
      if ( offsetX > 0 && offsetY > 0 )
        nearestCorner = frameRectangle.bottomLeft();

      else if ( offsetX > 0 && offsetY <= 0 )
        nearestCorner = frameRectangle.topLeft();

      else if ( offsetX <= 0 && offsetY <= 0 )
        nearestCorner = frameRectangle.topRight();

      // Determine triangle quadrant
      int quadrant = qRound( std::atan2( nearestCorner.y(), nearestCorner.x() ) / M_PI * 180 / 90 );

      if ( quadrant < 0 )
        quadrant += 4;

      // Well defined cases (not by the corner)
      if ( offsetX > 0.5 * framew && qAbs( offsetY ) < 0.5 * frameh )
        quadrant = QUADRANT_LEFT;
      else if ( offsetX < -0.5 * framew && qAbs( offsetY ) < 0.5 * frameh )
        quadrant = QUADRANT_RIGHT;
      else if ( offsetY > 0.5 * frameh && qAbs( offsetX ) < 0.5 * framew )
        quadrant = QUADRANT_BOTTOM;
      else if ( offsetY < -0.5 * frameh && qAbs( offsetX ) < 0.5 * framew )
        quadrant = QUADRANT_TOP;

      QgsPointXY framePos;
      QgsVector baseDir;
      if ( quadrant == QUADRANT_LEFT || quadrant == QUADRANT_RIGHT ) // Triangle to the left (quadrant = 0) or right (quadrant = 2)
      {
        baseDir = QgsVector( 0, quadrant == 0 ? -1 : 1 );
        framePos.setX( quadrant == 0 ? offsetX - 0.5 * framew : offsetX + 0.5 * framew );
        framePos.setY( std::min( std::max( -offsetY - 0.5 * frameh + arrowWidth, 0. ), -offsetY + 0.5 * frameh - arrowWidth ) );
      }
      else // Triangle above (quadrant = 1) or below (quadrant = 3)
      {
        framePos.setX( std::min( std::max( offsetX - 0.5 * framew + arrowWidth, 0. ), offsetX + 0.5 * framew - arrowWidth ) );
        framePos.setY( quadrant == 1 ? -offsetY - 0.5 * frameh : -offsetY + 0.5 * frameh );
        baseDir = QgsVector( quadrant == 1 ? 1 : -1, 0 );
      }
      int inspos = quadrant + 1;
      poly.insert( inspos++, QPointF( framePos.x() - arrowWidth * baseDir.x(), framePos.y() - arrowWidth * baseDir.y() ) );
      poly.insert( inspos++, QPointF( 0, 0 ) );
      poly.insert( inspos++, QPointF( framePos.x() + arrowWidth * baseDir.x(), framePos.y() + arrowWidth * baseDir.y() ) );
    }
    QPainterPath path;
    path.addPolygon( poly );
    context.painter()->drawPath( path );
  }

  const QPointF center = QPointF( offsetX, -offsetY );
  const QRect rect( offsetX - 0.5 * w - 0.5, -offsetY - 0.5 * h - 0.5, w, h );
  renderPrivate( context, center, rect, dpiScale );
}

bool KadasRectangleItemBase::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::DrawStatus::Drawing;
  state()->mPos = toItemPos( firstPoint, mapSettings );
  update();
  return false;
}

bool KadasRectangleItemBase::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasRectangleItemBase::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

void KadasRectangleItemBase::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

bool KadasRectangleItemBase::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasRectangleItemBase::endPart()
{
  state()->drawStatus = State::DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasRectangleItemBase::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute { "x" } );
  attributes.insert( AttrY, NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasRectangleItemBase::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasRectangleItemBase::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasRectangleItemBase::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  double tol = pickTolSqr( mapSettings );
  QList<KadasMapPos> corners = cornerPoints( mapSettings );
  for ( int i = 0, n = corners.size(); i < n; ++i )
  {
    if ( pos.sqrDist( corners[i] ) < tol )
    {
      return EditContext( QgsVertexId( 0, 0, 1 + i ), corners[i], KadasMapItem::AttribDefs(), i % 2 == 0 ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor );
    }
  }
  KadasMapPos testPos = toMapPos( constState()->mPos, mapSettings );
  bool frameClicked = hitTest( pos, mapSettings );
  if ( !mPosLocked && ( ( !constState()->frame() && frameClicked ) || pos.sqrDist( testPos ) < tol ) )
  {
    return EditContext( QgsVertexId( 0, 0, 0 ), testPos, drawAttribs() );
  }
  if ( frameClicked )
  {
    double mup = mapSettings.mapUnitsPerPixel();
    KadasMapPos mapPos = toMapPos( constState()->mPos, mapSettings );
    KadasMapPos framePos( mapPos.x() + constState()->mOffsetX * mup, mapPos.y() + constState()->mOffsetY * mup );
    return EditContext( QgsVertexId(), framePos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return EditContext();
}

void KadasRectangleItemBase::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.vertex == 0 )
  {
    state()->mPos = toItemPos( newPoint, mapSettings );
    state()->mFootprint.clear();
    update();
  }
  else if ( context.vidx.vertex >= 1 && context.vidx.vertex <= 4 )
  {
    editPrivate( newPoint, mapSettings );
    update();
  }
  else if ( constState()->frame() )
  {
    QgsCoordinateTransform crst( crs(), mapSettings.destinationCrs(), QgsProject::instance() );
    QgsPointXY screenPos = mapSettings.mapToPixel().transform( newPoint );
    QgsPointXY screenAnchor = mapSettings.mapToPixel().transform( toMapPos( constState()->mPos, mapSettings ) );
    state()->mOffsetX = screenPos.x() - screenAnchor.x();
    state()->mOffsetY = screenAnchor.y() - screenPos.y();
    update();
  }
}

void KadasRectangleItemBase::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasRectangleItemBase::populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings )
{
  QAction *frameAction = menu->addAction( QObject::tr( "Frame visible" ), [this]( bool active ) { setFrameVisible( active ); } );
  frameAction->setCheckable( true );
  frameAction->setChecked( constState()->frame() );

  QAction *lockedAction = menu->addAction( QObject::tr( "Position locked" ), [this]( bool active ) { setPositionLocked( active ); } );
  lockedAction->setCheckable( true );
  lockedAction->setChecked( mPosLocked );

  populateContextMenuPrivate( menu, context, clickPos, mapSettings );
}

KadasMapItem::AttribValues KadasRectangleItemBase::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasRectangleItemBase::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

static QMatrix3x3 rotAngleAxis( const std::array<float, 3> &u, float angle )
{
  float sina = std::sin( angle );
  float cosa = std::cos( angle );
  return QMatrix3x3(
    std::array<float, 9> {
      cosa + u[0] * u[0] * ( 1 - cosa ), u[0] * u[1] * ( 1 - cosa ) - u[2] * sina, u[0] * u[2] * ( 1 - cosa ) + u[1] * sina,
      u[0] * u[1] * ( 1 - cosa ) + u[2] * sina, cosa + u[1] * u[1] * ( 1 - cosa ), u[1] * u[2] * ( 1 - cosa ) - u[0] * sina,
      u[0] * u[2] * ( 1 - cosa ) - u[1] * sina, u[1] * u[2] * ( 1 - cosa ) + u[0] * sina, cosa + u[2] * u[2] * ( 1 - cosa )
    }
      .data()
  );
}
