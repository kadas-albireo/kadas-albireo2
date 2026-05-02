/***************************************************************************
    kadasmilxlayer.cpp
    ------------------
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

#include <QApplication>

#include <qgis/qgscoordinatetransform.h>

#include "kadas/gui/milx/kadasmilxclient.h"
#include "kadas/gui/milx/kadasmilxitem.h"
#include "kadas/gui/milx/kadasmilxlayer.h"

KadasMilxLayer::KadasMilxLayer( const QString &name )
  : KadasItemLayer( name, QgsCoordinateReferenceSystem( "EPSG:4326" ), layerType() )
{}

bool KadasMilxLayer::importFromMilxly( const QDomElement &milxLayerEl, int dpi, QString &errorMsg )
{
  setName( milxLayerEl.firstChildElement( "Name" ).text() );
  float symbolSize = milxLayerEl.firstChildElement( "SymbolSize" ).text().toFloat(); // This is in mm
  symbolSize = ( symbolSize * dpi ) / 25.4;                                          // mm to px
  QString crs = milxLayerEl.firstChildElement( "CoordSystemType" ).text();
  if ( crs.isEmpty() )
  {
    errorMsg = tr( "The file is corrupt" );
    return false;
  }
  QString utmZone = milxLayerEl.firstChildElement( "CoordSystemUtmZone" ).text();
  QgsCoordinateReferenceSystem srcCrs;
  if ( crs == "SwissLv03" )
  {
    srcCrs = QgsCoordinateReferenceSystem( "EPSG:21781" );
  }
  else if ( crs == "WGS84" )
  {
    srcCrs = QgsCoordinateReferenceSystem( "EPSG:4326" );
  }
  else if ( crs == "UTM" )
  {
    QString zoneLetter = utmZone.right( 1 ).toUpper();
    QString zoneNumber = utmZone.left( utmZone.length() - 1 );
    QString projZone = zoneNumber + ( zoneLetter == "S" ? " +south" : "" );
    srcCrs.createFromProj( QString( "+proj=utm +zone=%1 +datum=WGS84 +units=m +no_defs" ).arg( projZone ) );
  }
  QgsCoordinateTransform crst( srcCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), mTransformContext );

  QDomNodeList graphicEls = milxLayerEl.firstChildElement( "GraphicList" ).elementsByTagName( "MilXGraphic" );
  for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
  {
    QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();
    addItem( KadasMilxItem::fromMilx( graphicEl, crst, symbolSize ) );
  }
  return true;
}

const KadasMilxSymbolSettings &KadasMilxLayer::milxSymbolSettings() const
{
  if ( mOverrideMilxSymbolSettings )
  {
    return mMilxSymbolSettings;
  }
  else
  {
    return KadasMilxClient::globalSymbolSettings();
  }
}
