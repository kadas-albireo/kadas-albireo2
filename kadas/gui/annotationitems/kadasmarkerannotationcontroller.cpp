/***************************************************************************
    kadasmarkerannotationcontroller.cpp
    -----------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QObject>
#include <QTextStream>

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"

namespace
{
  inline QgsAnnotationMarkerItem *asMarker( QgsAnnotationItem *item )
  {
    Q_ASSERT( dynamic_cast<QgsAnnotationMarkerItem *>( item ) );
    return static_cast<QgsAnnotationMarkerItem *>( item );
  }
  inline const QgsAnnotationMarkerItem *asMarker( const QgsAnnotationItem *item )
  {
    Q_ASSERT( dynamic_cast<const QgsAnnotationMarkerItem *>( item ) );
    return static_cast<const QgsAnnotationMarkerItem *>( item );
  }
} // namespace


QString KadasMarkerAnnotationController::itemType() const
{
  return QStringLiteral( "marker" );
}

QString KadasMarkerAnnotationController::itemName() const
{
  return QObject::tr( "Point" );
}

QgsAnnotationItem *KadasMarkerAnnotationController::createItem() const
{
  return new QgsAnnotationMarkerItem( QgsPoint() );
}

QList<KadasMapItem::Node> KadasMarkerAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const QgsPointXY p = asMarker( item )->geometry();
  return { { toMapPos( KadasItemPos( p.x(), p.y() ), ctx ) } };
}

bool KadasMarkerAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapPos &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const KadasItemPos ip = toItemPos( firstPoint, ctx );
  asMarker( item )->setGeometry( QgsPoint( ip.x(), ip.y() ) );
  // Marker is fully placed after the first click; no further points expected.
  return false;
}

bool KadasMarkerAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

void KadasMarkerAnnotationController::setCurrentPoint( QgsAnnotationItem *, const KadasMapPos &, const KadasAnnotationItemContext & )
{
  // no-op: marker is finalized in startPart
}

void KadasMarkerAnnotationController::setCurrentAttributes( QgsAnnotationItem *, const KadasMapItem::AttribValues &, const KadasAnnotationItemContext & )
{
  // no-op
}

bool KadasMarkerAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasMarkerAnnotationController::endPart( QgsAnnotationItem * )
{
  // no-op
}

KadasMapItem::AttribDefs KadasMarkerAnnotationController::drawAttribs() const
{
  KadasMapItem::AttribDefs attributes;
  attributes.insert( AttrX, KadasMapItem::NumericAttribute { "x" } );
  attributes.insert( AttrY, KadasMapItem::NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasMarkerAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const KadasMapPos &pos, const KadasAnnotationItemContext & ) const
{
  KadasMapItem::AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasMarkerAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext & ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasMarkerAnnotationController::getEditContext( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsPointXY geom = asMarker( item )->geometry();
  const QgsPointXY testPos = toMapPos( geom, ctx );
  if ( pos.sqrDist( KadasMapPos::fromPoint( testPos ) ) < pickTolSqr( ctx ) )
  {
    return KadasMapItem::EditContext( QgsVertexId( 0, 0, 0 ), KadasMapPos::fromPoint( testPos ), drawAttribs() );
  }
  return KadasMapItem::EditContext();
}

void KadasMarkerAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapPos &newPoint, const KadasAnnotationItemContext &ctx )
{
  const KadasItemPos ip = toItemPos( newPoint, ctx );
  asMarker( item )->setGeometry( QgsPoint( ip.x(), ip.y() ) );
}

void KadasMarkerAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

KadasMapItem::AttribValues KadasMarkerAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx
) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

KadasMapPos KadasMarkerAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

KadasItemPos KadasMarkerAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsPointXY p = asMarker( item )->geometry();
  return KadasItemPos( p.x(), p.y() );
}

void KadasMarkerAnnotationController::setPosition( QgsAnnotationItem *item, const KadasItemPos &pos )
{
  asMarker( item )->setGeometry( QgsPoint( pos.x(), pos.y() ) );
}

void KadasMarkerAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  QgsAnnotationMarkerItem *marker = asMarker( item );
  const QgsPointXY p = marker->geometry();
  marker->setGeometry( QgsPoint( p.x() + dx, p.y() + dy ) );
}

QString KadasMarkerAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const QgsAnnotationMarkerItem *marker = asMarker( item );
  if ( marker->geometry().isEmpty() )
    return QString();

  // Read styling from the marker's first simple-marker symbol layer (if any).
  Qgis::MarkerShape shape = Qgis::MarkerShape::Circle;
  QColor fillColor = Qt::white;
  QColor strokeColor = Qt::red;
  double strokeWidth = 1.0;
  Qt::PenStyle strokeStyle = Qt::SolidLine;
  Qt::BrushStyle fillStyle = Qt::SolidPattern;
  if ( const QgsMarkerSymbol *symbol = marker->symbol() )
  {
    if ( symbol->symbolLayerCount() > 0 )
    {
      if ( const QgsSimpleMarkerSymbolLayer *sl = dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( symbol->symbolLayer( 0 ) ) )
      {
        shape = sl->shape();
        fillColor = sl->color();
        strokeColor = sl->strokeColor();
        strokeWidth = sl->strokeWidth();
        strokeStyle = sl->strokeStyle();
      }
    }
  }

  auto color2hex = []( const QColor &c ) {
    return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) );
  };

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  // Note: Kadas-specific exportName() lived on KadasMapItem; QgsAnnotationItem has no equivalent.
  // The owning layer / per-item metadata can carry a display name in the future; for now omit.
  outStream << "<name></name>\n";
  outStream << "<Style>\n";
  outStream << QString( "<LineStyle><width>%1</width><color>%2</color></LineStyle>\n<PolyStyle><fill>%3</fill><color>%4</color></PolyStyle>\n" )
                 .arg( strokeWidth )
                 .arg( color2hex( strokeColor ) )
                 .arg( 1 )
                 .arg( color2hex( fillColor ) );
  outStream << "</Style>\n";
  outStream << "<ExtendedData>\n";
  outStream << "<SchemaData schemaUrl=\"#KadasGeometryItem\">\n";
  outStream << QString( "<SimpleData name=\"icon_type\">%1</SimpleData>\n" ).arg( static_cast<int>( shape ) );
  outStream << QString( "<SimpleData name=\"outline_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodePenStyle( strokeStyle ) );
  outStream << QString( "<SimpleData name=\"fill_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodeBrushStyle( fillStyle ) );
  outStream << "</SchemaData>\n";
  outStream << "</ExtendedData>\n";
  QgsPoint point( marker->geometry() );
  point.transform( QgsCoordinateTransform( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ) );
  outStream << point.asKml( 6 ) << "\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
