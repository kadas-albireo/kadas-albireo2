/***************************************************************************
    kadaspictureitem.cpp
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

#include <QDesktopServices>
#include <QGenericMatrix>
#include <QImageReader>
#include <QJsonArray>
#include <QMenu>

#include <array>
#include <exiv2/exiv2.hpp>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>

#include <quazip/quazipfile.h>

#include "kadas/core/kadascoordinateutils.h"
#include "kadas/analysis/kadaslineofsight.h"
#include "kadas/gui/mapitems/kadaspictureitem.h"


class QVector3 : public QGenericMatrix<1, 3, float>
{
  public:
    QVector3( const std::array<float, 3> &data )
      : QGenericMatrix<1, 3, float>( data.data() )
    {}
    QVector3( const QGenericMatrix<1, 3, float> &other )
      : QGenericMatrix<1, 3, float>( other )
    {}
    const QVector3 &operator=( const QGenericMatrix<1, 3, float> &other )
    {
      static_cast<QGenericMatrix<1, 3, float>>( *this ) = other;
      return *this;
    }
    float operator[]( int idx ) const { return ( *this )( idx, 0 ); }
    operator std::array<float, 3>() const
    {
      return {( *this )[0], ( *this )[1], ( *this )[2]};
    }
};


QJsonObject KadasPictureItem::State::serialize() const
{
  return KadasRectangleItemBase::State::serialize();
}

bool KadasPictureItem::State::deserialize( const QJsonObject &json )
{
  if ( json.contains( "cameraTarget" ) )
  {
    json["anchor"] = json["cameraTarget"];
  }
  return KadasRectangleItemBase::State::deserialize( json );
}


KadasPictureItem::KadasPictureItem( const QgsCoordinateReferenceSystem &crs )
  : KadasRectangleItemBase( crs )
{
}

KadasPictureItem::~KadasPictureItem()
{
  cleanupAttachment( mFilePath );
}

void KadasPictureItem::setup( const QString &path, const KadasItemPos &fallbackPos, bool ignoreExiv, double offsetX, double offsetY, int width, int height )
{
  cleanupAttachment( mFilePath );

  mFilePath = path;
  QImageReader reader( path );

  // If a size is given, set size while maintaining aspect ratio (width prevails over height)
  if ( width > 0 )
  {
    state()->mSize.setWidth( width );
    state()->mSize.setHeight( reader.size().height() * double( width ) / reader.size().width() );
  }
  else if ( height > 0 )
  {
    state()->mSize.setHeight( height );
    state()->mSize.setWidth( reader.size().width() * double( height ) / reader.size().height() );
  }
  else
  {
    // Scale such that largest dimension is max 64px
    QSize size = reader.size();
    if ( size.width() > size.height() )
    {
      size.setHeight( ( 64. * size.height() ) / size.width() );
      size.setWidth( 64 );
    }
    else
    {
      size.setWidth( ( 64. * size.width() ) / size.height() );
      size.setHeight( 64 );
    }

    state()->mSize = size;
  }

  state()->mPos = fallbackPos;
  KadasItemPos cameraPos;
  QList<KadasItemPos> footprint;
  KadasItemPos cameraTarget;
  if ( !ignoreExiv && readGeoPos( path, mCrs, cameraPos, footprint, cameraTarget ) )
  {
    state()->mPos = cameraPos;
    state()->mFootprint = footprint;
    state()->mAnchorPoint = cameraTarget;
    mPosLocked = true;
  }

  state()->mOffsetX = offsetX;
  state()->mOffsetY = offsetY;

  reader.setBackgroundColor( Qt::white );
  reader.setScaledSize( state()->mSize );
  mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );

  update();
}

void KadasPictureItem::setFilePath( const QString &filePath )
{
  setup( filePath, constState()->mPos, true, state()->mOffsetX, state()->mOffsetY );
  update();
  emit propertyChanged();
}

void KadasPictureItem::setState( const KadasMapItem::State *state )
{
  const KadasPictureItem::State *pictureState = dynamic_cast<const KadasPictureItem::State *>( state );
  if ( pictureState && pictureState->mSize != constState()->mSize )
  {
    mImage = readImage();
  }
  KadasRectangleItemBase::setState( state );
}

QImage KadasPictureItem::readImage( double dpiScale ) const
{
  QImageReader reader( mFilePath );
  reader.setBackgroundColor( Qt::white );
  reader.setScaledSize( constState()->mSize * dpiScale );
  return reader.read().convertToFormat( QImage::Format_ARGB32 );
}

void KadasPictureItem::renderPrivate(QgsRenderContext &context , double dpiScale, double offsetX, double offsetY, double width, double height) const
{
  if ( dpiScale != 1. )
  {
    QImage image = readImage(dpiScale);
    context.painter()->drawImage( QPointF( offsetX - 0.5 * width - 0.5, -offsetY - 0.5 * height - 0.5 ), image );
  }
  else
  {
    context.painter()->drawImage( QPointF( offsetX - 0.5 * width - 0.5, -offsetY - 0.5 * height - 0.5 ), mImage );
  }
}

QString KadasPictureItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  if ( !kmzZip )
  {
    // Can only export to KMZ
    return "";
  }

  QString fileName = QUuid::createUuid().toString();
  fileName = fileName.mid( 1, fileName.length() - 2 ) + ".png";
  QuaZipFile outputFile( kmzZip );
  QuaZipNewInfo info( fileName );
  info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
  if ( !outputFile.open( QIODevice::WriteOnly, info ) || !mImage.save( &outputFile, "PNG" ) )
  {
    return "";
  }

  double hotSpotX = 0.5 * constState()->mSize.width();
  double hotSpotY = 0.5 * constState()->mSize.height();
  QgsPointXY pos = QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ).transform( position() );

  QString outString;
  QTextStream outStream( &outString );

  QString id = QUuid::createUuid().toString();
  id = id.mid( 1, id.length() - 2 );
  outStream << "<StyleMap id=\"" << id << "\">" << "\n";
  outStream << "  <Pair>" << "\n";
  outStream << "    <key>normal</key>" << "\n";
  outStream << "    <Style>" << "\n";
  outStream << "      <IconStyle>" << "\n";
  outStream << "        <scale>1.0</scale>" << "\n";
  outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
  outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />" << "\n";
  outStream << "      </IconStyle>" << "\n";
  outStream << "    </Style>" << "\n";
  outStream << "  </Pair>" << "\n";
  outStream << "  <Pair>" << "\n";
  outStream << "    <key>highlight</key>" << "\n";
  outStream << "    <Style>" << "\n";
  outStream << "      <IconStyle>" << "\n";
  outStream << "        <scale>1.0</scale>" << "\n";
  outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
  outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />" << "\n";
  outStream << "      </IconStyle>" << "\n";
  outStream << "    </Style>" << "\n";
  outStream << "  </Pair>" << "\n";
  outStream << "</StyleMap>" << "\n";
  outStream << "<Placemark>" << "\n";
  outStream << "  <name>" << itemName() << "</name>" << "\n";
  outStream << "  <styleUrl>#" << id << "</styleUrl>" << "\n";
  outStream << "  <Point>" << "\n";
  outStream << "    <coordinates>" << QString::number( pos.x(), 'f', 10 ) << "," << QString::number( pos.y(), 'f', 10 ) << ",0</coordinates>" << "\n";
  outStream << "  </Point>" << "\n";
  outStream << "</Placemark>" << "\n";
  outStream.flush();

  return outString;
}

void KadasPictureItem::editPrivate( const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
    QImageReader reader( mFilePath );

    double scale = mapSettings.mapUnitsPerPixel() * mSymbolScale;
    KadasMapPos mapPos = toMapPos( constState()->mPos, mapSettings );
    KadasMapPos frameCenter( mapPos.x() + constState()->mOffsetX * scale, mapPos.y() + constState()->mOffsetY * scale );

    QgsVector halfSize = ( mapSettings.mapToPixel().transform( newPoint ) - mapSettings.mapToPixel().transform( frameCenter ) ) / mSymbolScale;
    state()->mSize.setWidth( 2 * qAbs( halfSize.x() ) );
    state()->mSize.setHeight( state()->mSize.width() / double( reader.size().width() ) * reader.size().height() );

    reader.setBackgroundColor( Qt::white );
    reader.setScaledSize( state()->mSize );
    mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );
}

void KadasPictureItem::onDoubleClick( const QgsMapSettings &mapSettings )
{
  QDesktopServices::openUrl( QUrl::fromLocalFile( mFilePath ) );
}

static QMatrix3x3 rotAngleAxis( const std::array<float, 3> &u, float angle )
{
  float sina = std::sin( angle );
  float cosa = std::cos( angle );
  return QMatrix3x3(
           std::array<float, 9>
  {
    cosa + u[0] * u[0] * ( 1 - cosa ),         u[0] * u[1] * ( 1 - cosa ) - u[2] * sina,  u[0] * u[2] * ( 1 - cosa ) + u[1] * sina,
    u[0] * u[1] * ( 1 - cosa ) + u[2] * sina,  cosa + u[1] * u[1] * ( 1 - cosa ),         u[1] * u[2] * ( 1 - cosa ) - u[0] * sina,
    u[0] * u[2] * ( 1 - cosa ) - u[1] * sina,  u[1] * u[2] * ( 1 - cosa ) + u[0] * sina,  cosa + u[2] * u[2] * ( 1 - cosa )
  }.data()
         );
}

bool KadasPictureItem::readGeoPos( const QString &filePath, const QgsCoordinateReferenceSystem &destCrs, KadasItemPos &cameraPos, QList<KadasItemPos> &footprint, KadasItemPos &cameraTarget )
{
  // Read EXIF position
  Exiv2::Image::UniquePtr image;
  try
  {
    image = Exiv2::ImageFactory::open( filePath.toLocal8Bit().data() );
  }
  catch ( const Exiv2::Error & )
  {
    return false;
  }

  if ( image.get() == 0 )
  {
    return false;
  }

  image->readMetadata();
  Exiv2::ExifData &exifData = image->exifData();
  if ( exifData.empty() )
  {
    return false;
  }

  Exiv2::ExifData::iterator itLatRef = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLatitudeRef" ) );
  Exiv2::ExifData::iterator itLatVal = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLatitude" ) );
  Exiv2::ExifData::iterator itLonRef = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLongitudeRef" ) );
  Exiv2::ExifData::iterator itLonVal = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSLongitude" ) );

  if ( itLatRef == exifData.end() || itLatVal == exifData.end() ||
       itLonRef == exifData.end() || itLonVal == exifData.end() )
  {
    return false;
  }
  QString latRef = QString::fromStdString( itLatRef->value().toString() );
  QString lonRef = QString::fromStdString( itLonRef->value().toString() );

  double lat = parseExifRational( QString::fromStdString( itLatVal->value().toString() ) );
  double lon = parseExifRational( QString::fromStdString( itLonVal->value().toString() ) );

  if ( latRef.compare( "S", Qt::CaseInsensitive ) == 0 )
  {
    lat *= -1;
  }
  if ( lonRef.compare( "W", Qt::CaseInsensitive ) == 0 )
  {
    lon *= -1;
  }

  if ( lon == 0 && lat == 0 )
  {
    // Assume 0, 0 is an invalid coordinate as it is pretty unlikely the image was captured at that position
    return false;
  }

  QgsCoordinateReferenceSystem crs4326( "EPSG:4326" );
  cameraPos = KadasItemPos::fromPoint( QgsCoordinateTransform( crs4326, destCrs, QgsProject::instance() ).transform( QgsPointXY( lon, lat ) ) );

  // Footprint
  Exiv2::ExifData::iterator itUserComment = exifData.findKey( Exiv2::ExifKey( "Exif.Photo.UserComment" ) );
  Exiv2::ExifData::iterator itAltitude = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSAltitude" ) );
  Exiv2::ExifData::iterator itDirection = exifData.findKey( Exiv2::ExifKey( "Exif.GPSInfo.GPSImgDirection" ) );
  if ( itUserComment != exifData.end() && itAltitude != exifData.end() && itDirection != exifData.end() )
  {
    // I.e. {"ImgPitch":-6.84,"ImgRoll":1.26,"HorizontalFOV":65.83,"VerticalFOV":51.79,"Inclination":62.6,"Address":"Schlossgasse 4, 3752 Wimmis, Switzerland"}
    QVariantMap data = QJsonDocument::fromJson( QByteArray::fromStdString( itUserComment->value().toString() ) ).object().toVariantMap();

    if ( !data.isEmpty() && data.contains( "ImgRoll" ) && data.contains( "ImgPitch" ) && data.contains( "HorizontalFOV" ) && data.contains( "VerticalFOV" ) )
    {
      double alt = parseExifRational( QString::fromStdString( itAltitude->value().toString() ) );
      double direction = parseExifRational( QString::fromStdString( itDirection->value().toString( ) ) );

      QVector3 ey( {0, 1, 0} );

      // Camera rotation matrix
      float yaw = -direction / 180. * M_PI;
      float roll = data["ImgRoll"].toDouble() / 180. * M_PI;
      float pitch = data["ImgPitch"].toDouble() / 180. * M_PI;

      QMatrix3x3 Rcz = rotAngleAxis( {0, 0, 1}, yaw );
      QMatrix3x3 Rcy = rotAngleAxis( QVector3( Rcz * QVector3( {0, 1, 0} ) ), roll );
      QMatrix3x3 Rcx = rotAngleAxis( QVector3( Rcy * Rcz * QVector3( {1, 0, 0} ) ), pitch );

      QMatrix3x3 R = Rcx * Rcy * Rcz;

      // Aperture boundary direction vectors wrt unrotated camera
      float halfHFov = 0.5 * data["HorizontalFOV"].toDouble() / 180. * M_PI;
      float halfVFov = 0.5 * data["VerticalFOV"].toDouble() / 180. * M_PI;
      QMatrix3x3 Rz = rotAngleAxis( {0, 0, 1}, halfHFov );
      QMatrix3x3 Rx = rotAngleAxis( QVector3( Rz * QVector3( {1, 0, 0} ) ), -halfVFov );

      QVector3 bottomleft = Rx * Rz * ey;
      QVector3 bottomright( {-bottomleft[0], bottomleft[1], bottomleft[2]} );
      QVector3 topleft( {bottomleft[0], bottomleft[1], -bottomleft[2]} );
      QVector3 topright( {-topleft[0], topleft[1], topleft[2]} );

      // Aperture boundary direction vectors wrt camera
      QVector3 rbottomleft = R * bottomleft;
      QVector3 rbottomright = R * bottomright;
      QVector3 rtopleft = R * topleft;
      QVector3 rtopright = R * topright;

      // Terrain intersection: up to max 25km, binary search for terrain point from which camera becomes visible
      QgsCoordinateReferenceSystem crs3857( "EPSG:3857" );
      QgsPointXY mrcPosXY = QgsCoordinateTransform( destCrs, crs3857, QgsProject::instance() ).transform( cameraPos );
      // Ensure altitude is at least 1m above terrain
      double terrHeigth = KadasCoordinateUtils::getHeightAtPos( mrcPosXY, crs3857, Qgis::DistanceUnit::Meters );
      QgsPoint mrcPos( mrcPosXY.x(), mrcPosXY.y(), std::max( alt, terrHeigth + 1 ) );

      double d = 25000;
      QgsPoint pTerrBottomLeft = findTerrainIntersection( mrcPos, mrcPos, QgsPoint( mrcPos.x() + rbottomleft[0] * d, mrcPos.y() + rbottomleft[1] * d, mrcPos.z() + rbottomleft[2] * d ), crs3857 );
      QgsPoint pTerrBottomRight = findTerrainIntersection( mrcPos, mrcPos, QgsPoint( mrcPos.x() + rbottomright[0] * d, mrcPos.y() + rbottomright[1] * d, mrcPos.z() + rbottomright[2] * d ), crs3857 );
      QgsPoint pTerrTopLeft = findTerrainIntersection( mrcPos, mrcPos, QgsPoint( mrcPos.x() + rtopleft[0] * d, mrcPos.y() + rtopleft[1] * d, mrcPos.z() + rtopleft[2] * d ), crs3857 );
      QgsPoint pTerrTopRight = findTerrainIntersection( mrcPos, mrcPos, QgsPoint( mrcPos.x() + rtopright[0] * d, mrcPos.y() + rtopright[1] * d, mrcPos.z() + rtopright[2] * d ), crs3857 );

      QVector3 reye = R * ey;
      // Point in camera direction 25km away to compute the camera direction
      QgsPoint target( mrcPos.x() + reye[0] * .75 * d, mrcPos.y() + reye[1] * .75 * d, mrcPos.z() + reye[2] * .75 * d );

      QgsCoordinateTransform crst( crs3857, destCrs, QgsProject::instance() );
      footprint =
      {
        KadasItemPos::fromPoint( crst.transform( pTerrBottomLeft ) ),
        KadasItemPos::fromPoint( crst.transform( pTerrBottomRight ) ),
        KadasItemPos::fromPoint( crst.transform( pTerrTopRight ) ),
        KadasItemPos::fromPoint( crst.transform( pTerrTopLeft ) )
      };
      cameraTarget = KadasItemPos::fromPoint( crst.transform( target ) );
    }

  }

  return true;
}

double KadasPictureItem::parseExifRational( const QString &entry )
{
  QStringList parts = entry.split( QRegExp( "\\s+" ) );

  double value = 0;
  double div = 1;
  for ( const QString &rational : parts )
  {
    QStringList pair = rational.split( "/" );
    if ( pair.size() != 2 )
    {
      break;
    }
    value += ( pair[0].toDouble() / pair[1].toDouble() ) / div;
    div *= 60;
  }
  return value;
}

QgsPoint KadasPictureItem::findTerrainIntersection( const QgsPoint &cameraPos, QgsPoint nearPos, QgsPoint farPos, const QgsCoordinateReferenceSystem &crs )
{
  // Binary search between cameraPos and farPos until farPos sees camera, up to 2m resolution, 2m terrain resolution
  double resolution = 2;
  while ( nearPos.distanceSquared( farPos ) > resolution * resolution )
  {
    QgsPoint midPos( 0.5 * ( nearPos.x() + farPos.x() ), 0.5 * ( nearPos.y() + farPos.y() ), 0.5 * ( nearPos.z() + farPos.z() ) );
    double len = cameraPos.distance( midPos );
    int samples = std::round( len / resolution );
    if ( KadasLineOfSight::computeTargetVisibility( cameraPos, midPos, crs, samples, true, true ) )
    {
      nearPos = midPos;
    }
    else
    {
      farPos = midPos;
    }
  }
  return QgsPoint( 0.5 * ( nearPos.x() + farPos.x() ), 0.5 * ( nearPos.y() + farPos.y() ), 0.5 * ( nearPos.z() + farPos.z() ) );
}
