/***************************************************************************
    kadaspictureannotationcontroller.cpp
    ------------------------------------
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

#include <QFileInfo>
#include <QObject>
#include <QTextStream>

#include <qgis/qgis.h>
#include <qgis/qgsannotationpictureitem.h>
#include <qgis/qgscallout.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/annotationitems/kadaspictureannotationcontroller.h"

namespace
{
  inline QgsAnnotationPictureItem *asPicture( QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "picture" ) );
    return static_cast<QgsAnnotationPictureItem *>( item );
  }
  inline const QgsAnnotationPictureItem *asPicture( const QgsAnnotationItem *item )
  {
    Q_ASSERT( item && item->type() == QLatin1String( "picture" ) );
    return static_cast<const QgsAnnotationPictureItem *>( item );
  }

  Qgis::PictureFormat formatFromPath( const QString &path )
  {
    const QString ext = QFileInfo( path ).suffix().toLower();
    if ( ext == QLatin1String( "svg" ) )
      return Qgis::PictureFormat::SVG;
    if (
      ext == QLatin1String( "png" )
      || ext == QLatin1String( "jpg" )
      || ext == QLatin1String( "jpeg" )
      || ext == QLatin1String( "bmp" )
      || ext == QLatin1String( "gif" )
      || ext == QLatin1String( "tif" )
      || ext == QLatin1String( "tiff" )
    )
      return Qgis::PictureFormat::Raster;
    return Qgis::PictureFormat::Unknown;
  }
} // namespace


QString KadasPictureAnnotationController::itemType() const
{
  return QStringLiteral( "picture" );
}

QString KadasPictureAnnotationController::itemName() const
{
  return QObject::tr( "Picture" );
}

void KadasPictureAnnotationController::setPath( QgsAnnotationPictureItem *item, const QString &path )
{
  if ( !item )
    return;
  item->setPath( formatFromPath( path ), path );
}

QgsAnnotationItem *KadasPictureAnnotationController::createItem() const
{
  // Default 200x150 pixel picture frame placed at the origin; the actual
  // anchor is set by startPart().
  auto *item = new QgsAnnotationPictureItem( Qgis::PictureFormat::Unknown, QString(), QgsRectangle( 0, 0, 0, 0 ) );
  item->setPlacementMode( Qgis::AnnotationPlacementMode::FixedSize );
  item->setFixedSize( QSizeF( 200, 150 ) );
  item->setFixedSizeUnit( Qgis::RenderUnit::Pixels );
  // Default offset so the picture sits visibly above its anchor; users can
  // adjust via the offset handle. Units are pixels, matching fixedSizeUnit.
  item->setOffsetFromCallout( QSizeF( 0, -50 ) );
  item->setOffsetFromCalloutUnit( Qgis::RenderUnit::Pixels );
  // Auto-install a simple-line callout from the anchor to the picture.
  item->setCallout( new QgsSimpleLineCallout() );
  return item;
}

QList<KadasMapItem::Node> KadasPictureAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  // Single edit handle = the geographic anchor (bounds center).
  const QgsRectangle b = asPicture( item )->bounds();
  return { { toMapPos( KadasItemPos( b.center().x(), b.center().y() ), ctx ) } };
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapPos &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const KadasItemPos ip = toItemPos( firstPoint, ctx );
  // Bounds center is the anchor; size doesn't matter in FixedSize mode but
  // keeping it as a degenerate rectangle is clearer than a 0x0 area.
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  // Callout anchor mirrors the bounds center (a Point geometry).
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
  return false; // single-click placement
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

void KadasPictureAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const KadasMapPos &p, const KadasAnnotationItemContext &ctx )
{
  const KadasItemPos ip = toItemPos( p, ctx );
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
}

void KadasPictureAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

bool KadasPictureAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasPictureAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasMapItem::AttribDefs KadasPictureAnnotationController::drawAttribs() const
{
  KadasMapItem::AttribDefs attributes;
  attributes.insert( AttrX, KadasMapItem::NumericAttribute { "x" } );
  attributes.insert( AttrY, KadasMapItem::NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasPictureAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const KadasMapPos &pos, const KadasAnnotationItemContext & ) const
{
  KadasMapItem::AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasPictureAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext & ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasPictureAnnotationController::getEditContext( const QgsAnnotationItem *item, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsRectangle b = asPicture( item )->bounds();
  const KadasMapPos anchor = toMapPos( KadasItemPos( b.center().x(), b.center().y() ), ctx );
  if ( pos.sqrDist( anchor ) < pickTolSqr( ctx ) )
    return KadasMapItem::EditContext( QgsVertexId( 0, 0, 0 ), anchor, drawAttribs() );
  // The picture frame itself sits at a screen-space offset and would need a
  // map-settings-aware pick test to hit-test; the whole-item move falls back
  // to anchor-relative for now.
  if ( toMapRect( b, ctx ).contains( pos ) )
    return KadasMapItem::EditContext( QgsVertexId(), anchor, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  return KadasMapItem::EditContext();
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapPos &newPoint, const KadasAnnotationItemContext &ctx )
{
  Q_UNUSED( editContext );
  // Both the anchor handle and the whole-item move set the new anchor.
  const KadasItemPos ip = toItemPos( newPoint, ctx );
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasMapItem::EditContext &editContext, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, KadasMapPos( values[AttrX], values[AttrY] ), ctx );
}

KadasMapItem::AttribValues KadasPictureAnnotationController::editAttribsFromPosition(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapPos &pos, const KadasAnnotationItemContext &ctx
) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

KadasMapPos KadasPictureAnnotationController::positionFromEditAttribs(
  const QgsAnnotationItem *item, const KadasMapItem::EditContext &, const KadasMapItem::AttribValues &values, const KadasAnnotationItemContext &ctx
) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

KadasItemPos KadasPictureAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsRectangle b = asPicture( item )->bounds();
  return KadasItemPos( b.center().x(), b.center().y() );
}

void KadasPictureAnnotationController::setPosition( QgsAnnotationItem *item, const KadasItemPos &pos )
{
  asPicture( item )->setBounds( QgsRectangle( pos.x(), pos.y(), pos.x(), pos.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( pos.x(), pos.y() ) ) );
}

void KadasPictureAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  const QgsRectangle b = asPicture( item )->bounds();
  const QgsPointXY c = b.center();
  setPosition( item, KadasItemPos( c.x() + dx, c.y() + dy ) );
}

QString KadasPictureAnnotationController::asKml( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs, const QgsRenderContext &, QuaZip * ) const
{
  // KMZ embedding of the image asset is deferred; emit a Point Placemark at
  // the anchor so the picture is at least geographically locatable in KML.
  const QgsRectangle b = asPicture( item )->bounds();
  const QgsCoordinateTransform ct( itemCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  const QgsPointXY anchor = ct.transform( QgsPointXY( b.center() ) );

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << "<name></name>\n";
  outStream << "<Point><coordinates>" << QString::number( anchor.x(), 'f', 10 ) << "," << QString::number( anchor.y(), 'f', 10 ) << "</coordinates></Point>\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}
