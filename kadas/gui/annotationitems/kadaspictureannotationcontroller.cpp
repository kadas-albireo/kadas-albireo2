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

#include "kadas/gui/annotationitems/kadasannotationzindex.h"
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
  item->setZIndex( KadasAnnotationZIndex::Picture );
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

QList<KadasNode> KadasPictureAnnotationController::nodes( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  // Single edit handle = the geographic anchor (bounds center).
  const QgsRectangle b = asPicture( item )->bounds();
  return { { toMapPos( QgsPointXY( b.center().x(), b.center().y() ), ctx ) } };
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const QgsPointXY &firstPoint, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( firstPoint, ctx );
  // Bounds center is the anchor; size doesn't matter in FixedSize mode but
  // keeping it as a degenerate rectangle is clearer than a 0x0 area.
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  // Callout anchor mirrors the bounds center (a Point geometry).
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
  return false; // single-click placement
}

bool KadasPictureAnnotationController::startPart( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  return startPart( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

void KadasPictureAnnotationController::setCurrentPoint( QgsAnnotationItem *item, const QgsPointXY &p, const KadasAnnotationItemContext &ctx )
{
  const QgsPointXY ip = toItemPos( p, ctx );
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
}

void KadasPictureAnnotationController::setCurrentAttributes( QgsAnnotationItem *item, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  setCurrentPoint( item, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

bool KadasPictureAnnotationController::continuePart( QgsAnnotationItem *, const KadasAnnotationItemContext & )
{
  return false;
}

void KadasPictureAnnotationController::endPart( QgsAnnotationItem * )
{}

KadasAttribDefs KadasPictureAnnotationController::drawAttribs() const
{
  KadasAttribDefs attributes;
  attributes.insert( AttrX, KadasNumericAttribute { "x" } );
  attributes.insert( AttrY, KadasNumericAttribute { "y" } );
  return attributes;
}

KadasAttribValues KadasPictureAnnotationController::drawAttribsFromPosition( const QgsAnnotationItem *, const QgsPointXY &pos, const KadasAnnotationItemContext & ) const
{
  KadasAttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

QgsPointXY KadasPictureAnnotationController::positionFromDrawAttribs( const QgsAnnotationItem *, const KadasAttribValues &values, const KadasAnnotationItemContext & ) const
{
  return QgsPointXY( values[AttrX], values[AttrY] );
}

KadasEditContext KadasPictureAnnotationController::getEditContext( const QgsAnnotationItem *item, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  const QgsRectangle b = asPicture( item )->bounds();
  const QgsPointXY anchor = toMapPos( QgsPointXY( b.center().x(), b.center().y() ), ctx );
  if ( pos.sqrDist( anchor ) < pickTolSqr( ctx ) )
    return KadasEditContext( QgsVertexId( 0, 0, 0 ), anchor, drawAttribs() );
  // The picture frame itself sits at a screen-space offset and would need a
  // map-settings-aware pick test to hit-test; the whole-item move falls back
  // to anchor-relative for now.
  if ( toMapRect( b, ctx ).contains( pos ) )
    return KadasEditContext( QgsVertexId(), anchor, KadasAttribDefs(), Qt::ArrowCursor );
  return KadasEditContext();
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const QgsPointXY &newPoint, const KadasAnnotationItemContext &ctx )
{
  Q_UNUSED( editContext );
  // Both the anchor handle and the whole-item move set the new anchor.
  const QgsPointXY ip = toItemPos( newPoint, ctx );
  asPicture( item )->setBounds( QgsRectangle( ip.x(), ip.y(), ip.x(), ip.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( ip.x(), ip.y() ) ) );
}

void KadasPictureAnnotationController::edit( QgsAnnotationItem *item, const KadasEditContext &editContext, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx )
{
  edit( item, editContext, QgsPointXY( values[AttrX], values[AttrY] ), ctx );
}

KadasAttribValues KadasPictureAnnotationController::editAttribsFromPosition( const QgsAnnotationItem *item, const KadasEditContext &, const QgsPointXY &pos, const KadasAnnotationItemContext &ctx ) const
{
  return drawAttribsFromPosition( item, pos, ctx );
}

QgsPointXY KadasPictureAnnotationController::positionFromEditAttribs( const QgsAnnotationItem *item, const KadasEditContext &, const KadasAttribValues &values, const KadasAnnotationItemContext &ctx ) const
{
  return positionFromDrawAttribs( item, values, ctx );
}

QgsPointXY KadasPictureAnnotationController::position( const QgsAnnotationItem *item ) const
{
  const QgsRectangle b = asPicture( item )->bounds();
  return QgsPointXY( b.center().x(), b.center().y() );
}

void KadasPictureAnnotationController::setPosition( QgsAnnotationItem *item, const QgsPointXY &pos )
{
  asPicture( item )->setBounds( QgsRectangle( pos.x(), pos.y(), pos.x(), pos.y() ) );
  item->setCalloutAnchor( QgsGeometry( new QgsPoint( pos.x(), pos.y() ) ) );
}

void KadasPictureAnnotationController::translate( QgsAnnotationItem *item, double dx, double dy )
{
  const QgsRectangle b = asPicture( item )->bounds();
  const QgsPointXY c = b.center();
  setPosition( item, QgsPointXY( c.x() + dx, c.y() + dy ) );
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
