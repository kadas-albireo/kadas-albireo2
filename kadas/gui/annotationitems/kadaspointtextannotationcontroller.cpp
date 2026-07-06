/***************************************************************************
    kadaspointtextannotationcontroller.cpp
    --------------------------------------
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

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QTextStream>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgssettingsentryimpl.h>
#include <qgis/qgstextformat.h>

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadasannotationrotation.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadaspointtextannotationcontroller.h"


const QgsSettingsEntryDouble *KadasPointTextAnnotationController::settingsSize
  = new QgsSettingsEntryDouble( QStringLiteral( "text-size" ), sTreeAnnotation, 10.0, QStringLiteral( "Last-used point-text size (points)." ) );
const QgsSettingsEntryColor *KadasPointTextAnnotationController::settingsColor
  = new QgsSettingsEntryColor( QStringLiteral( "text-color" ), sTreeAnnotation, QColor( 0, 0, 0 ), QStringLiteral( "Last-used point-text color." ) );
const QgsSettingsEntryColor *KadasPointTextAnnotationController::settingsBufferColor
  = new QgsSettingsEntryColor( QStringLiteral( "text-buffer-color" ), sTreeAnnotation, QColor( 255, 255, 255, 0 ), QStringLiteral( "Last-used point-text buffer (border) color." ) );
const QgsSettingsEntryDouble *KadasPointTextAnnotationController::settingsBufferWidth
  = new QgsSettingsEntryDouble( QStringLiteral( "text-buffer-width" ), sTreeAnnotation, 0.0, QStringLiteral( "Last-used point-text buffer (border) width (mm)." ) );

namespace
{
  inline QgsAnnotationPointTextItem *asText( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "pointtext" ) );
    return static_cast<QgsAnnotationPointTextItem *>( item );
  }
  inline const QgsAnnotationPointTextItem *asText( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "pointtext" ) );
    return static_cast<const QgsAnnotationPointTextItem *>( item );
  }
} // namespace


QString KadasPointTextAnnotationController::itemType() const
{
  return QStringLiteral( "pointtext" );
}

QString KadasPointTextAnnotationController::itemName() const
{
  return QObject::tr( "Text" );
}

QgsAnnotationItem *KadasPointTextAnnotationController::createItem() const
{
  auto *item = new QgsAnnotationPointTextItem( QString(), QgsPointXY() );
  item->setZIndex( KadasAnnotationZIndex::PointText );
  return item;
}

bool KadasPointTextAnnotationController::isEmpty( const QgsAnnotationItem *item ) const
{
  return asText( item )->text().trimmed().isEmpty();
}

QList<KadasNode> KadasPointTextAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  const QgsPointXY anchor = toMapPos( asText( item )->point(), ctx );
  const double off = KadasAnnotationRotation::sHandleOffsetPixels * ctx.mapSettings().mapUnitsPerPixel();
  const QgsPointXY handle = KadasAnnotationRotation::handlePos( anchor, asText( item )->angle(), off );
  return { { anchor }, { handle, []( QPainter *p, const QPointF &pt, int sz ) { KadasAnnotationRotation::renderHandle( p, pt, sz ); } } };
}

bool KadasPointTextAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  asText( item )->setPoint( toItemPos( firstPoint, ctx ) );
  return false;
}

bool KadasPointTextAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasPointTextAnnotationController::setCurrentPoint( QgsAnnotationItem *, const QgsPointXY &, const KadasAnnotationItemContext & )
{}

void KadasPointTextAnnotationController::setCurrentAttributes( QgsAnnotationItem *, const KadasAttribValues &, const KadasAnnotationItemContext & )
{}

bool KadasPointTextAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasPointTextAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasPointTextAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasPointTextAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasPointTextAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasPointTextAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsPointXY testPos = toMapPos( QgsPointXY( asText( item )->point() ), ctx );
  if ( pos.sqrDist( testPos ) < pickTolSqr( ctx ) )
  {
    return KadasEditContext( QgsVertexId( 0, 0, 0 ), testPos, drawAttribs() );
  }

  const double off = KadasAnnotationRotation::sHandleOffsetPixels * ctx.mapSettings().mapUnitsPerPixel();
  const QgsPointXY handle = KadasAnnotationRotation::handlePos( testPos, asText( item )->angle(), off );
  if ( pos.sqrDist( handle ) < pickTolSqr( ctx ) )
  {
    KadasAttribDefs rot;
    rot.insert( AttrAngle, KadasNumericAttribute { "angle", KadasNumericAttribute::Type::TypeAngle } );
    return KadasEditContext( QgsVertexId( 0, 0, RotationHandleVertex ), handle, rot, Qt::CrossCursor );
  }

  QgsRenderContext rc = QgsRenderContext::fromMapSettings( ctx.mapSettings() );
  QgsRectangle bbox = item->boundingBox( rc );
  bbox.grow( 4 * ctx.mapSettings().mapUnitsPerPixel() );
  if ( bbox.contains( toItemPos( pos, ctx ) ) )
  {
    return KadasEditContext( QgsVertexId( 0, 0, 0 ), testPos, drawAttribs() );
  }
  return KadasEditContext();
}

void KadasPointTextAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  if ( editContext.vidx.vertex == RotationHandleVertex )
  {
    const QgsPointXY center = toMapPos( asText( item )->point(), ctx );
    const double angle = KadasAnnotationRotation::angleFromHandle( center, newPoint );
    asText( item )->setAngle( KadasAnnotationRotation::snapAngle( angle, ctx.modifiers() & Qt::ShiftModifier ) );
    return;
  }
  asText( item )->setPoint( toItemPos( newPoint, ctx ) );
}

void KadasPointTextAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  if ( editContext.vidx.vertex == RotationHandleVertex )
  {
    asText( item )->setAngle( values[AttrAngle] );
    return;
  }
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasPointTextAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx
) const
{
  if ( editContext.vidx.vertex == RotationHandleVertex )
  {
    const QgsPointXY center = toMapPos( asText( item )->point(), ctx );
    KadasAttribValues v;
    v.insert( AttrAngle, KadasAnnotationRotation::angleFromHandle( center, pos ) );
    return v;
  }
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasPointTextAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  if ( editContext.vidx.vertex == RotationHandleVertex )
    return asText( item )->point();
  return positionFromDrawAttribs( item, values, ctx );
}

void KadasPointTextAnnotationController::populateContextMenu( QgsAnnotationItem *item, QMenu *menu, const KadasEditContext &, const QgsPointXY &, const KadasAnnotationItemContext &ctx )
{
  QgsAnnotationPointTextItem *text = asText( item );
  if ( text->angle() == 0.0 )
    return;
  QPointer<QgsAnnotationLayer> layerPtr( ctx.layer() );
  QAction *reset = menu->addAction( QObject::tr( "Reset rotation" ) );
  QObject::connect( reset, &QAction::triggered, reset, [text, layerPtr]() {
    text->setAngle( 0.0 );
    if ( layerPtr )
      layerPtr->triggerRepaint();
  } );
}

QgsPointXY KadasPointTextAnnotationController::position( const QgsAnnotationItem *item ) const
{
  return asText( item )->point();
}

void KadasPointTextAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  asText( item )->setPoint( pos );
}

void KadasPointTextAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  QgsAnnotationPointTextItem *text = asText( item );
  const QgsPointXY p = text->point();
  text->setPoint( QgsPointXY( p.x() + dx, p.y() + dy ) );
}

QString KadasPointTextAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  const QgsAnnotationPointTextItem *text = asText( item );

  auto color2hex = []( const QColor &c ) {
    return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) );
  };

  const QgsTextFormat fmt = text->format();
  const QColor color = fmt.color();
  const QFont font = fmt.font();

  const QgsPointXY pos = QgsCoordinateTransform( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ).transform( text->point() );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>" << "\n";
  outStream << QString( "<name>%1</name>\n" ).arg( text->text() );
  outStream << "<Style>";
  outStream << QString( "<IconStyle><scale>0</scale></IconStyle><LabelStyle><color>%1</color><scale>%2</scale></LabelStyle></Style>" ).arg( color2hex( color ) ).arg( font.pointSizeF() / QFont().pointSizeF() );
  outStream << QString( "<Point><coordinates>%1,%2</coordinates></Point>" ).arg( QString::number( pos.x(), 'f', 10 ) ).arg( QString::number( pos.y(), 'f', 10 ) );
  outStream << "</Placemark>" << "\n";
  outStream.flush();
  return outString;
}

void KadasPointTextAnnotationController::applyPersistedStyle( QgsAnnotationItem *item ) const
{
  auto *pt = dynamic_cast<QgsAnnotationPointTextItem *>( item );
  if ( !pt || !settingsColor->exists() )
    return;
  QgsTextFormat fmt = pt->format();
  fmt.setSize( settingsSize->value() );
  fmt.setSizeUnit( Qgis::RenderUnit::Points );
  fmt.setColor( settingsColor->value() );
  QgsTextBufferSettings buf = fmt.buffer();
  const double bw = settingsBufferWidth->value();
  const QColor bc = settingsBufferColor->value();
  buf.setEnabled( bw > 0.0 && bc.alpha() > 0 );
  buf.setColor( bc );
  buf.setSize( bw );
  buf.setSizeUnit( Qgis::RenderUnit::Millimeters );
  fmt.setBuffer( buf );
  pt->setFormat( fmt );
}

void KadasPointTextAnnotationController::persistStyle( const QgsAnnotationItem *item ) const
{
  const auto *pt = dynamic_cast<const QgsAnnotationPointTextItem *>( item );
  if ( !pt )
    return;
  const QgsTextFormat fmt = pt->format();
  settingsSize->setValue( fmt.size() );
  settingsColor->setValue( fmt.color() );
  const QgsTextBufferSettings buf = fmt.buffer();
  settingsBufferColor->setValue( buf.enabled() ? buf.color() : QColor( 0, 0, 0, 0 ) );
  settingsBufferWidth->setValue( buf.enabled() ? buf.size() : 0.0 );
}

KadasAnnotationStyleEditor *KadasPointTextAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasPointTextStyleEditor( parent );
}
