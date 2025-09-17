/***************************************************************************
    kadassymbolitem.cpp
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

#include <QSvgRenderer>
#include <QImageReader>

#include <quazip/quazipfile.h>

#include <svg2svgt/processorengine.h>
#include <svg2svgt/ruleengine.h>
#include <svg2svgt/logger.h>

#include <qgis/qgsgeometryengine.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrendercontext.h>

#include "kadas/core/kadascoordinateformat.h"
#include "kadas/gui/mapitems/kadassymbolitem.h"


KadasSymbolItem::KadasSymbolItem()
  : KadasAnchoredItem()
{
  clear();
}

KadasSymbolItem::~KadasSymbolItem()
{
  cleanupAttachment( mFilePath );
}

void KadasSymbolItem::setup( const QString &path, double anchorX, double anchorY, int width, int height )
{
  cleanupAttachment( mFilePath );

  mAnchorX = anchorX;
  mAnchorY = anchorY;

  mFilePath = path;
  QImageReader reader( path );
  mScalable = reader.format() == "svg";
  reader.setBackgroundColor( Qt::transparent );

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
    state()->size = reader.size();
  }
  mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );

  update();
}

void KadasSymbolItem::setFilePath( const QString &path )
{
  setup( path, mAnchorX, mAnchorY, constState()->size.width(), constState()->size.height() );
  // TODO !!! emit propertyChanged();
}

void KadasSymbolItem::setName( const QString &name )
{
  mName = name;
  // TODO !!! emit propertyChanged();
}

void KadasSymbolItem::setRemarks( const QString &remarks )
{
  mRemarks = remarks;
  // TODO !!! emit propertyChanged();
}

void KadasSymbolItem::setState( const KadasMapItem::State *state )
{
  const KadasSymbolItem::State *symbolState = dynamic_cast<const KadasSymbolItem::State *>( state );
  if ( symbolState && symbolState->size != constState()->size )
  {
    QImageReader reader( mFilePath );
    reader.setBackgroundColor( Qt::white );
    reader.setScaledSize( symbolState->size );
    mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );
  }
  KadasMapItem::setState( state );
}

void KadasSymbolItem::render( QgsRenderContext &context, QgsFeedback *feedback )
{
  if ( constState()->drawStatus == State::DrawStatus::Empty )
  {
    return;
  }

  double dpiScale = outputDpiScale( context );

  QgsPoint pos = QgsPoint( constState()->pos );
  pos.transform( context.coordinateTransform() );
  pos.transform( context.mapToPixel().transform() );

  context.painter()->translate( pos.x(), pos.y() );
  context.painter()->rotate( -constState()->angle );
  context.painter()->scale( mSymbolScale * dpiScale, mSymbolScale * dpiScale );
  context.painter()->translate( -mAnchorX * constState()->size.width(), -mAnchorY * constState()->size.height() );
  if ( mScalable )
  {
    svg2svgt::Logger logger;
    svg2svgt::RuleEngine ruleEngine( logger );
    ruleEngine.setDefaultRules();
    svg2svgt::ProcessorEngine processor( ruleEngine, logger );

    QSvgRenderer svgRenderer;
    QFile file( mFilePath );
    if ( file.open( QIODevice::ReadOnly ) )
    {
      svgRenderer.load( processor.process( file.readAll() ) );
    }
    QSize renderSize = constState()->size;
    svgRenderer.render( context.painter(), QRectF( 0, 0, renderSize.width(), renderSize.height() ) );
  }
  else
  {
    context.painter()->setRenderHint( QPainter::SmoothPixmapTransform, true );
    context.painter()->drawImage( 0, 0, mImage );
  }
}

QString KadasSymbolItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
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
  if ( !outputFile.open( QIODevice::WriteOnly, info ) || !mImage.scaled( constState()->size ).save( &outputFile, "PNG" ) )
  {
    return "";
  }

  double hotSpotX = mAnchorX * constState()->size.width();
  double hotSpotY = mAnchorY * constState()->size.height();
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

KadasMapItem::EditContext KadasSymbolItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  double tol = pickTolSqr( mapSettings );
  QList<KadasMapPos> corners = rotatedCornerPoints( constState()->angle, mapSettings );
  for ( int i = 0, n = corners.size(); i < n; ++i )
  {
    if ( pos.sqrDist( corners[i] ) < tol )
    {
      int res = ( int( 360 + qRound( constState()->angle + 45. ) ) / 90 ) % 2 == 0 ? 0 : 1;
      return EditContext( QgsVertexId( 0, 0, 2 + i ), corners[i], KadasMapItem::AttribDefs(), i % 2 == res ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor );
    }
  }
  return KadasAnchoredItem::getEditContext( pos, mapSettings );
}

void KadasSymbolItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.vertex >= 2 && context.vidx.vertex <= 5 )
  {
    QImageReader reader( mFilePath );

    QgsVector halfSize = mapSettings.mapToPixel().transform( newPoint ) - mapSettings.mapToPixel().transform( toMapPos( position(), mapSettings ) );
    halfSize = halfSize.rotateBy( state()->angle / 180. * M_PI ) / mSymbolScale;
    state()->size.setWidth( 2 * qAbs( halfSize.x() ) );
    state()->size.setHeight( state()->size.width() / double( reader.size().width() ) * reader.size().height() );

    reader.setBackgroundColor( Qt::white );
    reader.setScaledSize( state()->size );
    mImage = reader.read().convertToFormat( QImage::Format_ARGB32 );

    update();
  }
  else
  {
    KadasAnchoredItem::edit( context, newPoint, mapSettings );
  }
}


KadasPinItem::KadasPinItem()
  : KadasSymbolItem()
{
  setup( ":/kadas/icons/pin_red", 0.5, 1.0 );
  // TODO !!! connect( this, &KadasPinItem::changed, this, &KadasPinItem::updateTooltip );
}

void KadasPinItem::updateTooltip()
{
  QString posStr = KadasCoordinateFormat::instance()->getDisplayString( position(), crs() );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( QgsPointXY( position() ).toString() ).arg( crs().authid() );
  }
  QString toolTipText = QObject::tr( "<b>Position:</b> %1<br /><b>Height:</b> %2 %3<br /><b>Name:</b> %4<br /><b>Remarks:</b><br />%5" )
                          .arg( posStr )
                          .arg( KadasCoordinateFormat::instance()->getHeightAtPos( position(), crs() ) )
                          .arg( KadasCoordinateFormat::instance()->getHeightDisplayUnit() == Qgis::DistanceUnit::Feet ? "ft" : "m" )
                          .arg( name() )
                          .arg( remarks() );
  // TODO !!! blockSignals( true ); // block changed() signal as it would end up triggering updateTooltip again
  setTooltip( toolTipText );
  // TODO !!! blockSignals( false );
}
