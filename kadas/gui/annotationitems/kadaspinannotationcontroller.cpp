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

#include <QObject>
#include <QFile>

#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgssymbollayer.h>

#include "kadas/gui/annotationitems/kadasannotationstyleeditor.h"
#include "kadas/gui/annotationitems/kadasannotationzindex.h"
#include "kadas/gui/annotationitems/kadaspinannotationcontroller.h"
#include "kadas/gui/annotationitems/kadaspinannotationitem.h"


namespace
{
  // Walk the symbol layer list and rewrite any SvgMarker layer whose path
  // points at a Qt resource (\":/...\") to an inline base64-encoded payload.
  // This makes the saved QGIS shadow self-contained: vanilla QGIS can't
  // resolve Kadas qrc paths, so without this the shadow would render as a
  // missing-SVG placeholder. \\see QgsSymbolLayerUtils::svgSymbolNameToPath.
  void embedQrcSvgPaths( QgsMarkerSymbol *symbol )
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
        continue; // not a qrc path; leave it
      QFile f( path );
      if ( !f.open( QIODevice::ReadOnly ) )
        continue;
      const QByteArray data = f.readAll();
      svg->setPath( QStringLiteral( "base64:" ) + QString::fromLatin1( data.toBase64() ) );
    }
  }
} // namespace


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
