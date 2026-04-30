/***************************************************************************
    kadasannotationlayerhelpers.cpp
    -------------------------------
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

#include <QSet>

#include <qgis/qgsannotationlayer.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"


namespace
{
  const QString KEY_PREFIX = QStringLiteral( "kadas:item-meta:" );
  const QString FIELD_EDITOR = QStringLiteral( "editor" );
  const QString FIELD_TOOLTIP = QStringLiteral( "tooltip" );

  QString makeKey( const QString &itemId, const QString &field )
  {
    return KEY_PREFIX + itemId + QLatin1Char( ':' ) + field;
  }

  QString readField( const QgsAnnotationLayer *layer, const QString &itemId, const QString &field )
  {
    if ( !layer || itemId.isEmpty() )
      return QString();
    return layer->customProperty( makeKey( itemId, field ) ).toString();
  }

  void writeField( QgsAnnotationLayer *layer, const QString &itemId, const QString &field, const QString &value )
  {
    if ( !layer || itemId.isEmpty() )
      return;
    const QString key = makeKey( itemId, field );
    if ( value.isEmpty() )
      layer->removeCustomProperty( key );
    else
      layer->setCustomProperty( key, value );
  }
} // namespace


QString KadasAnnotationLayerHelpers::editorName( const QgsAnnotationLayer *layer, const QString &itemId )
{
  return readField( layer, itemId, FIELD_EDITOR );
}

void KadasAnnotationLayerHelpers::setEditorName( QgsAnnotationLayer *layer, const QString &itemId, const QString &name )
{
  writeField( layer, itemId, FIELD_EDITOR, name );
}

QString KadasAnnotationLayerHelpers::tooltip( const QgsAnnotationLayer *layer, const QString &itemId )
{
  return readField( layer, itemId, FIELD_TOOLTIP );
}

void KadasAnnotationLayerHelpers::setTooltip( QgsAnnotationLayer *layer, const QString &itemId, const QString &tooltip )
{
  writeField( layer, itemId, FIELD_TOOLTIP, tooltip );
}

void KadasAnnotationLayerHelpers::clearMetadata( QgsAnnotationLayer *layer, const QString &itemId )
{
  if ( !layer || itemId.isEmpty() )
    return;
  const QString prefix = KEY_PREFIX + itemId + QLatin1Char( ':' );
  const QStringList keys = layer->customPropertyKeys();
  for ( const QString &key : keys )
  {
    if ( key.startsWith( prefix ) )
      layer->removeCustomProperty( key );
  }
}

QStringList KadasAnnotationLayerHelpers::itemsWithMetadata( const QgsAnnotationLayer *layer )
{
  if ( !layer )
    return {};
  QStringList result;
  QSet<QString> seen;
  const QStringList keys = layer->customPropertyKeys();
  for ( const QString &key : keys )
  {
    if ( !key.startsWith( KEY_PREFIX ) )
      continue;
    const QString remainder = key.mid( KEY_PREFIX.size() );
    const int sep = remainder.indexOf( QLatin1Char( ':' ) );
    if ( sep <= 0 )
      continue;
    const QString itemId = remainder.left( sep );
    if ( !seen.contains( itemId ) )
    {
      seen.insert( itemId );
      result.append( itemId );
    }
  }
  return result;
}
