/***************************************************************************
    kadaspinsearchprovider.cpp
    --------------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QGraphicsItem>

#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgsmapcanvas.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/search/kadaspinsearchprovider.h>


KadasPinSearchProvider::KadasPinSearchProvider( QgsMapCanvas *mapCanvas )
  : QgsLocatorFilter()
  , mMapCanvas( mapCanvas )
{
}

QgsLocatorFilter *KadasPinSearchProvider::clone() const
{
  return new KadasPinSearchProvider( mMapCanvas );
}

void KadasPinSearchProvider::fetchResults( const QString &string, const QgsLocatorContext &context, QgsFeedback *feedback )
{
  for ( QgsMapLayer *layer : mMapCanvas->layers() )
  {
    KadasItemLayer *itemLayer = dynamic_cast<KadasItemLayer *>( layer );
    if ( !itemLayer )
    {
      return;
    }
    for ( KadasMapItem *item : itemLayer->items() )
    {
      const KadasSymbolItem *symbolItem = dynamic_cast<KadasSymbolItem *>( item );
      if ( !symbolItem )
      {
        continue;
      }
      if ( symbolItem->name().contains( string, Qt::CaseInsensitive ) ||
           symbolItem->remarks().contains( string, Qt::CaseInsensitive ) )
      {
        QgsLocatorResult result;
        QVariantMap resultData;

        //searchResult.zoomScale = 1000;
        result.displayString = tr( "Pin %1" ).arg( symbolItem->name() );
        resultData[QStringLiteral( "pos" )] = QgsPointXY( symbolItem->constState()->pos );
        resultData[QStringLiteral( "crs" )] = symbolItem->crs().authid();

        result.setUserData( resultData );
        emit resultFetched( result );
      }
    }
  }
}

void KadasPinSearchProvider::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().value<QVariantMap>();
  QgsPointXY pos = data.value( QStringLiteral( "pos" ) ).value<QgsPointXY>();
  QString crsIs = data.value( QStringLiteral( "crs" ) ).toString();

  QgsPointXY itemPos = QgsCoordinateTransform(
                         QgsCoordinateReferenceSystem( crsIs ),
                         mMapCanvas->mapSettings().destinationCrs(),
                         QgsProject::instance()
                       ).transform( pos );

  mMapCanvas->setCenter( itemPos );
}
