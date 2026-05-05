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
#include <cmath>
#include <memory>

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"


// ----- Persisted last-used style ----------------------------------------

const QgsSettingsEntryInteger *KadasMarkerAnnotationController::settingsShape
  = new QgsSettingsEntryInteger( QStringLiteral( "marker-shape" ), sTreeAnnotation, static_cast<int>( Qgis::MarkerShape::Circle ), QStringLiteral( "Last-used marker shape." ) );
const QgsSettingsEntryInteger *KadasMarkerAnnotationController::settingsSize
  = new QgsSettingsEntryInteger( QStringLiteral( "marker-size" ), sTreeAnnotation, 3, QStringLiteral( "Last-used marker size (mm)." ) );
const QgsSettingsEntryDouble *KadasMarkerAnnotationController::settingsStrokeWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "marker-stroke-width" ), sTreeAnnotation, 0.0, QStringLiteral( "Last-used marker outline width (mm)." ) );
const QgsSettingsEntryColor *KadasMarkerAnnotationController::settingsFillColor
  = new QgsSettingsEntryColor( QStringLiteral( "marker-fill-color" ), sTreeAnnotation, QColor( 255, 0, 0 ), QStringLiteral( "Last-used marker fill color." ) );
const QgsSettingsEntryColor *KadasMarkerAnnotationController::settingsStrokeColor
  = new QgsSettingsEntryColor( QStringLiteral( "marker-stroke-color" ), sTreeAnnotation, QColor( 0, 0, 0 ), QStringLiteral( "Last-used marker outline color." ) );
const QgsSettingsEntryInteger *KadasMarkerAnnotationController::settingsStrokeStyle
  = new QgsSettingsEntryInteger( QStringLiteral( "marker-stroke-style" ), sTreeAnnotation, static_cast<int>( Qt::SolidLine ), QStringLiteral( "Last-used marker outline style." ) );

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
  auto *item = new QgsAnnotationMarkerItem( QgsPoint() );
  item->setZIndex( KadasAnnotationZIndex::Marker );
  // QgsAnnotationMarkerItem ships with an empty QgsMarkerSymbol (zero
  // symbol layers), which renders nothing. Install a sensible default so
  // a freshly placed marker is visible even before any persisted style.
  auto *sl = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Circle );
  sl->setSize( 3.0 );
  sl->setColor( QColor( 255, 0, 0 ) );
  sl->setStrokeColor( QColor( 0, 0, 0 ) );
  QgsMarkerSymbol *sym = new QgsMarkerSymbol( QgsSymbolLayerList() << sl );
  item->setSymbol( sym );
  return item;
}

QList<KadasNode> KadasMarkerAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const QgsPointXY p = asMarker( item )->geometry();
  return { { toMapPos( QgsPointXY( p.x(), p.y() ), ctx ) } };
}

bool KadasMarkerAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  asMarker( item )->setGeometry( QgsPoint( ip.x(), ip.y() ) );
  // Marker is fully placed after the first click; no further points expected.
  return false;
}

bool KadasMarkerAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasMarkerAnnotationController::setCurrentPoint( QgsAnnotationItem *, const QgsPointXY &, const KadasAnnotationItemContext & )
{
  // no-op: marker is finalized in startPart
}

void KadasMarkerAnnotationController::setCurrentAttributes( QgsAnnotationItem *, const KadasAttribValues &, const KadasAnnotationItemContext & )
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

KadasAttribDefs KadasMarkerAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasMarkerAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasMarkerAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasMarkerAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsPointXY geom = asMarker( item )->geometry();
  const QgsPointXY testPos = toMapPos( geom, ctx );
  if ( pos.sqrDist( testPos ) < pickTolSqr( ctx ) )
  {
    return KadasEditContext( QgsVertexId( 0, 0, 0 ), testPos, drawAttribs() );
  }
  return KadasEditContext();
}

void KadasMarkerAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( newPoint, ctx );
  asMarker( item )->setGeometry( QgsPoint( ip.x(), ip.y() ) );
}

void KadasMarkerAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasMarkerAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasMarkerAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasMarkerAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsPointXY p = asMarker( item )->geometry();
  return QgsPointXY( p.x(), p.y() );
}

void KadasMarkerAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
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

void KadasMarkerAnnotationController::applyPersistedStyle( QgsAnnotationItem *item ) const
{
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item );
  if ( !marker || !settingsStrokeColor->exists() )
    return;
  std::unique_ptr<QgsMarkerSymbol> sym( marker->symbol() ? marker->symbol()->clone() : new QgsMarkerSymbol() );
  if ( sym->symbolLayerCount() == 0 )
    sym->appendSymbolLayer( new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Circle ) );
  auto *sl = dynamic_cast<QgsSimpleMarkerSymbolLayer *>( sym->symbolLayer( 0 ) );
  if ( !sl )
  {
    // Subclasses (pin SVG, coord cross, GPX waypoint, ...) ship a custom
    // symbol that must not be replaced by the generic-marker defaults.
    return;
  }
  // Preserve the shape chosen by the toolbar / item factory; persist only
  // the visual style attributes (size, colors, stroke).
  const int size = std::max( 1, settingsSize->value() );
  sl->setSize( size );
  sl->setStrokeWidth( settingsStrokeWidth->value() );
  // Reject fully-transparent fills, which would make every freshly placed
  // marker invisible after a single accidental persist.
  QColor fill = settingsFillColor->value();
  if ( fill.alpha() == 0 )
    fill.setAlpha( 255 );
  sl->setColor( fill );
  sl->setStrokeColor( settingsStrokeColor->value() );
  sl->setStrokeStyle( static_cast<Qt::PenStyle>( settingsStrokeStyle->value() ) );
  marker->setSymbol( sym.release() );
}

void KadasMarkerAnnotationController::persistStyle( const QgsAnnotationItem *item ) const
{
  const auto *marker = dynamic_cast<const QgsAnnotationMarkerItem *>( item );
  if ( !marker || !marker->symbol() || marker->symbol()->symbolLayerCount() == 0 )
    return;
  const auto *sl = dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( marker->symbol()->symbolLayer( 0 ) );
  if ( !sl )
    return;
  settingsSize->setValue( static_cast<int>( std::round( sl->size() ) ) );
  settingsStrokeWidth->setValue( sl->strokeWidth() );
  settingsFillColor->setValue( sl->color() );
  settingsStrokeColor->setValue( sl->strokeColor() );
  settingsStrokeStyle->setValue( static_cast<int>( sl->strokeStyle() ) );
}

KadasAnnotationStyleEditor *KadasMarkerAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasMarkerStyleEditor( parent );
}
