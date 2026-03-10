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

#include <QRegularExpression>

#include <qgis/qgscoordinateformatter.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include "kadas/core/kadas.h"
#include "kadas/core/kadascoordinateformat.h"
#include "kadas/core/kadascoordinateutils.h"
#include "kadas/core/kadaslatlontoutm.h"

static const QRegularExpression gPatDflt( QString( "^(-?[\\d']+\\.?\\d*)?\\s*[,;:\\s]\\s*(-?[\\d']+\\.?\\d*)?$" ) );
static const QRegularExpression gPatDD( QString( "^(-?[\\d']+\\.?\\d*)%1?\\s*[,;:\\s]\\s*(-?[\\d']+\\.?\\d*)%1?$" ).arg( QChar( 0x00B0 ) ) );
static const QRegularExpression gPatDM(
  QString( "^(\\d+)%1(\\d+\\.?\\d*)[%2%3%4]([NnSsEeWw])\\s*[,;:]?\\s*(\\d+)%1(\\d+\\.?\\d*)[%2%3%4]([NnSsEeWw])$" ).arg( QChar( 0x00B0 ) ).arg( QChar( 0x2032 ) ).arg( QChar( 0x02BC ) ).arg( QChar( 0x2019 ) )
);
static const QRegularExpression gPatDMS( QString( "^(\\d+)%1(\\d+)[%2%3%4](\\d+\\.?\\d*)[%5]([NnSsEeWw])\\s*[,;:]?\\s*(\\d+)%1(\\d+)[%2%3%4](\\d+\\.?\\d*)[\"%5]([NnSsEeWw])$" )
                                           .arg( QChar( 0x00B0 ) )
                                           .arg( QChar( 0x2032 ) )
                                           .arg( QChar( 0x02BC ) )
                                           .arg( QChar( 0x2019 ) )
                                           .arg( QChar( 0x2033 ) ) );
static const QRegularExpression gPatUTM( "^([\\d']+\\.?\\d*)[,\\s]\\s*([\\d']+\\.?\\d*)\\s*\\(\\w+\\s+(\\d+)([A-Za-z])\\)$" );
static const QRegularExpression gPatUTM2( "^(\\d+)\\s*([A-Za-z])\\s+([\\d']+\\.?\\d*)[,\\s]\\s*([\\d']+\\.?\\d*)$" );
static const QRegularExpression gPatMGRS( "^(\\d+)\\s*(\\w)\\s*(\\w\\w)\\s*[,:;\\s]?\\s*(\\d{5})\\s*[,:;\\s]?\\s*(\\d{5})$" );


KadasCoordinateFormat::KadasCoordinateFormat()
{
  mFormat = KadasCoordinateFormat::Format::Default;
  mEpsg = "EPSG:4326";
  mHeightUnit = Qgis::DistanceUnit::Meters;
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

void KadasCoordinateFormat::setHeightDisplayUnit( Qgis::DistanceUnit heightUnit )
{
  mHeightUnit = heightUnit;
  emit heightDisplayUnitChanged( heightUnit );
}

KadasCoordinateFormat::Format KadasCoordinateFormat::getCoordinateDisplayFormat() const
{
  return mFormat;
}

const QString &KadasCoordinateFormat::getCoordinateDisplayCrs() const
{
  return mEpsg;
}

QString KadasCoordinateFormat::getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs, bool translateDirectionSuffixes ) const
{
  return KadasCoordinateFormat::getDisplayString( p, sSrs, mFormat, mEpsg, translateDirectionSuffixes );
}

QString KadasCoordinateFormat::getDisplayString( const QgsPointXY &p, const QgsCoordinateReferenceSystem &sSrs, Format format, const QString &epsg, bool translateDirectionSuffixes )
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
    case Format::Default:
    {
      int prec = destCrs.mapUnits() == Qgis::DistanceUnit::Degrees ? 4 : 0;
      return QString( "%1, %2" ).arg( pTrans.x(), 0, 'f', prec ).arg( pTrans.y(), 0, 'f', prec );
    }
    case Format::DegMinSec:
    {
      QgsCoordinateFormatter::FormatFlags formatFlags = QgsCoordinateFormatter::FormatFlag::FlagDegreesUseStringSuffix;
      if ( !translateDirectionSuffixes )
        formatFlags |= QgsCoordinateFormatter::FormatFlag::FlagDegreesUseUntranslatedStringSuffix;

      return QgsCoordinateFormatter::format( pTrans, QgsCoordinateFormatter::FormatDegreesMinutesSeconds, 1, formatFlags );
    }
    case Format::DegMin:
    {
      QgsCoordinateFormatter::FormatFlags formatFlags = QgsCoordinateFormatter::FormatFlag::FlagDegreesUseStringSuffix;
      if ( !translateDirectionSuffixes )
        formatFlags |= QgsCoordinateFormatter::FormatFlag::FlagDegreesUseUntranslatedStringSuffix;

      return QgsCoordinateFormatter::format( pTrans, QgsCoordinateFormatter::FormatDegreesMinutes, 3, formatFlags );
    }
    case Format::DecDeg:
    {
      QgsCoordinateFormatter::FormatFlags formatFlags = QgsCoordinateFormatter::FormatFlag::FlagDegreesUseStringSuffix;
      if ( !translateDirectionSuffixes )
        formatFlags |= QgsCoordinateFormatter::FormatFlag::FlagDegreesUseUntranslatedStringSuffix;

      return QgsCoordinateFormatter::format( pTrans, QgsCoordinateFormatter::FormatDecimalDegrees, 5, formatFlags );
    }
    case Format::UTM:
    {
      KadasLatLonToUTM::UTMCoo coo = KadasLatLonToUTM::LL2UTM( pTrans );
      return QString( "%1, %2 (zone %3%4)" ).arg( coo.easting ).arg( coo.northing ).arg( coo.zoneNumber ).arg( coo.zoneLetter );
    }
    case Format::MGRS:
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
  return KadasCoordinateUtils::getHeightAtPos( p, crs, mHeightUnit, errMsg );
}

double KadasCoordinateFormat::getHeightAtPos( const QgsPointXY &p, const QgsCoordinateReferenceSystem &crs, Qgis::DistanceUnit unit, QString *errMsg )
{
  return KadasCoordinateUtils::getHeightAtPos( p, crs, unit, errMsg );
}

QgsPointXY KadasCoordinateFormat::parseCoordinate( const QString &text, Format format, bool &valid ) const
{
  valid = true;
  QRegularExpressionMatch match;
  if ( format == Format::Default )
  {
    match = gPatDflt.match( text );
    if ( match.hasMatch() )
    {
      return QgsPointXY( match.captured( 1 ).toDouble(), match.captured( 2 ).toDouble() );
    }
  }
  else if ( format == Format::DecDeg )
  {
    match = gPatDflt.match( text );
    if ( match.hasMatch() )
    {
      return QgsPointXY( match.captured( 1 ).toDouble(), match.captured( 2 ).toDouble() );
    }
    match = gPatDD.match( text );
    if ( match.hasMatch() )
    {
      return QgsPointXY( match.captured( 1 ).toDouble(), match.captured( 2 ).toDouble() );
    }
  }
  else if ( format == Format::DegMin )
  {
    match = gPatDM.match( text );
    if ( match.hasMatch() )
    {
      QString NS = "NnSs";
      if ( ( NS.indexOf( match.captured( 3 ) ) != -1 ) + ( NS.indexOf( match.captured( 6 ) ) != -1 ) == 1 )
      {
        double lon = match.captured( 1 ).toDouble() + 1. / 60. * match.captured( 2 ).toDouble();
        if ( QString( "WwSs" ).indexOf( match.captured( 3 ) ) != -1 )
        {
          lon *= -1;
        }
        double lat = match.captured( 4 ).toDouble() + 1. / 60. * match.captured( 5 ).toDouble();
        if ( QString( "WwSs" ).indexOf( match.captured( 6 ) ) != -1 )
        {
          lat *= -1;
        }
        if ( ( NS.indexOf( match.captured( 3 ) ) != -1 ) )
        {
          qSwap( lat, lon );
        }

        return QgsPointXY( lon, lat );
      }
    }
  }
  else if ( format == Format::DegMinSec )
  {
    match = gPatDMS.match( text );
    if ( match.hasMatch() )
    {
      QString NS = "NnSs";
      if ( ( NS.indexOf( match.captured( 4 ) ) != -1 ) + ( NS.indexOf( match.captured( 8 ) ) != -1 ) == 1 )
      {
        double lon = match.captured( 1 ).toDouble() + 1. / 60. * ( match.captured( 2 ).toDouble() + 1. / 60. * match.captured( 3 ).toDouble() );
        if ( QString( "WwSs" ).indexOf( match.captured( 4 ) ) != -1 )
        {
          lon *= -1;
        }
        double lat = match.captured( 5 ).toDouble() + 1. / 60. * ( match.captured( 6 ).toDouble() + 1. / 60. * match.captured( 7 ).toDouble() );
        if ( QString( "WwSs" ).indexOf( match.captured( 8 ) ) != -1 )
        {
          lat *= -1;
        }
        if ( ( NS.indexOf( match.captured( 4 ) ) != -1 ) )
        {
          qSwap( lat, lon );
        }

        return QgsPointXY( lon, lat );
      }
    }
  }
  else if ( format == Format::UTM )
  {
    match = gPatUTM.match( text );
    if ( match.hasMatch() )
    {
      KadasLatLonToUTM::UTMCoo utm;
      utm.easting = match.captured( 1 ).toInt();
      utm.northing = match.captured( 2 ).toInt();
      utm.zoneNumber = match.captured( 3 ).toInt();
      utm.zoneLetter = match.captured( 4 );
      return KadasLatLonToUTM::UTM2LL( utm, valid );
    }
    match = gPatUTM2.match( text );
    if ( match.hasMatch() )
    {
      KadasLatLonToUTM::UTMCoo utm;
      utm.easting = match.captured( 3 ).toInt();
      utm.northing = match.captured( 4 ).toInt();
      utm.zoneNumber = match.captured( 1 ).toInt();
      utm.zoneLetter = match.captured( 2 );
      return KadasLatLonToUTM::UTM2LL( utm, valid );
    }
  }
  else if ( format == Format::MGRS )
  {
    match = gPatMGRS.match( text );
    if ( match.hasMatch() )
    {
      KadasLatLonToUTM::MGRSCoo mgrs;
      mgrs.easting = match.captured( 4 ).toInt();
      mgrs.northing = match.captured( 5 ).toInt();
      mgrs.zoneNumber = match.captured( 1 ).toInt();
      mgrs.zoneLetter = match.captured( 2 );
      mgrs.letter100kID = match.captured( 3 );
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
