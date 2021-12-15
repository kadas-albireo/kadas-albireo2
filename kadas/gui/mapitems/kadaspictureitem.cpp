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

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/analysis/kadaslineofsight.h>
#include <kadas/gui/mapitems/kadaspictureitem.h>


KADAS_REGISTER_MAP_ITEM( KadasPictureItem, []( const QgsCoordinateReferenceSystem &crs )  { return new KadasPictureItem( crs ); } );

QJsonObject KadasPictureItem::State::serialize() const
{
  QJsonArray p;
  p.append( pos.x() );
  p.append( pos.y() );
  QJsonArray s;
  s.append( size.width() );
  s.append( size.height() );
  QJsonArray f;
  for ( const KadasItemPos &footprintPos : footprint )
  {
    QJsonArray fp;
    fp.append( footprintPos.x() );
    fp.append( footprintPos.y() );
    f.append( fp );
  }
  QJsonArray t;
  t.append( cameraTarget.x() );
  t.append( cameraTarget.y() );

  QJsonObject json;
  json["status"] = drawStatus;
  json["pos"] = p;
  json["angle"] = angle;
  json["offsetX"] = offsetX;
  json["offsetY"] = offsetY;
  json["size"] = s;
  json["footprint"] = f;
  json["cameraTarget"] = t;
  return json;
}

bool KadasPictureItem::State::deserialize( const QJsonObject &json )
{
  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  QJsonArray p = json["pos"].toArray();
  pos = KadasItemPos( p.at( 0 ).toDouble(), p.at( 1 ).toDouble() );
  angle = json["angle"].toDouble();
  offsetX = json["offsetX"].toDouble();
  offsetY = json["offsetY"].toDouble();
  QJsonArray s = json["size"].toArray();
  size = QSize( s.at( 0 ).toDouble(), s.at( 1 ).toDouble() );
  QJsonArray f = json["footprint"].toArray();
  footprint.clear();
  for ( const QJsonValue &v : f )
  {
    QJsonArray fp = v.toArray();
    footprint.append( KadasItemPos( fp.at( 0 ).toDouble(), fp.at( 1 ).toDouble() ) );
  }
  QJsonArray t = json["cameraTarget"].toArray();
  cameraTarget = KadasItemPos( t.at( 0 ).toDouble(), t.at( 1 ).toDouble() );
  return true;
}


KadasPictureItem::KadasPictureItem( const QgsCoordinateReferenceSystem &crs )
  : KadasMapItem( crs )
{
  mIsPointSymbol = true;
  clear();
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
    state()->size.setWidth( width );
    state()->size.setHeight( reader.size().height() * double( width ) / reader.size().width() );
  }
  else if ( height > 0 )
  {
    state()->size.setHeight( height );
    state()->size.setWidth( reader.size().width() * double( height ) / reader.size().height() );
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

    state()->size = size;
  }

  state()->pos = fallbackPos;
  KadasItemPos cameraPos;
  QList<KadasItemPos> footprint;
  KadasItemPos cameraTarget;
  if ( !ignoreExiv && readGeoPos( path, mCrs, cameraPos, footprint, cameraTarget ) )
  {
    state()->pos = cameraPos;
    state()->footprint = footprint;
    state()->cameraTarget = cameraTarget;
    mPosLocked = true;
  }

  state()->offsetX = offsetX;
  state()->offsetY = offsetY;

  reader.setBackgroundColor( Qt::white );
  reader.setScaledSize( state()->size );
  mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );

  update();
}

void KadasPictureItem::setFilePath( const QString &filePath )
{
  setup( filePath, constState()->pos, true, state()->offsetX, state()->offsetY );
  update();
}

void KadasPictureItem::setFrameVisible( bool frame )
{
  mFrame = frame;
  if ( !frame )
  {
    state()->offsetX = 0;
    state()->offsetY = 0;
  }
  update();
}

void KadasPictureItem::setPositionLocked( bool locked )
{
  mPosLocked = locked;
  update();
}

void KadasPictureItem::setPosition( const KadasItemPos &pos )
{
  if ( !mPosLocked )
  {
    state()->pos = pos;
    state()->drawStatus = State::DrawStatus::Finished;
    update();
  }
}

void KadasPictureItem::setState( const KadasMapItem::State *state )
{
  const KadasPictureItem::State *pictureState = dynamic_cast<const KadasPictureItem::State *>( state );
  if ( pictureState && pictureState->size != constState()->size )
  {
    QImageReader reader( mFilePath );
    reader.setBackgroundColor( Qt::white );
    reader.setScaledSize( pictureState->size );
    mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );
  }
  KadasMapItem::setState( state );
}

KadasItemRect KadasPictureItem::boundingBox() const
{
  double xmin = constState()->pos.x(), xmax = constState()->pos.x();
  double ymin = constState()->pos.y(), ymax = constState()->pos.y();
  for ( const KadasItemPos &p : constState()->footprint )
  {
    xmin = std::min( xmin, p.x() );
    xmax = std::max( xmax, p.x() );
    ymin = std::min( ymin, p.y() );
    ymax = std::max( ymax, p.y() );
  }
  return KadasItemRect( xmin, ymin, xmax, ymax );
}

KadasMapItem::Margin KadasPictureItem::margin() const
{
  double framePadding = mFrame ? sFramePadding : 0;
  return Margin
  {
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->size.width() - constState()->offsetX + framePadding ) * mSymbolScale ) ),
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->size.height() + constState()->offsetY + framePadding ) * mSymbolScale ) ),
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->size.width() + constState()->offsetX + framePadding ) * mSymbolScale ) ),
    static_cast<int>( std::ceil( std::max( 0., 0.5 * constState()->size.height() - constState()->offsetY + framePadding ) * mSymbolScale ) )
  };
}

QList<KadasMapPos> KadasPictureItem::cornerPoints( const QgsMapSettings &settings ) const
{
  KadasMapPos mapPos = toMapPos( constState()->pos, settings );
  double halfW = 0.5 * constState()->size.width();
  double halfH = 0.5 * constState()->size.height();
  double scale = settings.mapUnitsPerPixel() * mSymbolScale;

  KadasMapPos p1( mapPos.x() + ( constState()->offsetX - halfW ) * scale, mapPos.y() + ( constState()->offsetY - halfH ) * scale );
  KadasMapPos p2( mapPos.x() + ( constState()->offsetX + halfW ) * scale, mapPos.y() + ( constState()->offsetY - halfH ) * scale );
  KadasMapPos p3( mapPos.x() + ( constState()->offsetX + halfW ) * scale, mapPos.y() + ( constState()->offsetY + halfH ) * scale );
  KadasMapPos p4( mapPos.x() + ( constState()->offsetX - halfW ) * scale, mapPos.y() + ( constState()->offsetY + halfH ) * scale );

  return QList<KadasMapPos>() << p1 << p2 << p3 << p4;
}

QList<KadasMapItem::Node> KadasPictureItem::nodes( const QgsMapSettings &settings ) const
{
  QList<KadasMapPos> points = cornerPoints( settings );
  QList<Node> nodes;
  nodes.append( {points[0]} );
  nodes.append( {points[1]} );
  nodes.append( {points[2]} );
  nodes.append( {points[3]} );
  nodes.append( {toMapPos( constState()->pos, settings ), anchorNodeRenderer} );
  return nodes;
}

bool KadasPictureItem::intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains ) const
{
  if ( constState()->size.isEmpty() )
  {
    return false;
  }

  QList<KadasMapPos> points = cornerPoints( settings );
  QgsPolygon imageRect;
  imageRect.setExteriorRing( new QgsLineString( QgsPointSequence() << QgsPoint( points[0] ) << QgsPoint( points[1] ) << QgsPoint( points[2] ) << QgsPoint( points[3] ) << QgsPoint( points[0] ) ) );

  QgsPolygon filterRect;
  QgsLineString *exterior = new QgsLineString();
  exterior->setPoints( QgsPointSequence()
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMinimum() )
                       << QgsPoint( rect.xMaximum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMaximum() )
                       << QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
  filterRect.setExteriorRing( exterior );

  QgsGeometryEngine *geomEngine = QgsGeometry::createGeometryEngine( &filterRect );
  bool intersects = contains ? geomEngine->contains( &imageRect ) : geomEngine->intersects( &imageRect );
  delete geomEngine;
  return intersects;
}

void KadasPictureItem::render( QgsRenderContext &context ) const
{
  QgsPoint pos( constState()->pos );
  pos.transform( context.coordinateTransform() );
  pos.transform( context.mapToPixel().transform() );

  // Draw footprint
  QPolygonF poly;
  if ( constState()->footprint.size() == 4 )
  {
    QgsPoint target( constState()->cameraTarget );
    target.transform( context.coordinateTransform() );
    target.transform( context.mapToPixel().transform() );

    for ( const KadasItemPos &fp : constState()->footprint )
    {
      QgsPointXY p = fp;
      p = context.coordinateTransform().transform( p );
      poly.append( context.mapToPixel().transform( p ).toQPointF() );
    }
    poly.append( poly.front() );
    QPainterPath path;
    path.addPolygon( poly );

    QLinearGradient strokeGradient( pos.toQPointF(), target.toQPointF() );
    strokeGradient.setColorAt( 0, QColor( 255, 0, 0 ) );
    strokeGradient.setColorAt( 0.75, QColor( 255, 0, 0 ) );
    strokeGradient.setColorAt( 1., QColor( 255, 0, 0, 12 ) );
    context.painter()->setPen( QPen( strokeGradient, 2, Qt::SolidLine ) );

    QLinearGradient fillGradient( pos.toQPointF(), target.toQPointF() );
    fillGradient.setColorAt( 0, QColor( 255, 0, 0, 127 ) );
    fillGradient.setColorAt( 0.75, QColor( 255, 0, 0, 127 ) );
    fillGradient.setColorAt( 1., QColor( 255, 0, 0, 12 ) );
    context.painter()->setBrush( fillGradient );
    context.painter()->drawPath( path );
    context.painter()->setPen( QPen( Qt::blue, 2, Qt::SolidLine ) );
    context.painter()->drawLine( pos.x(), pos.y(), poly.at( 0 ).x(), poly.at( 0 ).y() );
    context.painter()->drawLine( pos.x(), pos.y(), poly.at( 1 ).x(), poly.at( 1 ).y() );
  }

  context.painter()->translate( pos.x(), pos.y() );
  context.painter()->scale( mSymbolScale, mSymbolScale );
  double dpiScale = outputDpiScale( context );
  double arrowWidth = sArrowWidth * dpiScale;

  double w = constState()->size.width() * dpiScale;
  double h = constState()->size.height() * dpiScale;
  double offsetX = constState()->offsetX * dpiScale;
  double offsetY = constState()->offsetY * dpiScale;

  // Draw frame
  if ( mFrame )
  {
    double framew = w + 2 * sFramePadding * dpiScale;
    double frameh = h + 2 * sFramePadding * dpiScale;
    context.painter()->setPen( QPen( Qt::black, 1 ) );
    context.painter()->setBrush( Qt::white );
    QPolygonF poly;
    poly.append( QPointF( offsetX - 0.5 * framew, -offsetY + 0.5 * frameh ) );
    poly.append( QPointF( offsetX - 0.5 * framew, -offsetY - 0.5 * frameh ) );
    poly.append( QPointF( offsetX + 0.5 * framew, -offsetY - 0.5 * frameh ) );
    poly.append( QPointF( offsetX + 0.5 * framew, -offsetY + 0.5 * frameh ) );
    poly.append( QPointF( offsetX - 0.5 * framew, -offsetY + 0.5 * frameh ) );
    // Draw frame triangle
    if ( qAbs( offsetX ) > qAbs( 0.5 * framew ) || qAbs( offsetY ) > qAbs( 0.5 * frameh ) )
    {
      QgsPointXY framePos;
      QgsVector baseDir;
      // Determine triangle quadrant
      int quadrant = qRound( std::atan2( -offsetY, offsetX ) / M_PI * 180 / 90 );
      if ( quadrant < 0 ) { quadrant += 4; }
      if ( quadrant == 0 || quadrant == 2 )   // Triangle to the left (quadrant = 0) or right (quadrant = 2)
      {
        baseDir = QgsVector( 0, quadrant == 0 ? -1 : 1 );
        framePos.setX( quadrant == 0 ? offsetX - 0.5 * framew : offsetX + 0.5 * framew );
        framePos.setY( std::min( std::max( -offsetY - 0.5 * frameh + arrowWidth, 0. ), -offsetY + 0.5 * frameh - arrowWidth ) );
      }
      else     // Triangle above (quadrant = 1) or below (quadrant = 3)
      {
        framePos.setX( std::min( std::max( offsetX - 0.5 * framew + arrowWidth, 0. ), offsetX + 0.5 * framew - arrowWidth ) );
        framePos.setY( quadrant == 1 ? -offsetY - 0.5 * frameh : -offsetY + 0.5 * frameh );
        baseDir = QgsVector( quadrant == 1 ? 1 : -1, 0 );
      }
      int inspos = quadrant + 1;
      poly.insert( inspos++, QPointF( framePos.x() - arrowWidth * baseDir.x(), framePos.y() - arrowWidth * baseDir.y() ) );
      poly.insert( inspos++, QPointF( 0, 0 ) );
      poly.insert( inspos++, QPointF( framePos.x() + arrowWidth * baseDir.x(), framePos.y() + arrowWidth * baseDir.y() ) );
    }
    QPainterPath path;
    path.addPolygon( poly );
    context.painter()->drawPath( path );
  }

  if ( dpiScale != 1. )
  {
    QImageReader reader( mFilePath );
    reader.setBackgroundColor( Qt::white );
    reader.setScaledSize( constState()->size * dpiScale );
    QImage image = reader.read().convertToFormat( QImage::Format_ARGB32 );
    context.painter()->drawImage( QPointF( offsetX - 0.5 * w - 0.5, -offsetY - 0.5 * h - 0.5 ), image );
  }
  else
  {
    context.painter()->drawImage( QPointF( offsetX - 0.5 * w - 0.5, -offsetY - 0.5 * h - 0.5 ), mImage );
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

  double hotSpotX = 0.5 * constState()->size.width();
  double hotSpotY = 0.5 * constState()->size.height();
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

bool KadasPictureItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  state()->drawStatus = State::Drawing;
  state()->pos = toItemPos( firstPoint, mapSettings );
  update();
  return false;
}

bool KadasPictureItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasPictureItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

void KadasPictureItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

bool KadasPictureItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasPictureItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasPictureItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x"} );
  attributes.insert( AttrY, NumericAttribute{"y"} );
  return attributes;
}

KadasMapItem::AttribValues KadasPictureItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasPictureItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasPictureItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  double tol = pickTolSqr( mapSettings );
  QList<KadasMapPos> corners = cornerPoints( mapSettings );
  for ( int i = 0, n = corners.size(); i < n; ++i )
  {
    if ( pos.sqrDist( corners[i] ) < tol )
    {
      return EditContext( QgsVertexId( 0, 0, 1 + i ), corners[i], KadasMapItem::AttribDefs(), i % 2 == 0 ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor );
    }
  }
  KadasMapPos testPos = toMapPos( constState()->pos, mapSettings );
  bool frameClicked = hitTest( pos, mapSettings );
  if ( !mPosLocked && ( ( !mFrame && frameClicked ) || pos.sqrDist( testPos ) < tol ) )
  {
    return EditContext( QgsVertexId( 0, 0, 0 ), testPos, drawAttribs() );
  }
  if ( frameClicked )
  {
    double mup = mapSettings.mapUnitsPerPixel();
    KadasMapPos mapPos = toMapPos( constState()->pos, mapSettings );
    KadasMapPos framePos( mapPos.x() + constState()->offsetX * mup, mapPos.y() + constState()->offsetY * mup );
    return EditContext( QgsVertexId(), framePos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return EditContext();
}

void KadasPictureItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.vertex == 0 )
  {
    state()->pos = toItemPos( newPoint, mapSettings );
    state()->footprint.clear();
    update();
  }
  else if ( context.vidx.vertex >= 1 && context.vidx.vertex <= 4 )
  {
    QImageReader reader( mFilePath );

    double scale = mapSettings.mapUnitsPerPixel() * mSymbolScale;
    KadasMapPos mapPos = toMapPos( constState()->pos, mapSettings );
    KadasMapPos frameCenter( mapPos.x() + constState()->offsetX * scale, mapPos.y() + constState()->offsetY * scale );

    QgsVector halfSize = ( mapSettings.mapToPixel().transform( newPoint ) - mapSettings.mapToPixel().transform( frameCenter ) ) / mSymbolScale;
    state()->size.setWidth( 2 * qAbs( halfSize.x() ) );
    state()->size.setHeight( state()->size.width() / double( reader.size().width() ) * reader.size().height() );

    reader.setBackgroundColor( Qt::white );
    reader.setScaledSize( state()->size );
    mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );

    update();
  }
  else if ( mFrame )
  {
    QgsCoordinateTransform crst( crs(), mapSettings.destinationCrs(), QgsProject::instance() );
    QgsPointXY screenPos = mapSettings.mapToPixel().transform( newPoint );
    QgsPointXY screenAnchor = mapSettings.mapToPixel().transform( toMapPos( constState()->pos, mapSettings ) );
    state()->offsetX = screenPos.x() - screenAnchor.x();
    state()->offsetY = screenAnchor.y() - screenPos.y();
    update();
  }
}

void KadasPictureItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasPictureItem::populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings )
{
  QAction *frameAction = menu->addAction( tr( "Frame visible" ), [this]( bool active ) { setFrameVisible( active ); } );
  frameAction->setCheckable( true );
  frameAction->setChecked( mFrame );

  QAction *lockedAction = menu->addAction( tr( "Position locked" ), [this]( bool active ) { setPositionLocked( active ); } );
  lockedAction->setCheckable( true );
  lockedAction->setChecked( mPosLocked );
}

void KadasPictureItem::onDoubleClick( const QgsMapSettings &mapSettings )
{
  QDesktopServices::openUrl( QUrl::fromLocalFile( mFilePath ) );
}

KadasMapItem::AttribValues KadasPictureItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasPictureItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

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
  Exiv2::Image::AutoPtr image;
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
      double terrHeigth = KadasCoordinateFormat::instance()->getHeightAtPos( mrcPosXY, crs3857, QgsUnitTypes::DistanceMeters );
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
