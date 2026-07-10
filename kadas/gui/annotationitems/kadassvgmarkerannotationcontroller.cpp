/***************************************************************************
    kadassvgmarkerannotationcontroller.cpp
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

#include <QObject>
#include <cmath>
#include <memory>

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssettingsentryimpl.h>

#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadassvgmarkerannotationcontroller.h"
#include "kadas/gui/annotationitems/kadassvgmarkerannotationitem.h"


// ----- Persisted last-used style ----------------------------------------

const QgsSettingsEntryString *KadasSvgMarkerAnnotationController::settingsSvgPath
  = new QgsSettingsEntryString( QStringLiteral( "svgmarker-path" ), sTreeAnnotation, KadasSvgMarkerAnnotationItem::placeholderIconPath(), QStringLiteral( "Last-used custom SVG marker path." ) );
const QgsSettingsEntryInteger *KadasSvgMarkerAnnotationController::settingsSvgSize
  = new QgsSettingsEntryInteger( QStringLiteral( "svgmarker-size" ), sTreeAnnotation, 24, QStringLiteral( "Last-used custom SVG marker size." ) );
const QgsSettingsEntryColor *KadasSvgMarkerAnnotationController::settingsSvgFillColor
  = new QgsSettingsEntryColor( QStringLiteral( "svgmarker-fill-color" ), sTreeAnnotation, QColor( 255, 0, 0 ), QStringLiteral( "Last-used custom SVG marker fill color." ) );


QString KadasSvgMarkerAnnotationController::itemType() const
{
  return KadasSvgMarkerAnnotationItem::itemTypeId();
}

QString KadasSvgMarkerAnnotationController::itemName() const
{
  return QObject::tr( "SVG Marker" );
}

QgsAnnotationItem *KadasSvgMarkerAnnotationController::createItem() const
{
  auto *item = new KadasSvgMarkerAnnotationItem();
  item->setZIndex( KadasAnnotationZIndex::Marker );
  return item;
}

KadasAnnotationStyleEditor *KadasSvgMarkerAnnotationController::createStyleEditor( QWidget *parent ) const
{
  return new KadasSvgMarkerStyleEditor( parent );
}

void KadasSvgMarkerAnnotationController::applyPersistedStyle( QgsAnnotationItem *item ) const
{
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item );
  if ( !marker || !marker->symbol() || marker->symbol()->symbolLayerCount() == 0 )
    return;
  std::unique_ptr<QgsMarkerSymbol> sym( marker->symbol()->clone() );
  auto *sl = dynamic_cast<QgsSvgMarkerSymbolLayer *>( sym->symbolLayer( 0 ) );
  if ( !sl )
    return;
  const QString path = settingsSvgPath->value();
  if ( !path.isEmpty() )
    sl->setPath( path );
  sl->setSize( std::max( 1, settingsSvgSize->value() ) );
  sl->setFillColor( settingsSvgFillColor->value() );
  marker->setSymbol( sym.release() );
}

void KadasSvgMarkerAnnotationController::persistStyle( const QgsAnnotationItem *item ) const
{
  const auto *marker = dynamic_cast<const QgsAnnotationMarkerItem *>( item );
  if ( !marker || !marker->symbol() || marker->symbol()->symbolLayerCount() == 0 )
    return;
  const auto *sl = dynamic_cast<const QgsSvgMarkerSymbolLayer *>( marker->symbol()->symbolLayer( 0 ) );
  if ( !sl )
    return;
  settingsSvgPath->setValue( sl->path() );
  settingsSvgSize->setValue( static_cast<int>( std::round( sl->size() ) ) );
  settingsSvgFillColor->setValue( sl->fillColor() );
}
