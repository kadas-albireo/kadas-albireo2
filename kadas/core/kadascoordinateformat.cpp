/***************************************************************************
    kadascoordinateformat.cpp
    -------------------------
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

#include <gdal.h>
#include <qmath.h>
#include <QRegExp>

#include <qgis/qgscoordinateformatter.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslogger.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/core/kadaslatlontoutm.h>

static QRegExp gPatDflt = QRegExp( QString( "^(-?[\\d']+\\.?\\d*)?\\s*[,;:\\s]\\s*(-?[\\d']+\\.?\\d*)?$" ) );
static QRegExp gPatDD   = QRegExp( QString( "^(-?[\\d']+\\.?\\d*)%1?\\s*[,;:\\s]\\s*(-?[\\d']+\\.?\\d*)%1?$" ).arg( QChar( 0x00B0 ) ) );
static QRegExp gPatDM   = QRegExp( QString( "^(\\d+)%1(\\d+\\.?\\d*)[%2%3%4]([NnSsEeWw])\\s*[,;:]?\\s*(\\d+)%1(\\d+\\.?\\d*)[%2%3%4]([NnSsEeWw])$" ).arg( QChar( 0x00B0 ) ).arg( QChar( 0x2032 ) ).arg( QChar( 0x02BC ) ).arg( QChar( 0x2019 ) ) );
static QRegExp gPatDMS  = QRegExp( QString( "^(\\d+)%1(\\d+)[%2%3%4](\\d+\\.?\\d*)[%5]([NnSsEeWw])\\s*[,;:]?\\s*(\\d+)%1(\\d+)[%2%3%4](\\d+\\.?\\d*)[\"%5]([NnSsEeWw])$" ).arg( QChar( 0x00B0 ) ).arg( QChar( 0x2032 ) ).arg( QChar( 0x02BC ) ).arg( QChar( 0x2019 ) ).arg( QChar( 0x2033 ) ) );
static QRegExp gPatUTM  = QRegExp( "^([\\d']+\\.?\\d*)[,\\s]\\s*([\\d']+\\.?\\d*)\\s*\\(\\w+\\s+(\\d+)([A-Za-z])\\)$" );
static QRegExp gPatUTM2 = QRegExp( "^(\\d+)\\s*([A-Za-z])\\s+([\\d']+\\.?\\d*)[,\\s]\\s*([\\d']+\\.?\\d*)$" );
static QRegExp gPatMGRS = QRegExp( "^(\\d+)\\s*(\\w)\\s*(\\w\\w)\\s*[,:;\\s]?\\s*(\\d{5})\\s*[,:;\\s]?\\s*(\\d{5})$" );


KadasCoordinateFormat::KadasCoordinateFormat()
{
  mFormat = KadasCoordinateFormat::Default;
  mEpsg = "EPSG:4326";
  mHeightUnit = QgsUnitTypes::DistanceMeters;
}

KadasCoordinateFormat *KadasCoordinateFormat::instance()
{
  static KadasCoordinateFormat instance;
  return &instance;
}

void KadasCoordinateFormat::setCoordinateDisplayFormat( Format format, const QString &epsg )
{
  mFormat = format;
  mEpsg = epsg;
  emit coordinateDisplayFormatChanged( format, epsg );
}

void KadasCoordinateFormat::setHeightDisplayUnit( QgsUnitTypes::DistanceUnit heightUnit )
{
  mHeightUnit = heightUnit;
  emit heightDisplayUnitChanged( heightUnit );
}

void KadasCoordinateFormat::getCoordinateDisplayFormat( Format &format, QString &epsg ) const
{
  format = mFormat;
  epsg = mEpsg;
}

QString KadasCoordinateFormat::getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs ) const
{
  return getDisplayString( p, sSrs, mFormat, mEpsg );
}

QString KadasCoordinateFormat::getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs, Format format, const QString &epsg )
{
  QgsCoordinateReferenceSystem destCrs( epsg );
  QgsPointXY pTrans;
  try
  {
    pTrans = QgsCoordinateTransform( sSrs, destCrs, QgsProject::instance() ).transform( p );
  }
  catch ( ... )
  {
    return "";
  }
  switch ( format )
  {
    case Default:
    {
      int prec = destCrs.mapUnits() == QgsUnitTypes::DistanceDegrees ? 4 : 0;
      return QString( "%1, %2" ).arg( pTrans.x(), 0, 'f', prec ).arg( pTrans.y(), 0, 'f', prec );
    }
    case DegMinSec:
    {
      return QgsCoordinateFormatter::format( pTrans, QgsCoordinateFormatter::FormatDegreesMinutesSeconds, 1 );
    }
    case DegMin:
    {
      return QgsCoordinateFormatter::format( pTrans, QgsCoordinateFormatter::FormatDegreesMinutes, 3 );
    }
    case DecDeg:
    {
      return QgsCoordinateFormatter::format( pTrans, QgsCoordinateFormatter::FormatDecimalDegrees, 5 );
    }
    case UTM:
    {
      KadasLatLonToUTM::UTMCoo coo = KadasLatLonToUTM::LL2UTM( pTrans );
      return QString( "%1, %2 (zone %3%4)" ).arg( coo.easting ).arg( coo.northing ).arg( coo.zoneNumber ).arg( coo.zoneLetter );
    }
    case MGRS:
    {
      KadasLatLonToUTM::UTMCoo utm = KadasLatLonToUTM::LL2UTM( pTrans );
      KadasLatLonToUTM::MGRSCoo mgrs = KadasLatLonToUTM::UTM2MGRS( utm );
      if ( mgrs.letter100kID.isEmpty() )
      {
        return QString();
      }

      return QString( "%1%2%3 %4 %5" ).arg( mgrs.zoneNumber ).arg( mgrs.zoneLetter ).arg( mgrs.letter100kID ).arg( mgrs.easting, 5, 10, QChar( '0' ) ).arg( mgrs.northing, 5, 10, QChar( '0' ) );
    }
  }
  return "";
}

double KadasCoordinateFormat::getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, QString *errMsg )
{
  return getHeightAtPos( p, crs, mHeightUnit, errMsg );
}

double KadasCoordinateFormat::getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, QgsUnitTypes::DistanceUnit unit, QString *errMsg )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayerType::RasterLayer )
  {
    if ( errMsg )
    {
      *errMsg = tr( "No heightmap is defined in the project." ), tr( "Right-click a raster layer in the layer tree and select it to be used as heightmap." );
    }
    return 0;
  }
  QString rasterFile = layer->source();
  GDALDatasetH raster = GDALOpen( rasterFile.toLocal8Bit().data(), GA_ReadOnly );
  if ( !raster )
  {
    if ( errMsg )
    {
      *errMsg = tr( "Failed to open raster file: %1" ).arg( rasterFile );
    }
    return 0;
  }

  double gtrans[6] = {};
  if ( GDALGetGeoTransform( raster, &gtrans[0] ) != CE_None )
  {
    if ( errMsg )
    {
      *errMsg = tr( "Failed to get raster geotransform" );
    }
    GDALClose( raster );
    return 0;
  }

  QString proj( GDALGetProjectionRef( raster ) );
  QgsCoordinateReferenceSystem rasterCrs = QgsCoordinateReferenceSystem::fromWkt( proj );
  if ( !rasterCrs.isValid() )
  {
    if ( errMsg )
    {
      *errMsg = tr( "Failed to get raster CRS" );
    }
    GDALClose( raster );
    return 0;
  }

  GDALRasterBandH band = GDALGetRasterBand( raster, 1 );
  if ( !raster )
  {
    if ( errMsg )
    {
      *errMsg = tr( "Failed to open raster band 0" );
    }
    GDALClose( raster );
    return 0;
  }

  // Get vertical unit
  QgsUnitTypes::DistanceUnit vertUnit = strcmp( GDALGetRasterUnitType( band ), "ft" ) == 0 ? QgsUnitTypes::DistanceFeet : QgsUnitTypes::DistanceMeters;

  // Transform geo position to raster CRS
  QgsPointXY pRaster = QgsCoordinateTransform( crs, rasterCrs, QgsProject::instance() ).transform( p );
  QgsDebugMsg( QString( "Transform %1 from %2 to %3 gives %4" ).arg( p.toString() )
               .arg( crs.authid() ).arg( rasterCrs.authid() ).arg( pRaster.toString() ) );

  // Transform raster geo position to pixel coordinates
  double row = ( -gtrans[0] * gtrans[4] + gtrans[1] * gtrans[3] - gtrans[1] * pRaster.y() + gtrans[4] * pRaster.x() ) / ( gtrans[2] * gtrans[4] - gtrans[1] * gtrans[5] );
  double col = ( -gtrans[0] * gtrans[5] + gtrans[2] * gtrans[3] - gtrans[2] * pRaster.y() + gtrans[5] * pRaster.x() ) / ( gtrans[1] * gtrans[5] - gtrans[2] * gtrans[4] );

  double pixValues[4] = {};
  if ( CE_None != GDALRasterIO( band, GF_Read,
                                qFloor( col ), qFloor( row ), 2, 2, &pixValues[0], 2, 2, GDT_Float64, 0, 0 ) )
  {
    if ( errMsg )
    {
      *errMsg = tr( "Failed to read pixel values" );
    }
    GDALClose( raster );
    return 0;
  }

  GDALClose( raster );

  // Interpolate values
  double lambdaR = row - qFloor( row );
  double lambdaC = col - qFloor( col );

  double value = ( pixValues[0] * ( 1. - lambdaC ) + pixValues[1] * lambdaC ) * ( 1. - lambdaR )
                 + ( pixValues[2] * ( 1. - lambdaC ) + pixValues[3] * lambdaC ) * ( lambdaR );
  if ( rasterCrs.mapUnits() != unit )
  {
    value *= QgsUnitTypes::fromUnitToUnitFactor( vertUnit, unit );
  }
  return value;
}

QgsPointXY KadasCoordinateFormat::parseCoordinate( const QString &text, Format format, bool &valid ) const
{
  valid =  true;
  if ( format == Format::Default )
  {
    if ( gPatDflt.exactMatch( text ) )
    {
      return QgsPointXY( gPatDflt.cap( 1 ).toDouble(), gPatDflt.cap( 2 ).toDouble() );
    }
  }
  else if ( format == Format::DecDeg )
  {
    if ( gPatDflt.exactMatch( text ) )
    {
      return QgsPointXY( gPatDflt.cap( 1 ).toDouble(), gPatDflt.cap( 2 ).toDouble() );
    }
    if ( gPatDD.exactMatch( text ) )
    {
      return QgsPointXY( gPatDD.cap( 1 ).toDouble(), gPatDD.cap( 2 ).toDouble() );
    }
  }
  else if ( format == Format::DegMin )
  {
    if ( gPatDM.exactMatch( text ) )
    {
      QString NS = "NnSs";
      if ( ( NS.indexOf( gPatDM.cap( 3 ) ) != -1 ) + ( NS.indexOf( gPatDM.cap( 6 ) ) != -1 ) == 1 )
      {
        double lon = gPatDM.cap( 1 ).toDouble() + 1. / 60. * gPatDM.cap( 2 ).toDouble();
        if ( QString( "WwSs" ).indexOf( gPatDM.cap( 3 ) ) != -1 )
        {
          lon *= -1;
        }
        double lat = gPatDM.cap( 4 ).toDouble() + 1. / 60. * gPatDM.cap( 5 ).toDouble();
        if ( QString( "WwSs" ).indexOf( gPatDM.cap( 6 ) ) != -1 )
        {
          lat *= -1;
        }
        if ( ( NS.indexOf( gPatDM.cap( 3 ) ) != -1 ) )
        {
          qSwap( lat, lon );
        }

        return QgsPointXY( lon, lat );
      }
    }
  }
  else if ( format == Format::DegMinSec )
  {
    if ( gPatDMS.exactMatch( text ) )
    {
      QString NS = "NnSs";
      if ( ( NS.indexOf( gPatDMS.cap( 4 ) ) != -1 ) + ( NS.indexOf( gPatDMS.cap( 8 ) ) != -1 ) == 1 )
      {
        double lon = gPatDMS.cap( 1 ).toDouble() + 1. / 60. * ( gPatDMS.cap( 2 ).toDouble() + 1. / 60. * gPatDMS.cap( 3 ).toDouble() );
        if ( QString( "WwSs" ).indexOf( gPatDMS.cap( 4 ) ) != -1 )
        {
          lon *= -1;
        }
        double lat = gPatDMS.cap( 5 ).toDouble() + 1. / 60. * ( gPatDMS.cap( 6 ).toDouble() + 1. / 60. * gPatDMS.cap( 7 ).toDouble() );
        if ( QString( "WwSs" ).indexOf( gPatDMS.cap( 8 ) ) != -1 )
        {
          lat *= -1;
        }
        if ( ( NS.indexOf( gPatDMS.cap( 4 ) ) != -1 ) )
        {
          qSwap( lat, lon );
        }

        return QgsPointXY( lon, lat );
      }
    }
  }
  else if ( format == Format::UTM )
  {
    if ( gPatUTM.exactMatch( text ) )
    {
      KadasLatLonToUTM::UTMCoo utm;
      utm.easting = gPatUTM.cap( 1 ).toInt();
      utm.northing = gPatUTM.cap( 2 ).toInt();
      utm.zoneNumber = gPatUTM.cap( 3 ).toInt();
      utm.zoneLetter = gPatUTM.cap( 4 );
      return KadasLatLonToUTM::UTM2LL( utm, valid );
    }
    else if ( gPatUTM2.exactMatch( text ) )
    {
      KadasLatLonToUTM::UTMCoo utm;
      utm.easting = gPatUTM2.cap( 3 ).toInt();
      utm.northing = gPatUTM2.cap( 4 ).toInt();
      utm.zoneNumber = gPatUTM2.cap( 1 ).toInt();
      utm.zoneLetter = gPatUTM2.cap( 2 );
      return KadasLatLonToUTM::UTM2LL( utm, valid );
    }
  }
  else if ( format == Format::MGRS )
  {
    if ( gPatMGRS.exactMatch( text ) )
    {
      KadasLatLonToUTM::MGRSCoo mgrs;
      mgrs.easting = gPatMGRS.cap( 4 ).toInt();
      mgrs.northing = gPatMGRS.cap( 5 ).toInt();
      mgrs.zoneNumber = gPatMGRS.cap( 1 ).toInt();
      mgrs.zoneLetter = gPatMGRS.cap( 2 );
      mgrs.letter100kID = gPatMGRS.cap( 3 );
      KadasLatLonToUTM::UTMCoo utm = KadasLatLonToUTM::MGRS2UTM( mgrs, valid );
      if ( valid )
      {
        return KadasLatLonToUTM::UTM2LL( utm, valid );
      }
    }
  }
  valid = false;
  return QgsPointXY();
}
