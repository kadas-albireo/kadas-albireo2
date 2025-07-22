/***************************************************************************
                        kadasalternategotofilter.cpp
                        ----------------------------
   begin                : February 2025
   copyright            : (C) 2025 by Denis Rouzaud
   email                : denis@opengis.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgscoordinateutils.h>
#include <qgis/qgscoordinatereferencesystemutils.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgsmarkersymbol.h>

#include "kadasalternategotolocatorfilter.h"
#include "kadas/core/kadaslatlontoutm.h"


KadasAlternateGotoLocatorFilter::KadasAlternateGotoLocatorFilter( QgsMapCanvas *mapCanvas, QObject *parent )
  : QgsLocatorFilter( parent )
  , mCanvas( mapCanvas )
{
}

KadasAlternateGotoLocatorFilter *KadasAlternateGotoLocatorFilter::clone() const
{
  return new KadasAlternateGotoLocatorFilter( mCanvas );
}

void KadasAlternateGotoLocatorFilter::fetchResults( const QString &string, const QgsLocatorContext &, QgsFeedback *feedback )
{
  if ( feedback->isCanceled() )
    return;

  if ( string.length() < 3 )
    return;

  //QString degChar = QStringLiteral( "%1" ).arg( QChar( 0x00B0 ) );
  //QString minChars = QString( "'%1%2%3" ).arg( QChar( 0x2032 ) ).arg( QChar( 0x02BC ) ).arg( QChar( 0x2019 ) );
  //QString secChars = QString( "\"%1" ).arg( QChar( 0x2033 ) );
  //mPatLVDD = QRegularExpression( QString( R"(^(-?[\d']+,?\d*)(%1)?\s*[;:\s]\s*(-?[\d']+,?\d*)(%1)?.*$)" ).arg( degChar ) );
  //mPatLVDDalt = QRegularExpression( QString( R"(^(-?[\d']+\.?\d*)(%1)?\s*[,;:\s]\s*(-?[\d']+\.?\d*)(%1)?.*$)" ).arg( degChar ) );
  // thread_local QRegularExpression mPatDM( QString( R"(^(\d+)%1(\d+,?\d*)[%2]\s?([NnSsEeWw])\s*[;:]?\s*(\d+)%1(\d+,?\d*)[%2]\s?([NnSsEeWw])$)" ).arg( degChar ).arg( minChars ) );
  // thread_local QRegularExpression mPatDMalt( QString( R"(^(\d+)%1(\d+\.?\d*)[%2]\s?([NnSsEeWw])\s*[,;:]?\s*(\d+)%1(\d+\.?\d*)[%2]\s?([NnSsEeWw])$)" ).arg( degChar ).arg( minChars ) );
  // mPatDMS = QRegularExpression( QString( R"(^(\d+)%1(\d+)[%2](\d+,?\d*)[%3]\s?([NnSsEeWw])\s*[;:]?\s*(\d+)%1(\d+)[%2](\d+,?\d*)[%3]\s?([NnSsEeWw])$)" ).arg( degChar ).arg( minChars ).arg( secChars ) );
  // mPatDMSalt = QRegularExpression( QString( R"(^(\d+)%1(\d+)[%2](\d+\.?\d*)[%3]\s?([NnSsEeWw])\s*[,;:]?\s*(\d+)%1(\d+)[%2](\d+\.?\d*)[%3]\s?([NnSsEeWw])$)" ).arg( degChar ).arg( minChars ).arg( secChars ) );

  const thread_local QRegularExpression mPatUTM( R"(^([\d']+),\d*\s+([\d']+),\d*\s*\(\w+\s+(\d+)([A-Za-z])\)$)" );
  const thread_local QRegularExpression mPatUTMalt( R"(^([\d']+)\.?\d*[,\s]\s*([\d']+)\.?\d*\s*\(\w+\s+(\d+)([A-Za-z])\)$)" );
  const thread_local QRegularExpression mPatUTM2( R"(^(\d+)\s*([A-Za-z])\s+([\d']+[.,]?\d*)[,\s]\s*([\d']+[.,]?\d*)$)" );
  const thread_local QRegularExpression mPatMGRS( R"(^(\d+)\s*(\w)\s*(\w\w)\s*[,:;\s]?\s*(\d{5})\s*[,:;\s]?\s*(\d{5})$)" );

  const QLocale locale;


  // Mostly copied from QgsGotoLocatorFilter::fetchResults
  // with additions for 2056 + 21781

  bool firstOk = false;
  bool secondOk = false;
  double firstNumber = 0.0;
  double secondNumber = 0.0;
  bool posIsWgs84 = false;

  // Coordinates such as 106.8468,-6.3804
  // Accept decimal numbers, possibly with degree symbol (°) after each number
  thread_local QRegularExpression separatorRx1(
    QStringLiteral(
      R"(^([0-9\-\%1\%2]*)(?:\s*°)?[\s%3]*([0-9\-\%1\%2]*)(?:\s*°)?$)"
    )
      .arg(
        locale.decimalPoint(),
        locale.groupSeparator(),
        locale.decimalPoint() != ',' && locale.groupSeparator() != ',' ? QStringLiteral( "\\," ) : QString()
      )
  );
  QRegularExpressionMatch match = separatorRx1.match( string.trimmed() );
  if ( match.hasMatch() )
  {
    firstNumber = locale.toDouble( match.captured( 1 ), &firstOk );
    secondNumber = locale.toDouble( match.captured( 2 ), &secondOk );
  }

  if ( !match.hasMatch() || !firstOk || !secondOk )
  {
    // Digit detection using user locale failed, use default C decimal separators
    thread_local QRegularExpression separatorRx2( QStringLiteral( R"(^([0-9\-\.]*)(?:\s*°)?[\s\,]*([0-9\-\.]*)(?:\s*°)?$)" ) );
    match = separatorRx2.match( string.trimmed() );
    if ( match.hasMatch() )
    {
      firstNumber = match.captured( 1 ).toDouble( &firstOk );
      secondNumber = match.captured( 2 ).toDouble( &secondOk );
    }
  }

  if ( !match.hasMatch() )
  {
    // Check if the string is a pair of decimal degrees with [N,S,E,W] suffixes
    thread_local QRegularExpression separatorRx3( QStringLiteral( R"(^\s*([-]?\d{1,3}(?:[\.\%1]\d+)?(?:\s*°)?\s*[NSEWnsew])[\s\,]*([-]?\d{1,3}(?:[\.\%1]\d+)?(?:\s*°)?\s*[NSEWnsew])\s*$)" )
                                                    .arg( locale.decimalPoint() ) );
    match = separatorRx3.match( string.trimmed() );
    if ( match.hasMatch() )
    {
      posIsWgs84 = true;
      bool isEasting = false;
      firstNumber = QgsCoordinateUtils::degreeToDecimal( match.captured( 1 ), &firstOk, &isEasting );
      secondNumber = QgsCoordinateUtils::degreeToDecimal( match.captured( 2 ), &secondOk );
      // normalize to northing (i.e. Y) first
      if ( isEasting )
        std::swap( firstNumber, secondNumber );
    }
  }

  if ( !match.hasMatch() )
  {
    // Check if the string is a pair of degree minute second
    thread_local QString dmsRx = QStringLiteral( R"(\d{1,3}(?:[^0-9.]+[0-5]?\d)?[^0-9.]+[0-5]?\d(?:[\.\%1]\d+)?)" ).arg( locale.decimalPoint() );
    thread_local QRegularExpression separatorRx4( QStringLiteral(
                                                    "^("
                                                    R"((\s*%1[^0-9.,]*[-+NSEWnsew]?)[,\s]+(%1[^0-9.,]*[-+NSEWnsew]?))"
                                                    ")|("
                                                    R"(((?:([-+NSEWnsew])\s*)%1[^0-9.,]*)[,\s]+((?:([-+NSEWnsew])\s*)%1[^0-9.,]*))"
                                                    ")$"
    )
                                                    .arg( dmsRx ) );
    match = separatorRx4.match( string.trimmed() );
    if ( match.hasMatch() )
    {
      qDebug() << match.captured( 1 ) << match.captured( 2 ) << match.captured( 3 ) << match.captured( 4 ) << match.captured( 5 ) << match.captured( 6 ) << match.captured( 7 ) << match.captured( 8 );

      posIsWgs84 = true;
      bool isEasting = false;
      if ( !match.captured( 1 ).isEmpty() )
      {
        firstNumber = QgsCoordinateUtils::dmsToDecimal( match.captured( 2 ), &firstOk, &isEasting );
        secondNumber = QgsCoordinateUtils::dmsToDecimal( match.captured( 3 ), &secondOk );
      }
      else
      {
        firstNumber = QgsCoordinateUtils::dmsToDecimal( match.captured( 5 ), &firstOk, &isEasting );
        secondNumber = QgsCoordinateUtils::dmsToDecimal( match.captured( 7 ), &secondOk );
      }
      // normalize to northing (i.e. Y) first
      if ( isEasting )
        std::swap( firstNumber, secondNumber );
    }
  }

  const QgsCoordinateReferenceSystem currentCrs = mCanvas->mapSettings().destinationCrs();
  const QgsCoordinateReferenceSystem wgs84Crs( QStringLiteral( "EPSG:4326" ) );

  if ( firstOk && secondOk )
  {
    const bool withinWgs84 = wgs84Crs.bounds().contains( secondNumber, firstNumber );

    QVariantMap data;

    QList<std::pair<QString, QgsCoordinateReferenceSystem>> coordSystems = {
      { QStringLiteral( "LV03" ), QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:21781" ) ) },
      { QStringLiteral( "LV95" ), QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:2056" ) ) },
    };
    bool currentCrsFound = false;
    for ( const auto &coordSys : coordSystems )
    {
      if ( coordSys.second == currentCrs )
        currentCrsFound = true;
    }
    if ( !currentCrsFound )
      coordSystems.append( { tr( "Map CRS" ), currentCrs } );

    for ( const std::pair<QString, QgsCoordinateReferenceSystem> &crsInfo : std::as_const( coordSystems ) )
    {
      const bool crsIsXY = QgsCoordinateReferenceSystemUtils::defaultCoordinateOrderForCrs( crsInfo.second ) == Qgis::CoordinateOrder::XY;

      if ( !posIsWgs84 && crsInfo.second != wgs84Crs )
      {
        const QgsPointXY point( crsIsXY ? firstNumber : secondNumber, crsIsXY ? secondNumber : firstNumber );
        const QgsCoordinateTransform crs2wgs84( crsInfo.second, wgs84Crs, QgsProject::instance()->transformContext() );

        if ( !crsInfo.second.bounds().contains( crs2wgs84.transform( point ) ) )
          continue;

        const QgsCoordinateTransform transform( crsInfo.second, currentCrs, QgsProject::instance()->transformContext() );
        QgsPointXY transformedPoint;
        try
        {
          transformedPoint = transform.transform( point );
        }
        catch ( const QgsException &e )
        {
          Q_UNUSED( e )
          return;
        }
        data.insert( QStringLiteral( "point" ), transformedPoint );

        const QList<Qgis::CrsAxisDirection> axisList = crsInfo.second.axisOrdering();
        QString firstSuffix;
        QString secondSuffix;
        if ( axisList.size() >= 2 )
        {
          firstSuffix = QgsCoordinateReferenceSystemUtils::axisDirectionToAbbreviatedString( axisList.at( 0 ) );
          secondSuffix = QgsCoordinateReferenceSystemUtils::axisDirectionToAbbreviatedString( axisList.at( 1 ) );
        }

        QgsLocatorResult result;
        result.filter = this;
        result.displayString = tr( "Go to %1%2 %3%4" ).arg( locale.toString( firstNumber, 'g', 10 ), firstSuffix, locale.toString( secondNumber, 'g', 10 ), secondSuffix );
        result.description = QString( "%1, %2" ).arg( crsInfo.first, crsInfo.second.userFriendlyIdentifier() );
        result.setUserData( data );
        result.score = 0.9;
        emit resultFetched( result );
      }
    }

    if ( withinWgs84 )
    {
      const QgsPointXY point( secondNumber, firstNumber );
      if ( currentCrs != wgs84Crs )
      {
        const QgsCoordinateTransform transform( wgs84Crs, currentCrs, QgsProject::instance()->transformContext() );
        QgsPointXY transformedPoint;
        try
        {
          transformedPoint = transform.transform( point );
        }
        catch ( const QgsException &e )
        {
          Q_UNUSED( e )
          return;
        }
        data[QStringLiteral( "point" )] = transformedPoint;
      }
      else
      {
        data[QStringLiteral( "point" )] = point;
      }

      QgsLocatorResult result;
      result.filter = this;
      result.displayString = tr( "Go to %1°N %2°E" ).arg( locale.toString( point.y(), 'g', 10 ), locale.toString( point.x(), 'g', 10 ) );
      result.description = wgs84Crs.userFriendlyIdentifier();
      result.setUserData( data );
      result.score = 1.0;
      emit resultFetched( result );
    }
  }

  QMap<int, double> scales;
  scales[0] = 739571909;
  scales[1] = 369785954;
  scales[2] = 184892977;
  scales[3] = 92446488;
  scales[4] = 46223244;
  scales[5] = 23111622;
  scales[6] = 11555811;
  scales[7] = 5777905;
  scales[8] = 2888952;
  scales[9] = 1444476;
  scales[10] = 722238;
  scales[11] = 361119;
  scales[12] = 180559;
  scales[13] = 90279;
  scales[14] = 45139;
  scales[15] = 22569;
  scales[16] = 11284;
  scales[17] = 5642;
  scales[18] = 2821;
  scales[19] = 1500;
  scales[20] = 1000;
  scales[21] = 282;

  const QUrl url( string );
  if ( url.isValid() )
  {
    double scale = 0.0;
    int meters = 0;
    bool okX = false;
    bool okY = false;
    double posX = 0.0;
    double posY = 0.0;
    if ( url.hasFragment() )
    {
      // Check for OSM/Leaflet/OpenLayers pattern (e.g. http://www.openstreetmap.org/#map=6/46.423/4.746)
      const QStringList fragments = url.fragment().split( '&' );
      for ( const QString &fragment : fragments )
      {
        if ( fragment.startsWith( QLatin1String( "map=" ) ) )
        {
          const QStringList params = fragment.mid( 4 ).split( '/' );
          if ( params.size() >= 3 )
          {
            if ( scales.contains( params.at( 0 ).toInt() ) )
            {
              scale = scales.value( params.at( 0 ).toInt() );
            }
            posX = params.at( 2 ).toDouble( &okX );
            posY = params.at( 1 ).toDouble( &okY );
          }
          break;
        }
      }
    }

    if ( !okX && !okY )
    {
      const thread_local QRegularExpression locationRx( QStringLiteral( "google.*\\/@([0-9\\-\\.\\,]*)(z|m|a)" ) );
      match = locationRx.match( string );
      if ( match.hasMatch() )
      {
        const QStringList params = match.captured( 1 ).split( ',' );
        if ( params.size() == 3 )
        {
          posX = params.at( 1 ).toDouble( &okX );
          posY = params.at( 0 ).toDouble( &okY );

          if ( okX && okY )
          {
            if ( match.captured( 2 ) == QChar( 'z' ) && scales.contains( static_cast<int>( params.at( 2 ).toDouble() ) ) )
            {
              scale = scales.value( static_cast<int>( params.at( 2 ).toDouble() ) );
            }
            else if ( match.captured( 2 ) == QChar( 'm' ) )
            {
              // satellite view URL, scale to be derived from canvas height
              meters = params.at( 2 ).toInt();
            }
            else if ( match.captured( 2 ) == QChar( 'a' ) )
            {
              // street view URL, use most zoomed in scale value
              scale = scales.value( 21 );
            }
          }
        }
      }
    }

    if ( okX && okY )
    {
      QVariantMap data;
      const QgsPointXY point( posX, posY );
      QgsPointXY dataPoint = point;
      const bool withinWgs84 = wgs84Crs.bounds().contains( point );
      if ( withinWgs84 && currentCrs != wgs84Crs )
      {
        const QgsCoordinateTransform transform( wgs84Crs, currentCrs, QgsProject::instance()->transformContext() );
        dataPoint = transform.transform( point );
      }
      data.insert( QStringLiteral( "point" ), dataPoint );

      if ( meters > 0 )
      {
        const QSize outputSize = mCanvas->mapSettings().outputSize();
        QgsDistanceArea da;
        da.setSourceCrs( currentCrs, QgsProject::instance()->transformContext() );
        da.setEllipsoid( QgsProject::instance()->ellipsoid() );
        const double height = da.measureLineProjected( dataPoint, meters );
        const double width = outputSize.width() * ( height / outputSize.height() );

        QgsRectangle extent;
        extent.setYMinimum( dataPoint.y() - height / 2.0 );
        extent.setYMaximum( dataPoint.y() + height / 2.0 );
        extent.setXMinimum( dataPoint.x() - width / 2.0 );
        extent.setXMaximum( dataPoint.x() + width / 2.0 );

        QgsScaleCalculator calculator;
        calculator.setMapUnits( currentCrs.mapUnits() );
        calculator.setDpi( mCanvas->mapSettings().outputDpi() );
        scale = calculator.calculate( extent, outputSize.width() );
      }

      if ( scale > 0.0 )
      {
        data.insert( QStringLiteral( "scale" ), scale );
      }

      QgsLocatorResult result;
      result.filter = this;
      result.displayString = tr( "Go to %1°N %2°E %3" ).arg( locale.toString( point.y(), 'g', 10 ), locale.toString( point.x(), 'g', 10 ), scale > 0.0 ? tr( "at scale 1:%1 " ).arg( scale ) : QString() );
      result.description = wgs84Crs.userFriendlyIdentifier();
      result.setUserData( data );
      result.score = 1.0;
      emit resultFetched( result );
    }
  }

  // END OF QGIS CODE QgsGotoLocatorFilter::fetchResults


  const QgsCoordinateReferenceSystem epsg4326( QStringLiteral( "EPSG:4326" ) );

  QgsLocatorResult result;
  result.filter = this;
  QVariantMap data;

  result.score = 1.0;
  /*
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
      result.displayString = QString( "%1 %2 (LV03, EPSG:21781)").arg(tr("Go to"), point.toString());
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
      result.setUserData(data);
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
        result.displayString = QString( "%1 %2 (LV95, EPSG:2056)").arg(tr("Go to"), point.toString());
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
        result.setUserData(data);
        emit resultFetched( result );
      }
    }
  }
  */
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
  else*/
  if ( matchOneOf( string, { mPatUTM, mPatUTMalt }, match ) )
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
      result.setUserData( data );
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
      result.setUserData( data );
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
        result.setUserData( data );
        emit resultFetched( result );
      }
    }
  }
}

void KadasAlternateGotoLocatorFilter::triggerResult( const QgsLocatorResult &result )
{
  QVariantMap data = result.userData().toMap();
  QgsPointXY point = data[QStringLiteral( "point" )].value<QgsPointXY>();
  mCanvas->setCenter( point );

  QgsCoordinateTransform annotationLayerTransform;
  if ( QgsProject::instance()->mainAnnotationLayer()->crs().isValid() && QgsProject::instance()->mainAnnotationLayer()->crs() != mCanvas->mapSettings().destinationCrs() )
  {
    annotationLayerTransform = QgsCoordinateTransform( mCanvas->mapSettings().destinationCrs(), QgsProject::instance()->mainAnnotationLayer()->crs(), QgsProject::instance() );
    point = annotationLayerTransform.transform( point );
  }
  QgsAnnotationMarkerItem *item = new QgsAnnotationMarkerItem( QgsPoint( point ) );
  QgsSvgMarkerSymbolLayer *symbolLayer = new QgsSvgMarkerSymbolLayer( QStringLiteral( ":/kadas/icons/pin_blue" ), 25 );
  symbolLayer->setVerticalAnchorPoint( Qgis::VerticalAnchorPoint::Bottom );
  item->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );
  mPinItemId = QgsProject::instance()->mainAnnotationLayer()->addItem( item );

  if ( data.contains( QStringLiteral( "scale" ) ) )
  {
    mCanvas->zoomScale( data[QStringLiteral( "scale" )].toDouble() );
  }
  else
  {
    mCanvas->refresh();
  }
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

void KadasAlternateGotoLocatorFilter::clearPreviousResults()
{
  if ( !mPinItemId.isEmpty() )
  {
    QgsProject::instance()->mainAnnotationLayer()->removeItem( mPinItemId );
    mPinItemId = QString();
  }
}
