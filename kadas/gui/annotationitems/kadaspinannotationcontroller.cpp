/***************************************************************************
    kadaspinannotationcontroller.cpp
    --------------------------------
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

#include <QFile>
#include <QObject>
#include <QTextDocument>

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayer.h>

#include "kadas/core/kadascoordinateformat.h"
#include "kadas/core/kadascoordinateutils.h"
#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadaspinannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"


void KadasPinAnnotationController::embedQrcSvgPaths( QgsMarkerSymbol *symbol )
{
  if ( !symbol )
    return;
  for ( int i = 0; i < symbol->symbolLayerCount(); ++i )
  {
    auto *svg = dynamic_cast<QgsSvgMarkerSymbolLayer *>( symbol->symbolLayer( i ) );
    if ( !svg )
      continue;
    const QString path = svg->path();
    if ( !path.startsWith( QLatin1Char( ':' ) ) )
      continue;
    QFile f( path );
    if ( !f.open( QIODevice::ReadOnly ) )
      continue;
    const QByteArray data = f.readAll();
    svg->setPath( QStringLiteral( "base64:" ) + QString::fromLatin1( data.toBase64() ) );
  }
}

bool KadasPinAnnotationController::projectHasHeightmap()
{
  const QString layerId = QgsProject::instance()->readEntry( QStringLiteral( "Heightmap" ), QStringLiteral( "layer" ) );
  const QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerId );
  return layer && layer->type() == Qgis::LayerType::Raster;
}

// Where the pin is, rather than what it says: a quiet grey band above the
// pin's own text, labels set small, italic and grey so the values lead.
QString KadasPinAnnotationController::headerHtml( const QgsPointXY &pos, const QgsCoordinateReferenceSystem &crs )
{
  KadasCoordinateFormat *coordinateFormat = KadasCoordinateFormat::instance();
  QString posStr = coordinateFormat->getDisplayString( pos, crs );
  if ( posStr.isEmpty() )
    posStr = QStringLiteral( "%1 (%2)" ).arg( pos.toString(), crs.authid() );
  posStr = posStr.toHtmlEscaped();

  const Qgis::DistanceUnit unit = coordinateFormat->getHeightDisplayUnit();
  QString errMsg;
  const double height = KadasCoordinateUtils::getHeightAtPos( pos, crs, unit, &errMsg );
  QString heightStr;
  if ( errMsg.isEmpty() )
    heightStr = QStringLiteral( "%1 %2" ).arg( QString::number( height, 'f', 0 ), unit == Qgis::DistanceUnit::Feet ? QObject::tr( "ft" ) : QObject::tr( "m" ) );
  else
    heightStr = QStringLiteral( "<i>%1</i>" ).arg( projectHasHeightmap() ? QObject::tr( "undefined (outside of heightmap)" ) : QObject::tr( "undefined (no heightmap defined)" ) );

  const QString row = QStringLiteral(
    "<tr><td width=\"30%\"><small><i><font color=\"#6B6B6B\">%1</font></i></small></td>"
    "<td><small>%2</small></td></tr>"
  );
  // The band carries the gap to whatever follows, so the spacing holds whether
  // or not the pin has a title.
  return QStringLiteral( "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"4\" bgcolor=\"#ECECEC\" style=\"margin-bottom:8px;\">%1%2</table>" )
    .arg( row.arg( QObject::tr( "Position" ), posStr ), row.arg( QObject::tr( "Altitude" ), heightStr ) );
}

QString KadasPinAnnotationController::itemType() const
{
  return KadasPinAnnotationItem::itemTypeId();
}

QString KadasPinAnnotationController::itemName() const
{
  return QObject::tr( "Pin" );
}

QgsAnnotationItem *KadasPinAnnotationController::createItem() const
{
  auto *item = new KadasPinAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::Pin );
  return item;
}

KadasAnnotationStyleEditor *KadasPinAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasPinStyleEditor( parent );
}

QString KadasPinAnnotationController::tooltip( const QgsAnnotationItem *item, const QgsCoordinateReferenceSystem &itemCrs ) const
{
  const auto *pin = dynamic_cast<const KadasPinAnnotationItem *>( item );
  if ( !pin )
    return QString();

  // Blocks are appended as siblings: nesting the description inside a paragraph
  // of our own would have Qt close that paragraph and leave a gap behind.
  QString html = headerHtml( pin->geometry(), itemCrs );
  if ( !pin->name().isEmpty() )
    html += QStringLiteral( "<p><b>%1</b></p>" ).arg( pin->name().toHtmlEscaped() );
  html += KadasPinAnnotationItem::remarksAsHtml( pin->remarks() );
  return html;
}

QList<QgsAnnotationItem *> KadasPinAnnotationController::generateShadows( const QgsAnnotationItem *item, const KadasAnnotationItemContext &ctx ) const
{
  Q_UNUSED( ctx );
  const auto *master = static_cast<const KadasPinAnnotationItem *>( item );
  const QgsPointXY pt = master->geometry();
  auto *shadow = new QgsAnnotationMarkerItem( QgsPoint( pt.x(), pt.y() ) );
  if ( master->symbol() )
  {
    QgsMarkerSymbol *cloned = master->symbol()->clone();
    embedQrcSvgPaths( cloned );
    shadow->setSymbol( cloned );
  }
  shadow->setZIndex( master->zIndex() );
  return { shadow };
}

QStringList KadasPinAnnotationController::shadowIds( const QgsAnnotationItem *item ) const
{
  return static_cast<const KadasPinAnnotationItem *>( item )->shadowIds();
}

void KadasPinAnnotationController::setShadowIds( QgsAnnotationItem *item, const QStringList &ids ) const
{
  static_cast<KadasPinAnnotationItem *>( item )->setShadowIds( ids );
}
