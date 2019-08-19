/***************************************************************************
    kadaslatlontoutm.cpp
    --------------------
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

#include <qmath.h>

#include <qgis/qgsdistancearea.h>
#include <qgis/qgspoint.h>

#include <kadas/core/kadaslatlontoutm.h>


const int KadasLatLonToUTM::NUM_100K_SETS = 6;
const QString KadasLatLonToUTM::SET_ORIGIN_COLUMN_LETTERS = "AJSAJS";
const QString KadasLatLonToUTM::SET_ORIGIN_ROW_LETTERS = "AFAFAF";

QgsPointXY KadasLatLonToUTM::UTM2LL ( const UTMCoo& utm, bool& ok )
{
  ok = false;

  // check whether the ZoneNummber and ZoneLetter are valid
  if ( utm.zoneNumber < 0 || utm.zoneNumber > 60 || utm.zoneLetter.isEmpty() ) {
    return QgsPointXY();
  }

  const double k0 = 0.9996;
  const double a = 6378137.0; //ellip.radius;
  const double eccSquared = 0.00669438; //ellip.eccsq;
  double e1 = ( 1 - qSqrt ( 1 - eccSquared ) ) / ( 1 + qSqrt ( 1 - eccSquared ) );

  // remove 500,000 meter offset for longitude
  double x = utm.easting - 500000.0;
  double y = utm.northing;

  // We must know somehow if we are in the Northern or Southern
  // hemisphere, this is the only time we use the letter So even
  // if the Zone letter isn't exactly correct it should indicate
  // the hemisphere correctly
  if ( utm.zoneLetter.at ( 0 ) < 'N' ) {
    y -= 10000000.0; // remove 10,000,000 meter offset used
    // for southern hemisphere
  }

  // There are 60 zones with zone 1 being at West -180 to -174
  double LongOrigin = ( utm.zoneNumber - 1 ) * 6 - 180 + 3; // +3 puts origin in middle of zone

  double eccPrimeSquared = ( eccSquared ) / ( 1 - eccSquared );

  double M = y / k0;
  double mu = M / ( a * ( 1 - eccSquared / 4 - 3 * eccSquared * eccSquared / 64 - 5 * eccSquared * eccSquared * eccSquared / 256 ) );

  double phi1Rad = mu + ( 3 * e1 / 2 - 27 * e1 * e1 * e1 / 32 ) * qSin ( 2 * mu ) + ( 21 * e1 * e1 / 16 - 55 * e1 * e1 * e1 * e1 / 32 ) * qSin ( 4 * mu ) + ( 151 * e1 * e1 * e1 / 96 ) * qSin ( 6 * mu );
  // double phi1 = ProjMath.radToDeg(phi1Rad);

  double N1 = a / qSqrt ( 1 - eccSquared * qSin ( phi1Rad ) * qSin ( phi1Rad ) );
  double T1 = qTan ( phi1Rad ) * qTan ( phi1Rad );
  double C1 = eccPrimeSquared * qCos ( phi1Rad ) * qCos ( phi1Rad );
  double R1 = a * ( 1 - eccSquared ) / qPow ( 1 - eccSquared * qSin ( phi1Rad ) * qSin ( phi1Rad ), 1.5 );
  double D = x / ( N1 * k0 );

  double lat = phi1Rad - ( N1 * qTan ( phi1Rad ) / R1 ) * ( D * D / 2 - ( 5 + 3 * T1 + 10 * C1 - 4 * C1 * C1 - 9 * eccPrimeSquared ) * D * D * D * D / 24 + ( 61 + 90 * T1 + 298 * C1 + 45 * T1 * T1 - 252 * eccPrimeSquared - 3 * C1 * C1 ) * D * D * D * D * D * D / 720 );
  lat = lat / M_PI * 180.;

  double lon = ( D - ( 1 + 2 * T1 + C1 ) * D * D * D / 6 + ( 5 - 2 * C1 + 28 * T1 - 3 * C1 * C1 + 8 * eccPrimeSquared + 24 * T1 * T1 ) * D * D * D * D * D / 120 ) / qCos ( phi1Rad );
  lon = LongOrigin + lon / M_PI * 180.;

  ok = true;
  return QgsPointXY ( lon, lat );
}

KadasLatLonToUTM::UTMCoo KadasLatLonToUTM::LL2UTM ( const QgsPointXY& pLatLong )
{
  const double a = 6378137.0; //ellip.radius;
  const double eccSqr = 0.00669438; //ellip.eccsq;
  const double k0 = 0.9996;

  double Long = pLatLong.x();
  double Lat = pLatLong.y();
  double LatRad = Lat / 180. * M_PI;
  double LongRad = Long / 180. * M_PI;
  int ZoneNumber = getZoneNumber ( Long, Lat );

  //+3 puts origin in middle of zone
  double LongOriginRad = ( ( ZoneNumber - 1 ) * 6 - 180 + 3 ) / 180. * M_PI;

  double eccPrimeSquared = ( eccSqr ) / ( 1 - eccSqr );

  double N = a / qSqrt ( 1 - eccSqr * qSin ( LatRad ) * qSin ( LatRad ) );
  double T = qTan ( LatRad ) * qTan ( LatRad );
  double C = eccPrimeSquared * qCos ( LatRad ) * qCos ( LatRad );
  double A = qCos ( LatRad ) * ( LongRad - LongOriginRad );
  double M = a * (
               ( 1 - eccSqr / 4 - 3 * eccSqr * eccSqr / 64 - 5 * eccSqr * eccSqr * eccSqr / 256 ) * LatRad -
               ( 3 * eccSqr / 8 + 3 * eccSqr * eccSqr / 32 + 45 * eccSqr * eccSqr * eccSqr / 1024 ) * qSin ( 2 * LatRad ) +
               ( 15 * eccSqr * eccSqr / 256 + 45 * eccSqr * eccSqr * eccSqr / 1024 ) * qSin ( 4 * LatRad ) -
               ( 35 * eccSqr * eccSqr * eccSqr / 3072 ) * qSin ( 6 * LatRad ) );

  UTMCoo coo;

  coo.easting = ( k0 * N * ( A + ( 1 - T + C ) * A * A * A / 6.0 + ( 5 - 18 * T + T * T + 72 * C - 58 * eccPrimeSquared ) * A * A * A * A * A / 120.0 ) + 500000.0 );

  coo.northing = ( k0 * ( M + N * qTan ( LatRad ) * ( A * A / 2 + ( 5 - T + 9 * C + 4 * C * C ) * A * A * A * A / 24.0 + ( 61 - 58 * T + T * T + 600 * C - 330 * eccPrimeSquared ) * A * A * A * A * A * A / 720.0 ) ) );
  if ( Lat < 0.0 ) {
    coo.northing += 10000000.0; //10000000 meter offset for southern hemisphere
  }

  coo.zoneNumber = ZoneNumber;
  coo.zoneLetter = getHemisphereLetter ( Lat );
  return coo;
}

int KadasLatLonToUTM::getZoneNumber ( double lon, double lat )
{
  int zoneNumber = qFloor ( ( lon + 180. ) / 6. ) + 1;

  //Make sure the longitude 180.00 is in Zone 60
  if ( lon >= 180.0 ) {
    zoneNumber = 60;
  }

  // Special zone for Norway
  if ( lat >= 56.0 && lat < 64.0 && lon >= 3.0 && lon < 12.0 ) {
    zoneNumber = 32;
  }

  // Special zones for Svalbard
  if ( lat >= 72.0 && lat < 84.0 ) {
    if ( lon >= 0.0 && lon < 9.0 ) {
      zoneNumber = 31;
    } else if ( lon >= 9.0 && lon < 21.0 ) {
      zoneNumber = 33;
    } else if ( lon >= 21.0 && lon < 33.0 ) {
      zoneNumber = 35;
    } else if ( lon >= 33.0 && lon < 42.0 ) {
      zoneNumber = 37;
    }
  }
  return zoneNumber;
}

QString KadasLatLonToUTM::getHemisphereLetter ( double lat )
{
  if ( ( 84 >= lat ) && ( lat >= 72 ) ) {
    return "X";
  } else if ( ( 72 > lat ) && ( lat >= 64 ) ) {
    return "W";
  } else if ( ( 64 > lat ) && ( lat >= 56 ) ) {
    return "V";
  } else if ( ( 56 > lat ) && ( lat >= 48 ) ) {
    return "U";
  } else if ( ( 48 > lat ) && ( lat >= 40 ) ) {
    return "T";
  } else if ( ( 40 > lat ) && ( lat >= 32 ) ) {
    return "S";
  } else if ( ( 32 > lat ) && ( lat >= 24 ) ) {
    return "R";
  } else if ( ( 24 > lat ) && ( lat >= 16 ) ) {
    return "Q";
  } else if ( ( 16 > lat ) && ( lat >= 8 ) ) {
    return "P";
  } else if ( ( 8 > lat ) && ( lat >= 0 ) ) {
    return "N";
  } else if ( ( 0 > lat ) && ( lat >= -8 ) ) {
    return "M";
  } else if ( ( -8 > lat ) && ( lat >= -16 ) ) {
    return "L";
  } else if ( ( -16 > lat ) && ( lat >= -24 ) ) {
    return "K";
  } else if ( ( -24 > lat ) && ( lat >= -32 ) ) {
    return "J";
  } else if ( ( -32 > lat ) && ( lat >= -40 ) ) {
    return "H";
  } else if ( ( -40 > lat ) && ( lat >= -48 ) ) {
    return "G";
  } else if ( ( -48 > lat ) && ( lat >= -56 ) ) {
    return "F";
  } else if ( ( -56 > lat ) && ( lat >= -64 ) ) {
    return "E";
  } else if ( ( -64 > lat ) && ( lat >= -72 ) ) {
    return "D";
  } else if ( ( -72 > lat ) && ( lat >= -80 ) ) {
    return "C";
  }

  //This is an error flag to show that the Latitude is outside MGRS limits
  return "Z";
}

KadasLatLonToUTM::MGRSCoo KadasLatLonToUTM::UTM2MGRS ( const UTMCoo& utmcoo )
{
  int setParm = utmcoo.zoneNumber % NUM_100K_SETS;
  if ( setParm == 0 ) {
    setParm = NUM_100K_SETS;
  }

  int setColumn = qFloor ( utmcoo.easting / 100000 );
  int setRow = int ( qFloor ( utmcoo.northing / 100000 ) ) % 20;

  MGRSCoo mgrscoo;
  mgrscoo.easting = utmcoo.easting % 100000;
  mgrscoo.northing = utmcoo.northing % 100000;
  mgrscoo.zoneLetter = utmcoo.zoneLetter;
  mgrscoo.zoneNumber = utmcoo.zoneNumber;
  mgrscoo.letter100kID = getLetter100kID ( setColumn, setRow, setParm );
  return mgrscoo;
}

KadasLatLonToUTM::UTMCoo KadasLatLonToUTM::MGRS2UTM ( const MGRSCoo& mgrs, bool& ok )
{
  UTMCoo utm;
  utm.zoneLetter = mgrs.zoneLetter;
  utm.zoneNumber = mgrs.zoneNumber;
  utm.easting = mgrs.easting;
  utm.northing = mgrs.northing;

  int setParam = mgrs.zoneNumber % NUM_100K_SETS;
  if ( setParam == 0 ) {
    setParam = NUM_100K_SETS;
  }

  double easting100k;
  {
    int e = mgrs.letter100kID.at ( 0 ).unicode();
    int curCol = SET_ORIGIN_COLUMN_LETTERS.at ( setParam - 1 ).unicode();
    easting100k = 100000.0;
    bool rewindMarker = false;

    while ( curCol != e ) {
      ++curCol;
      if ( curCol == 'I' ) {
        ++curCol;
      }
      if ( curCol == 'O' ) {
        ++curCol;
      }
      if ( curCol > 'Z' ) {
        if ( rewindMarker ) {
          // Bad character
          ok = false;
          return utm;
        }
        curCol = 'A';
        rewindMarker = true;
      }
      easting100k += 100000.0;
    }
  }

  double northing100k;
  {
    int n = mgrs.letter100kID.at ( 1 ).unicode();
    if ( n > 'V' ) {
      // Invalid Northing
      return utm;
    }

    // rowOrigin is the letter at the origin of the set for the
    // column
    int curRow = SET_ORIGIN_ROW_LETTERS.at ( setParam - 1 ).unicode();
    northing100k = 0.0;
    bool rewindMarker = false;

    while ( curRow != n ) {
      ++curRow;
      if ( curRow == 'I' ) {
        ++curRow;
      }
      if ( curRow == 'O' ) {
        ++curRow;
      }
      // fixing a bug making whole application hang in this loop
      // when 'n' is a wrong character
      if ( curRow > 'V' ) {
        if ( rewindMarker ) {
          // Bad character
          ok = false;
          return utm;
        }
        curRow = 'A';
        rewindMarker = true;
      }
      northing100k += 100000.0;
    }

    double minNorthing = getMinNorthing ( mgrs.zoneLetter.at ( 0 ).unicode() );
    if ( minNorthing < 0.0 ) {
      // Invalid zone letter
      ok = false;
      return utm;
    }
    while ( northing100k < minNorthing ) {
      northing100k += 2000000;
    }
  }

  utm.easting = utm.easting % 100000 + easting100k;
  utm.northing = utm.northing % 100000 + northing100k;
  ok = true;
  return utm;
}

double KadasLatLonToUTM::getMinNorthing ( int zoneLetter )
{
  double northing;
  switch ( zoneLetter ) {
  case 'C':
    northing = 1100000.0;
    break;
  case 'D':
    northing = 2000000.0;
    break;
  case 'E':
    northing = 2800000.0;
    break;
  case 'F':
    northing = 3700000.0;
    break;
  case 'G':
    northing = 4600000.0;
    break;
  case 'H':
    northing = 5500000.0;
    break;
  case 'J':
    northing = 6400000.0;
    break;
  case 'K':
    northing = 7300000.0;
    break;
  case 'L':
    northing = 8200000.0;
    break;
  case 'M':
    northing = 9100000.0;
    break;
  case 'N':
    northing = 0.0;
    break;
  case 'P':
    northing = 800000.0;
    break;
  case 'Q':
    northing = 1700000.0;
    break;
  case 'R':
    northing = 2600000.0;
    break;
  case 'S':
    northing = 3500000.0;
    break;
  case 'T':
    northing = 4400000.0;
    break;
  case 'U':
    northing = 5300000.0;
    break;
  case 'V':
    northing = 6200000.0;
    break;
  case 'W':
    northing = 7000000.0;
    break;
  case 'X':
    northing = 7900000.0;
    break;
  default:
    northing = -1.0;
  }
  return northing;
}

QString KadasLatLonToUTM::getLetter100kID ( int column, int row, int parm )
{
  // colOrigin and rowOrigin are the letters at the origin of the set
  int index = parm - 1;
  if ( index < 0 || index >= SET_ORIGIN_COLUMN_LETTERS.length() ) {
    return QString();
  }
  if ( index < 0 || index >= SET_ORIGIN_ROW_LETTERS.length() ) {
    return QString();
  }

  int colOrigin = SET_ORIGIN_COLUMN_LETTERS.at ( index ).unicode();
  int rowOrigin = SET_ORIGIN_ROW_LETTERS.at ( index ).unicode();

  // colInt and rowInt are the letters to build to return
  int colInt = colOrigin + column - 1;
  int rowInt = rowOrigin + row;
  bool rollover = false;

  if ( colInt > 'Z' ) {
    colInt = colInt - 'Z' + 'A' - 1;
    rollover = true;
  }

  if ( colInt == 'I' || ( colOrigin < 'I' && colInt > 'I' ) || ( ( colInt > 'I' || colOrigin < 'I' ) && rollover ) ) {
    colInt++;
  }

  if ( colInt == 'O' || ( colOrigin < 'O' && colInt > 'O' ) || ( ( colInt > 'O' || colOrigin < 'O' ) && rollover ) ) {
    colInt++;

    if ( colInt == 'I' ) {
      colInt++;
    }
  }

  if ( colInt > 'Z' ) {
    colInt = colInt - 'Z' + 'A' - 1;
  }

  if ( rowInt > 'V' ) {
    rowInt = rowInt - 'V' + 'A' - 1;
    rollover = true;
  } else {
    rollover = false;
  }

  if ( ( ( rowInt == 'I' ) || ( ( rowOrigin < 'I' ) && ( rowInt > 'I' ) ) ) || ( ( ( rowInt > 'I' ) || ( rowOrigin < 'I' ) ) && rollover ) ) {
    rowInt++;
  }

  if ( ( ( rowInt == 'O' ) || ( ( rowOrigin < 'O' ) && ( rowInt > 'O' ) ) ) || ( ( ( rowInt > 'O' ) || ( rowOrigin < 'O' ) ) && rollover ) ) {
    rowInt++;

    if ( rowInt == 'I' ) {
      rowInt++;
    }
  }

  if ( rowInt > 'V' ) {
    rowInt = rowInt - 'V' + 'A' - 1;
  }

  return QString ( "%1%2" ).arg ( QChar ( colInt ) ).arg ( QChar ( rowInt ) );
}

static inline QPolygonF polyGridLineX ( double x, double y1, double y2, double stepY )
{
  QPolygonF poly;
  for ( double y = y1; y <= y2; y += stepY ) {
    poly.append ( QPointF ( x, y ) );
  }
  poly.append ( QPointF ( x, y2 ) );
  return poly;
}

static inline QPolygonF polyGridLineY ( double x1, double x2, double stepX, double y )
{
  QPolygonF poly;
  for ( double x = x1; x <= x2; x += stepX ) {
    poly.append ( QPointF ( x, y ) );
  }
  poly.append ( QPointF ( x2, y ) );
  return poly;
}

static inline void truncateGridLine ( double& x1, double& x2, double& y1, double& y2, double xMin, double xMax, double yMin, double yMax )
{
  // Truncate at yMin
  if ( y1 < yMin ) {
    double lambda = ( yMin - y1 ) / ( y2 - y1 );
    y1 = yMin;
    x1 += lambda * ( x2 - x1 );
  }
  // Truncate at yMax
  if ( y2 > yMax ) {
    double lambda = ( yMax - y1 ) / ( y2 - y1 );
    y2 = yMax;
    x2 = x1 + lambda * ( x2 - x1 );
  }
  // Truncate at xMin/xMax
  if ( x1 < xMin ) {
    double lambda = ( xMin - x1 ) / ( x2 - x1 );
    x1 = xMin;
    y1 += lambda * ( y2 - y1 );
  } else if ( x1 > xMax ) {
    double lambda = ( xMax - x1 ) / ( x2 - x1 );
    x1 = xMax;
    y1 += lambda * ( y2 - y1 );
  }
  if ( x2 < xMin ) {
    double lambda = ( xMin - x1 ) / ( x2 - x1 );
    x2 = xMin;
    y2 = y1 + lambda * ( y2 - y1 );
  } else if ( x2 > xMax ) {
    double lambda = ( xMax - x1 ) / ( x2 - x1 );
    x2 = xMax;
    y2 = y1 + lambda * ( y2 - y1 );
  }
}

static inline QPointF truncateGridLineYMax ( const QPointF& p, const QPointF& q, double xMin, double xMax, double yMax, bool& truncated )
{
  QPointF ret = q;
  // Truncate at yMax
  if ( ret.y() > yMax ) {
    double dy = p.y() - ret.y();
    if ( qAbs ( dy ) > 1E-12 ) {
      double lambda = ( yMax - p.y() ) / ( ret.y() - p.y() );
      ret.ry() = yMax;
      ret.rx() = p.x() + lambda * ( ret.x() - p.x() );
    } else {
      ret = p;
    }
    truncated = true;
  }
  // Truncate at xMin/xMax
  if ( ret.x() < xMin ) {
    double dx = ret.x() - p.x();
    if ( qAbs ( dx ) > 1E-12 ) {
      double lambda = ( xMin - p.x() ) / ( ret.x() - p.x() );
      ret.rx() = xMin;
      ret.ry() = p.y() + lambda * ( ret.y() - p.y() );
    } else {
      ret = p;
    }
    truncated = true;
  } else if ( ret.x() > xMax ) {
    double dx = ret.x() - p.x();
    if ( qAbs ( dx ) > 1E-12 ) {
      double lambda = ( xMax - p.x() ) / ( ret.x() - p.x() );
      ret.rx() = xMax;
      ret.ry() = p.y() + lambda * ( ret.y() - p.y() );
    } else {
      ret = p;
    }
    truncated = true;
  }
  return ret;
}

static inline QPointF truncateGridLineYMin ( const QPointF& p, const QPointF& q, double xMin, double xMax, double yMin, bool& truncated )
{
  QPointF ret = q;
  // Truncate at yMin
  if ( ret.y() < yMin ) {
    double dy = p.y() - ret.y();
    if ( qAbs ( dy ) > 1E-12 ) {
      double lambda = ( yMin - ret.y() ) / ( p.y() - ret.y() );
      ret.ry() = yMin;
      ret.rx() += lambda * ( p.x() - ret.x() );
    } else {
      ret = p;
    }
    truncated = true;
  }
  // Truncate at xMin/xMax
  if ( ret.x() < xMin ) {
    double dx = ret.x() - p.x();
    if ( qAbs ( dx ) > 1E-12 ) {
      double lambda = ( xMin - p.x() ) / ( ret.x() - p.x() );
      ret.rx() = xMin;
      ret.ry() = p.y() + lambda * ( ret.y() - p.y() );
    } else {
      ret = p;
    }
    truncated = true;
  } else if ( ret.x() > xMax ) {
    double dx = ret.x() - p.x();
    if ( qAbs ( dx ) > 1E-12 ) {
      double lambda = ( xMax - p.x() ) / ( ret.x() - p.x() );
      ret.rx() = xMax;
      ret.ry() = p.y() + lambda * ( ret.y() - p.y() );
    } else {
      ret = p;
    }
    truncated = true;
  }
  return ret;
}

static inline QPointF truncateGridLineXMin ( const QPointF& p, const QPointF& q, double xMin )
{
  QPointF ret = q;
  if ( q.x() < xMin ) {
    double dx = ret.x() - p.x();
    if ( qAbs ( dx ) > 1E-12 ) {
      double lambda = ( xMin - p.x() ) / ( ret.x() - p.x() );
      ret.rx() = xMin;
      ret.ry() = p.y() + lambda * ( ret.y() - p.y() );
    } else {
      ret = p;
    }
  }
  return ret;
}

static inline QPointF truncateGridLineXMax ( const QPointF& p, const QPointF& q, double xMax )
{
  QPointF ret = q;
  if ( q.x() > xMax ) {
    double dx = ret.x() - p.x();
    if ( qAbs ( dx ) > 1E-12 ) {
      double lambda = ( xMax - p.x() ) / ( ret.x() - p.x() );
      ret.rx() = xMax;
      ret.ry() = p.y() + lambda * ( ret.y() - p.y() );
    } else {
      ret = p;
    }
  }
  return ret;
}

void KadasLatLonToUTM::computeGrid ( const QgsRectangle& bbox, double mapScale,
                                     QList<QPolygonF>& zoneLines, QList<QPolygonF>& subZoneLines, QList<QPolygonF>& gridLines,
                                     QList<ZoneLabel>& zoneLabels, QList<ZoneLabel>& subZoneLabels, QList<GridLabel>& gridLabels, GridMode gridMode )
{

  QgsDistanceArea da;
  da.setEllipsoid ( "WGS84" );

  double lats[] = { -90, -80, -72, -64, -56, -48, -40, -32, -24, -16, -8, 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 84, 90 };
  for ( int iy = 0, ny = sizeof ( lats ) / sizeof ( lats[0] ); iy < ny - 1; ++iy ) {
    for ( int ix = -30; ix < 30; ++ix ) {
      double x1 = ix * 6;
      double x2 = ( ix + 1 ) * 6;
      double y1 = lats[iy];
      double y2 = lats[iy + 1];

      // Special zone for Norway
      if ( y1 == 56. && y2 ==  64. ) {
        if ( x1 == 0 ) {
          x2 = 3;
        } else if ( x1 == 6 ) {
          x1 = 3;
        }
      }

      // Special zones from Svalbard
      if ( y1 == 72 && y2 == 84 ) {
        if ( x1 == 0 ) {
          x2 = 9;
        } else if ( x1 == 12 ) {
          x1 = 9;
          x2 = 21;
        } else if ( x1 == 24 ) {
          x1 = 21;
          x2 = 33;
        } else if ( x1 == 36 ) {
          x1 = 33;
        } else if ( x1 == 6 || x1 == 18 || x1 == 30 ) {
          continue;
        }
      }

      // Check if within area of interest
      if ( x1 > bbox.xMaximum() || x2 < bbox.xMinimum() ||
           y1 > bbox.yMaximum() || y2 < bbox.yMinimum() ) {
        continue;
      }

      double xMin = qMax ( x1, bbox.xMinimum() );
      double xMax = qMin ( x2, bbox.xMaximum() );
      double yMin = qMax ( y1, bbox.yMinimum() );
      double yMax = qMin ( y2, bbox.yMaximum() );

      // Split box perimeter into pieces and compute lines
      zoneLines << polyGridLineX ( xMin, yMin, yMax, 1 ) << polyGridLineX ( xMax, yMin, yMax, 1. );
      zoneLines << polyGridLineY ( xMin, xMax, 1., yMin ) << polyGridLineY ( xMin, xMax, 1., yMax );
      int zoneNumber = KadasLatLonToUTM::getZoneNumber ( x1, y1 );
      ZoneLabel label;
      label.pos = QPointF ( xMax, yMax );
      label.maxPos = QPointF ( xMin, yMin );
      label.label = QString ( "%1%2" ).arg ( zoneNumber ).arg ( KadasLatLonToUTM::getHemisphereLetter ( y1 ) );
      zoneLabels.append ( label );

      // Sub-grid
      if ( mapScale > 5000000 ) {
        continue;
      }
      int cellSize = 0;
      if ( mapScale > 500000 ) {
        if ( gridMode == GridMGRS ) {
          computeSubGrid ( 100000, xMin, xMax, yMin, yMax, subZoneLines, &subZoneLabels, 0, mgrs100kIDLabelCallback );
          continue;
        }
        cellSize = 100000;
      } else if ( mapScale > 50000 ) {
        cellSize = 10000;
      } else if ( mapScale > 5000 ) {
        cellSize = 1000;
      } else {
        cellSize = 100;
      }
      if ( gridMode == GridMGRS ) {
        computeSubGrid ( 100000, xMin, xMax, yMin, yMax, subZoneLines, &subZoneLabels, 0, mgrs100kIDLabelCallback );
        computeSubGrid ( cellSize, xMin, xMax, yMin, yMax, gridLines, 0, &gridLabels, 0, mgrsGridLabelCallback );
      } else {
        computeSubGrid ( cellSize, xMin, xMax, yMin, yMax, gridLines, 0, &gridLabels, 0, utmGridLabelCallback );
      }
    }
  }
}

void KadasLatLonToUTM::computeSubGrid ( int cellSize, double xMin, double xMax, double yMin, double yMax,
                                        QList<QPolygonF>& gridLines, QList<ZoneLabel>* zoneLabels, QList<GridLabel>* gridLabels,
                                        zoneLabelCallback_t* zoneLabelCallback, gridLabelCallback_t* lineLabelCallback )
{
  QgsPointXY p, q, r;
  bool ok;
  bool truncated;

  // X lines (vertical)
  UTMCoo coo = LL2UTM ( QgsPointXY ( xMin, yMin >= 0 ? yMin : yMax ) );
  // Round up to next grid line
  int reste = coo.easting % cellSize;
  coo.easting += reste != 0 ? cellSize - reste : 0;
  int restn = coo.northing % cellSize;
  int northing1 = coo.northing - restn;
  int northing2 = coo.northing + ( restn != 0 ? cellSize - restn : 0 );
  if ( zoneLabelCallback && reste != 0 && restn != 0 ) {
    UTMCoo maxCoo = coo;
    maxCoo.northing = northing2;
    QgsPointXY maxPos = UTM2LL ( maxCoo, ok );
    zoneLabels->append ( zoneLabelCallback ( xMin, yMin, maxPos.x(), maxPos.y() ) );
  }
  while ( ( p = UTM2LL ( coo, ok ) ).x() <= xMax && ok ) {
    QPolygonF xLine;
    // Draw segment from border of zone to next 100k position
    UTMCoo xcoo = coo;
    xcoo.northing = northing1;
    q = UTM2LL ( xcoo, ok );
    xcoo.northing = northing2;
    r = UTM2LL ( xcoo, ok );
    if ( yMin >= 0 ) {
      xLine.append ( truncateGridLineYMin ( QPointF ( r.x(), r.y() ), QPointF ( q.x(), q.y() ), xMin, xMax, yMin, truncated ) );
    } else {
      xLine.append ( truncateGridLineYMax ( QPointF ( r.x(), r.y() ), QPointF ( q.x(), q.y() ), xMin, xMax, yMax, truncated ) );
    }
    if ( lineLabelCallback ) {
      lineLabelCallback ( xLine.last().x(), xLine.last().y(), cellSize, false, gridLines.size(), *gridLabels );
    }
    if ( zoneLabelCallback && restn != 0 ) {
      UTMCoo maxCoo = xcoo;
      maxCoo.easting += cellSize;
      QgsPointXY maxPos = UTM2LL ( maxCoo, ok );
      zoneLabels->append ( zoneLabelCallback ( xLine.last().x(), qMax ( yMin, xLine.last().y() ), maxPos.x(), maxPos.y() ) );
    }
    truncated = false;
    // Draw remaining segments of grid line
    if ( yMin >= 0 ) {
      while ( !truncated && ( q = UTM2LL ( xcoo, ok ) ).y() < yMax && ok ) {
        xLine.append ( truncateGridLineYMax ( xLine.back(), QPointF ( q.x(), q.y() ), xMin, xMax, yMax, truncated ) );
        xcoo.northing += cellSize;
      }
      if ( !truncated ) {
        xLine.append ( truncateGridLineYMax ( xLine.back(), QPointF ( q.x(), q.y() ), xMin, xMax, yMax, truncated ) );
      }
    } else {
      while ( !truncated && ( q = UTM2LL ( xcoo, ok ) ).y() > yMin && ok ) {
        xLine.append ( truncateGridLineYMin ( xLine.back(), QPointF ( q.x(), q.y() ), xMin, xMax, yMin, truncated ) );
        xcoo.northing -= cellSize;
      }
      if ( !truncated ) {
        xLine.append ( truncateGridLineYMin ( xLine.back(), QPointF ( q.x(), q.y() ), xMin, xMax, yMin, truncated ) );
      }
    }
    coo.easting += cellSize;
    gridLines.append ( xLine );
  }

  // Y lines (horizontal)
  coo = LL2UTM ( QgsPointXY ( xMin, yMin ) );
  // Round up to next grid line
  restn = coo.northing % cellSize;
  coo.northing += restn != 0 ? cellSize - restn : 0;

  while ( ( p = UTM2LL ( coo, ok ) ).y() <= yMax && ok ) {
    QPolygonF yLine;
    // Draw segment from border of zone to next 100k position
    reste = coo.easting % cellSize;
    UTMCoo ycoo = coo;
    ycoo.easting = coo.easting + ( reste != 0 ? cellSize - reste : 0 );
    while ( UTM2LL ( ycoo, ok ).x() < xMin && ok ) {
      ycoo.easting += cellSize;
    }
    while ( ( q = UTM2LL ( ycoo, ok ) ).x() > xMin && ok ) {
      ycoo.easting -= cellSize;
    }
    ycoo.easting += cellSize;
    r = UTM2LL ( ycoo, ok );
    yLine.append ( truncateGridLineXMin ( QPointF ( r.x(), r.y() ), QPointF ( q.x(), q.y() ), xMin ) );
    if ( lineLabelCallback ) {
      lineLabelCallback ( yLine.last().x(), yLine.last().y(), cellSize, true, gridLines.size(), *gridLabels );
    }
    if ( zoneLabelCallback && reste != 0 ) {
      UTMCoo maxCoo = ycoo;
      maxCoo.northing += cellSize;
      QgsPointXY maxPos = UTM2LL ( maxCoo, ok );
      zoneLabels->append ( zoneLabelCallback ( yLine.last().x(), qMax ( xMin, yLine.last().y() ), maxPos.x(), maxPos.y() ) );
    }
    // Draw remaining segments of grid line
    while ( ( q = UTM2LL ( ycoo, ok ) ).x() < xMax && ok ) {
      yLine.append ( QPointF ( q.x(), q.y() ) );
      if ( zoneLabelCallback ) {
        UTMCoo maxCoo = ycoo;
        maxCoo.easting += cellSize;
        maxCoo.northing += cellSize;
        QgsPointXY maxPos = UTM2LL ( maxCoo, ok );
        maxPos.setX ( qMin ( maxPos.x(), xMax ) );
        maxPos.setY ( qMin ( maxPos.y(), yMax ) );
        zoneLabels->append ( zoneLabelCallback ( q.x(), q.y(), maxPos.x(), maxPos.y() ) );
      }
      ycoo.easting += cellSize;
    }
    yLine.append ( truncateGridLineXMax ( yLine.back(), QPointF ( q.x(), q.y() ), xMax ) );
    coo.northing += cellSize;
    gridLines.append ( yLine );
  }
}

KadasLatLonToUTM::ZoneLabel KadasLatLonToUTM::mgrs100kIDLabelCallback ( double posX, double posY, double maxLon, double maxLat )
{
  UTMCoo utmcoo = LL2UTM ( QgsPointXY ( posX + 0.01, posY + 0.01 ) );
  int setColumn = qFloor ( utmcoo.easting / 100000 );
  int setRow = int ( qFloor ( utmcoo.northing / 100000 ) ) % 20;
  int setParm = utmcoo.zoneNumber % NUM_100K_SETS;
  if ( setParm == 0 ) {
    setParm = NUM_100K_SETS;
  }
  ZoneLabel label;
  label.pos = QPointF ( posX, posY );
  label.label = getLetter100kID ( setColumn, setRow, setParm );
  label.maxPos = QPointF ( maxLon, maxLat );
  return label;
}

void KadasLatLonToUTM::utmGridLabelCallback ( double lon, double lat, double cellSize, bool horiz, int lineIdx, QList<GridLabel>& gridLabels )
{
  UTMCoo utmcoo = LL2UTM ( QgsPointXY ( lon + 0.01, lat + 0.01 ) );
  GridLabel label;
  if ( horiz ) {
    label.label = QString::number ( int ( utmcoo.northing / cellSize ) );
  } else {
    label.label = QString::number ( int ( utmcoo.easting / cellSize ) );
  }
  label.horiz = horiz;
  label.lineIdx = lineIdx;
  gridLabels.append ( label );
}

void KadasLatLonToUTM::mgrsGridLabelCallback ( double lon, double lat, double cellSize, bool horiz, int lineIdx, QList<GridLabel>& gridLabels )
{
  UTMCoo utmcoo = LL2UTM ( QgsPointXY ( lon + 0.0001, lat + 0.0001 ) );
  GridLabel label;
  if ( horiz ) {
    label.label = QString::number ( int ( ( utmcoo.northing % 100000 ) / cellSize ) );
  } else {
    label.label = QString::number ( int ( ( utmcoo.easting % 100000 ) / cellSize ) );
  }
  if ( label.label == "0" ) {
    return;    // 0 always overlaps with a 100kID zone label, so omit it
  }
  label.horiz = horiz;
  label.lineIdx = lineIdx;
  gridLabels.append ( label );
}
