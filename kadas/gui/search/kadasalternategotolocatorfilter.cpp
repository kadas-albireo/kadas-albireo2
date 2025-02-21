/***************************************************************************
                        KadasAlternateGotoLocatorFilters.cpp
                        ----------------------------
   begin                : May 2017
   copyright            : (C) 2017 by Nyall Dawson
   email                : nyall dot dawson at gmail dot com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgsfeedback.h>
#include <qgsmapcanvas.h>
#include <qgscoordinateutils.h>
#include <qgscoordinatereferencesystemutils.h>

#include "kadasalternategotolocatorfilter.h"
#include "kadas/core/kadaslatlontoutm.h"


KadasAlternateGotoLocatorFilter::KadasAlternateGotoLocatorFilter( QgsMapCanvas *mapCanvas, QObject *parent )
  : QgsLocatorFilter( parent )
  , mCanvas( mapCanvas )
{
  QString degChar = QString( "%1" ).arg( QChar( 0x00B0 ) );
  QString minChars = QString( "'%1%2%3" ).arg( QChar( 0x2032 ) ).arg( QChar( 0x02BC ) ).arg( QChar( 0x2019 ) );
  QString secChars = QString( "\"%1" ).arg( QChar( 0x2033 ) );

  mPatLVDD = QRegularExpression( QString( "^(-?[\\d']+,?\\d*)(%1)?\\s*[;:\\s]\\s*(-?[\\d']+,?\\d*)(%1)?.*$" ).arg( degChar ) );
  mPatLVDDalt = QRegularExpression( QString( "^(-?[\\d']+\\.?\\d*)(%1)?\\s*[,;:\\s]\\s*(-?[\\d']+\\.?\\d*)(%1)?.*$" ).arg( degChar ) );

  mPatDM = QRegularExpression( QString( "^(\\d+)%1(\\d+,?\\d*)[%2]\\s?([NnSsEeWw])\\s*[;:]?\\s*(\\d+)%1(\\d+,?\\d*)[%2]\\s?([NnSsEeWw])$" ).arg( degChar ).arg( minChars ) );
  mPatDMalt = QRegularExpression( QString( "^(\\d+)%1(\\d+\\.?\\d*)[%2]\\s?([NnSsEeWw])\\s*[,;:]?\\s*(\\d+)%1(\\d+\\.?\\d*)[%2]\\s?([NnSsEeWw])$" ).arg( degChar ).arg( minChars ) );

  mPatDMS = QRegularExpression( QString( "^(\\d+)%1(\\d+)[%2](\\d+,?\\d*)[%3]\\s?([NnSsEeWw])\\s*[;:]?\\s*(\\d+)%1(\\d+)[%2](\\d+,?\\d*)[%3]\\s?([NnSsEeWw])$" ).arg( degChar ).arg( minChars ).arg( secChars ) );
  mPatDMSalt = QRegularExpression( QString( "^(\\d+)%1(\\d+)[%2](\\d+\\.?\\d*)[%3]\\s?([NnSsEeWw])\\s*[,;:]?\\s*(\\d+)%1(\\d+)[%2](\\d+\\.?\\d*)[%3]\\s?([NnSsEeWw])$" ).arg( degChar ).arg( minChars ).arg( secChars ) );

  mPatUTM = QRegularExpression( "^([\\d']+,?\\d*)\\s+([\\d']+,?\\d*)\\s*\\(\\w+\\s+(\\d+)([A-Za-z])\\)$" );
  mPatUTMalt = QRegularExpression( "^([\\d']+\\.?\\d*)[,\\s]\\s*([\\d']+\\.?\\d*)\\s*\\(\\w+\\s+(\\d+)([A-Za-z])\\)$" );

  mPatUTM2 = QRegularExpression( "^(\\d+)\\s*([A-Za-z])\\s+([\\d']+[.,]?\\d*)[,\\s]\\s*([\\d']+[.,]?\\d*)$" );

  mPatMGRS = QRegularExpression( "^(\\d+)\\s*(\\w)\\s*(\\w\\w)\\s*[,:;\\s]?\\s*(\\d{5})\\s*[,:;\\s]?\\s*(\\d{5})$" );
}

KadasAlternateGotoLocatorFilter *KadasAlternateGotoLocatorFilter::clone() const
{
  return new KadasAlternateGotoLocatorFilter( mCanvas );
}

void KadasAlternateGotoLocatorFilter::fetchResults( const QString &string, const QgsLocatorContext &, QgsFeedback *feedback )
{
  if ( feedback->isCanceled() )
    return;

  const QgsCoordinateReferenceSystem currentCrs = mCanvas->mapSettings().destinationCrs();
  const QgsCoordinateReferenceSystem epsg4326( QStringLiteral( "EPSG:4326" ) );

  QgsLocatorResult result;
  result.filter = this;
  QVariantMap data;

  result.score = 1.0;

  QRegularExpressionMatch match;
  if ( matchOneOf( string, { mPatLVDD, mPatLVDDalt }, match ) )
  {
    // LV03, LV93 or decimal degrees
    double lon = parseNumber( match.captured( 1 ).replace( "'", "" ) );
    double lat = parseNumber( match.captured( 3 ).replace( "'", "" ) );
    bool haveDeg = !match.captured( 2 ).isEmpty() && match.captured( 4 ).isEmpty();

    // Right-padded lon to 6 digits, lat to 5 or 6 depending on value
    double pad6lon = lon * pow( 10, 5 - floor( log10( lon ) ) );
    double latfirst = floor( lat / pow( 10, floor( log10( lat ) ) ) );
    double pad6lat = lat * pow( 10, ( latfirst >= 6 ? 4 : 5 ) - floor( log10( lat ) ) );
    if ( !haveDeg && ( pad6lon >= 470000. && pad6lon <= 850000. ) && ( pad6lat >= 60000. && pad6lat <= 310000. ) )
    {
      const QgsCoordinateReferenceSystem epsg21781( QStringLiteral( "EPSG:21781" ) );
      QgsPointXY point( pad6lon, pad6lat );
      result.displayString = point.toString() + " (LV03)";
      if ( currentCrs != epsg21781 )
      {
        const QgsCoordinateTransform ct( epsg21781, currentCrs, QgsProject::instance()->transformContext() );
        try
        {
          point = ct.transform( point );
        }
        catch ( const QgsException &e )
        {
          Q_UNUSED( e )
          return;
        }
      }
      data[QStringLiteral( "point" )] = point;
      emit resultFetched( result );
    }

    // Right-padded lon and lat to 7 digits
    double pad7lon = lon * pow( 10, 6 - floor( log10( lon ) ) );
    double pad7lat = lat * pow( 10, 6 - floor( log10( lat ) ) );
    if ( !haveDeg && ( pad7lon >= 2450000. && pad7lon <= 2850000. ) && ( pad7lat >= 1050000. && pad7lat <= 1300000. ) )
    {
      {
        const QgsCoordinateReferenceSystem epsg2056( QStringLiteral( "EPSG:2056" ) );
        QgsPointXY point( pad7lon, pad7lat );
        result.displayString = point.toString() + " (LV95)";
        if ( currentCrs != epsg2056 )
        {
          const QgsCoordinateTransform ct( epsg2056, currentCrs, QgsProject::instance()->transformContext() );
          try
          {
            point = ct.transform( point );
          }
          catch ( const QgsException &e )
          {
            Q_UNUSED( e )
            return;
          }
        }
        data[QStringLiteral( "point" )] = point;
        emit resultFetched( result );
      }
    }
  }
  /*
  else if ( matchOneOf( string, {mPatDM, mPatDMalt}, match ) )
  {
    QString NS = "NnSs";
    if ( ( NS.indexOf( match.captured( 3 ) ) != -1 ) + ( NS.indexOf( match.captured( 6 ) ) != -1 ) == 1 )
    {
      double lon = parseNumber( match.captured( 1 ) ) + 1. / 60. * parseNumber( match.captured( 2 ) );
      if ( QString( "WwSs" ).indexOf( match.captured( 3 ) ) != -1 )
      {
        lon *= -1;
      }
      double lat = parseNumber( match.captured( 4 ) ) + 1. / 60. * parseNumber( match.captured( 5 ) );
      if ( QString( "WwSs" ).indexOf( match.captured( 6 ) ) != -1 )
      {
        lat *= -1;
      }
      if ( ( NS.indexOf( match.captured( 3 ) ) != -1 ) )
      {
        qSwap( lat, lon );
      }

      searchResult.crs = "EPSG:4326";
      searchResult.pos = QgsPointXY( lon, lat );
      searchResult.text = QgsCoordinateFormatter::format( searchResult.pos, QgsCoordinateFormatter::FormatDegreesMinutesSeconds, 2 );
      emit searchResultFound( searchResult );
    }
  }
*/
  /*
  else if ( matchOneOf( string, {mPatDMS, mPatDMSalt}, match ) )
  {
    QString NS = "NnSs";
    if ( ( NS.indexOf( match.captured( 4 ) ) != -1 ) + ( NS.indexOf( match.captured( 8 ) ) != -1 ) == 1 )
    {
      double lon = parseNumber( match.captured( 1 ) ) + 1. / 60. * ( parseNumber( match.captured( 2 ) ) + 1. / 60. * parseNumber( match.captured( 3 ) ) );
      if ( QString( "WwSs" ).indexOf( match.captured( 4 ) ) != -1 )
      {
        lon *= -1;
      }
      double lat = parseNumber( match.captured( 5 ) ) + 1. / 60. * ( parseNumber( match.captured( 6 ) ) + 1. / 60. * parseNumber( match.captured( 7 ) ) );
      if ( QString( "WwSs" ).indexOf( match.captured( 8 ) ) != -1 )
      {
        lat *= -1;
      }
      if ( ( NS.indexOf( match.captured( 4 ) ) != -1 ) )
      {
        qSwap( lat, lon );
      }

      searchResult.crs = "EPSG:4326";
      searchResult.pos = QgsPointXY( lon, lat );
      searchResult.text = QgsCoordinateFormatter::format( searchResult.pos, QgsCoordinateFormatter::FormatDegreesMinutesSeconds, 2 );
      emit searchResultFound( searchResult );
    }
  }
*/
  else if ( matchOneOf( string, { mPatUTM, mPatUTMalt }, match ) )
  {
    KadasLatLonToUTM::UTMCoo utm;
    utm.easting = match.captured( 1 ).replace( "'", "" ).toInt();
    utm.northing = match.captured( 2 ).replace( "'", "" ).toInt();
    utm.zoneNumber = match.captured( 3 ).toInt();
    utm.zoneLetter = match.captured( 4 );
    bool ok = false;
    QgsPointXY point = KadasLatLonToUTM::UTM2LL( utm, ok );
    if ( ok )
    {
      result.displayString = QString( "%1, %2 (%3 %4%5)" )
                               .arg( utm.easting )
                               .arg( utm.northing )
                               .arg( tr( "zone" ) )
                               .arg( utm.zoneNumber )
                               .arg( utm.zoneLetter );
      if ( currentCrs != epsg4326 )
      {
        const QgsCoordinateTransform ct( epsg4326, currentCrs, QgsProject::instance()->transformContext() );
        try
        {
          point = ct.transform( point );
        }
        catch ( const QgsException &e )
        {
          Q_UNUSED( e )
          return;
        }
      }
      data[QStringLiteral( "point" )] = point;
      emit resultFetched( result );
    }
  }
  else if ( matchOneOf( string, { mPatUTM2 }, match ) )
  {
    KadasLatLonToUTM::UTMCoo utm;
    utm.easting = match.captured( 3 ).replace( "'", "" ).toInt();
    utm.northing = match.captured( 4 ).replace( "'", "" ).toInt();
    utm.zoneNumber = match.captured( 1 ).toInt();
    utm.zoneLetter = match.captured( 2 ).toUpper();
    bool ok = false;
    QgsPointXY point = KadasLatLonToUTM::UTM2LL( utm, ok );
    if ( ok )
    {
      result.displayString = QString( "%1, %2 (%3 %4%5)" )
                               .arg( utm.easting )
                               .arg( utm.northing )
                               .arg( tr( "zone" ) )
                               .arg( utm.zoneNumber )
                               .arg( utm.zoneLetter );
      if ( currentCrs != epsg4326 )
      {
        const QgsCoordinateTransform ct( epsg4326, currentCrs, QgsProject::instance()->transformContext() );
        try
        {
          point = ct.transform( point );
        }
        catch ( const QgsException &e )
        {
          Q_UNUSED( e )
          return;
        }
      }
      data[QStringLiteral( "point" )] = point;
      emit resultFetched( result );
    }
  }
  else if ( matchOneOf( string, { mPatMGRS }, match ) )
  {
    KadasLatLonToUTM::MGRSCoo mgrs;
    mgrs.easting = match.captured( 4 ).replace( "'", "" ).toInt();
    mgrs.northing = match.captured( 5 ).replace( "'", "" ).toInt();
    mgrs.zoneNumber = match.captured( 1 ).toInt();
    mgrs.zoneLetter = match.captured( 2 ).toUpper();
    mgrs.letter100kID = match.captured( 3 ).toUpper();
    bool ok = false;
    KadasLatLonToUTM::UTMCoo utm = KadasLatLonToUTM::MGRS2UTM( mgrs, ok );
    if ( ok )
    {
      QgsPointXY point = KadasLatLonToUTM::UTM2LL( utm, ok );
      if ( ok )
      {
        result.displayString = QString( "%1%2%3 %4 %5" )
                                 .arg( mgrs.zoneNumber )
                                 .arg( mgrs.zoneLetter )
                                 .arg( mgrs.letter100kID )
                                 .arg( mgrs.easting )
                                 .arg( mgrs.northing );
        if ( currentCrs != epsg4326 )
        {
          const QgsCoordinateTransform ct( epsg4326, currentCrs, QgsProject::instance()->transformContext() );
          try
          {
            point = ct.transform( point );
          }
          catch ( const QgsException &e )
          {
            Q_UNUSED( e )
            return;
          }
        }
        data[QStringLiteral( "point" )] = point;
        emit resultFetched( result );
      }
    }
  }
}

void KadasAlternateGotoLocatorFilter::triggerResult( const QgsLocatorResult &result )
{
  QgsMapCanvas *mapCanvas = mCanvas;

  QVariantMap data = result.userData().toMap();
  const QgsPointXY point = data[QStringLiteral( "point" )].value<QgsPointXY>();
  mapCanvas->setCenter( point );
  if ( data.contains( QStringLiteral( "scale" ) ) )
  {
    mapCanvas->zoomScale( data[QStringLiteral( "scale" )].toDouble() );
  }
  else
  {
    mapCanvas->refresh();
  }

  mapCanvas->flashGeometries( QList<QgsGeometry>() << QgsGeometry::fromPointXY( point ) );
}

double KadasAlternateGotoLocatorFilter::parseNumber( const QString &string ) const
{
  QChar sep = QLocale().decimalPoint();
  QTextStream( stdout ) << string << " : " << sep << " ::" << QLocale().toDouble( QString( string ).replace( '.', sep ).replace( ',', sep ) ) << Qt::endl;
  return QLocale().toDouble( QString( string ).replace( '.', sep ).replace( ',', sep ) );
  return QString( string ).replace( '.', sep ).replace( ',', sep ).toDouble();
}

bool KadasAlternateGotoLocatorFilter::matchOneOf( const QString &str, const QVector<QRegularExpression> &patterns, QRegularExpressionMatch &match ) const
{
  for ( const QRegularExpression &pat : patterns )
  {
    match = pat.match( str );
    if ( match.hasMatch() )
    {
      return true;
    }
  }
  return false;
}
