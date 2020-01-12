/***************************************************************************
    kadaskmlexport.cpp
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
#include <QProgressDialog>
#include <QIODevice>
#include <QTextStream>
#include <QUuid>
#include <quazip5/quazipfile.h>

#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgsvectorlayer.h>
#include <qgis/qgsvectorlayerlabeling.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/app/kml/kadaskmlexport.h>
#include <kadas/app/kml/kadaskmllabeling.h>

bool KadasKMLExport::exportToFile( const QString &filename, const QList<QgsMapLayer *> &layers, double exportScale, const QgsCoordinateReferenceSystem &mapCrs, const QgsRectangle &exportMapRect )
{
  // Prepare outputs
  bool kmz = filename.endsWith( ".kmz", Qt::CaseInsensitive );
  QuaZip *quaZip = nullptr;
  if ( kmz )
  {
    quaZip = new QuaZip( filename );
    if ( !quaZip->open( QuaZip::mdCreate ) )
    {
      delete quaZip;
      return false;
    }
  }
  QString outString;
  QTextStream outStream( &outString );

  QProgressDialog progress( tr( "Plase wait..." ), tr( "Cancel" ), 0, 0 );
  progress.setWindowModality( Qt::ApplicationModal );
  progress.setWindowTitle( tr( "KML Export" ) );
  progress.show();
  QApplication::processEvents();

  // Write document header
  outStream << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << "\n";
  outStream << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << "\n";
  outStream << "<Document>" << "\n";

  // Write schemas
  for ( QgsMapLayer *ml : layers )
  {
    QgsVectorLayer *vl = dynamic_cast<QgsVectorLayer *>( ml );
    if ( vl )
    {
      outStream << QString( "<Schema name=\"%1\" id=\"%1\">" ).arg( vl->name() ) << "\n";
      const QgsFields &layerFields = vl->fields();
      for ( int i = 0; i < layerFields.size(); ++i )
      {
        const QgsField &field = layerFields.at( i );
        outStream << QString( "<SimpleField name=\"%1\" type=\"%2\"></SimpleField>" ).arg( field.name() ).arg( QVariant::typeToName( field.type() ) ) << "\n";
      }
      outStream << QString( "</Schema> " ) << "\n";
    }
  }

  // Prepare rendering
  QgsRectangle fullExtent;
  QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
  for ( QgsMapLayer *ml : layers )
  {
    QgsRectangle layerExtent = ml->extent();
    if ( !exportMapRect.isEmpty() )
    {
      QgsCoordinateTransform crst( mapCrs, ml->crs(), QgsProject::instance() );
      layerExtent = layerExtent.intersect( crst.transformBoundingBox( exportMapRect ) );
    }
    layerExtent = QgsCoordinateTransform( ml->crs(), crsWgs84, QgsProject::instance() ).transformBoundingBox( layerExtent );
    if ( fullExtent.isEmpty() )
      fullExtent = layerExtent;
    else
      fullExtent.combineExtentWith( layerExtent );
  }
  QgsMapSettings settings;
  settings.setDestinationCrs( crsWgs84 );
  settings.setExtent( fullExtent );
  settings.setOutputDpi( 96 );
  int dpi = 96;
  double factor = QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::DistanceDegrees, QgsUnitTypes::DistanceMeters ) * dpi / 25.4 * 1000 / exportScale;
  settings.setOutputSize( QSize( fullExtent.width() * factor, fullExtent.height() * factor ) );
  settings.setOutputDpi( dpi );

  QgsDefaultLabelingEngine engine;
  engine.setMapSettings( settings );

  QgsRenderContext rc = QgsRenderContext::fromMapSettings( settings );

  QImage image( 10, 10, QImage::Format_ARGB32_Premultiplied );
  image.setDotsPerMeterX( 96 / 25.4 * 1000 );
  image.setDotsPerMeterY( 96 / 25.4 * 1000 );
  QPainter painter( &image );
  rc.setPainter( &painter );
  rc.setRendererScale( exportScale );
  rc.setExtent( fullExtent );
  rc.setMapExtent( fullExtent );
  rc.setScaleFactor( 96.0 / 25.4 );
  rc.setMapToPixel( QgsMapToPixel( 1.0 / factor, fullExtent.center().x(), fullExtent.center().y(),
                                   fullExtent.width() * factor, fullExtent.height() * factor, 0 ) );
  rc.setCustomRenderFlags( QStringList() << "kml" );

  // Render layers
  int drawingOrder = 0;
  for ( QgsMapLayer *ml : layers )
  {
    QgsRectangle exportRect;
    if ( !exportMapRect.isEmpty() )
    {
      exportRect = QgsCoordinateTransform( mapCrs, ml->crs(), QgsProject::instance() ).transformBoundingBox( exportMapRect );
    }
    QgsCoordinateTransform crst84( ml->crs(), crsWgs84, QgsProject::instance() );
    rc.setCoordinateTransform( crst84 );

    if ( dynamic_cast<QgsVectorLayer *>( ml ) )
    {
      progress.setLabelText( tr( "Rendering layer %1..." ).arg( ml->name() ) );
      progress.setRange( 0, 0 );
      QApplication::processEvents();
      if ( progress.wasCanceled() )
      {
        delete quaZip;
        return false;
      }

      QgsVectorLayer *vl = static_cast<QgsVectorLayer *>( ml );

      QStringList attributes;
      const QgsFields &fields = vl->fields();
      for ( int i = 0; i < fields.size(); ++i )
      {
        attributes.append( fields.at( i ).name() );
      }

      outStream << "<Folder>" << "\n";
      outStream << "<name>" << vl->name() << "</name>" << "\n";
      writeVectorLayerFeatures( vl, outStream, rc, engine, exportRect );
      outStream << "</Folder>" << "\n";
    }
    else if ( dynamic_cast<KadasItemLayer *>( ml ) )
    {
      outStream << static_cast<KadasItemLayer *>( ml )->asKml( rc, quaZip, exportRect );
    }
    else if ( kmz && dynamic_cast<QgsRasterLayer *>( ml ) ) // Non-vector layers only supported in KMZ
    {
      QgsRectangle layerExtent = ml->extent();
      if ( !exportRect.isEmpty() )
      {
        layerExtent = layerExtent.intersect( exportRect );
      }
      layerExtent = crst84.transform( layerExtent );

      progress.setLabelText( tr( "Rendering layer %1..." ).arg( ml->name() ) );
      progress.setRange( 0, 0 );
      QApplication::processEvents();
      if ( progress.wasCanceled() )
      {
        delete quaZip;
        return false;
      }

      outStream << "<Folder>" << "\n";
      outStream << "<name>" << ml->name() << "</name>" << "\n";
#if 0
      QgsRasterLayer *rl = dynamic_cast<QgsRasterLayer *>( ml );
      if ( rl && rl->providerType() == "wms" )
      {
        QMap<QString, QString> parameterMap;
        for ( const QString &param : rl->source().split( "&" ) )
        {
          QStringList pair = param.split( "=" );
          if ( pair.size() >= 2 )
            parameterMap.insert( pair[0].toUpper(), pair[1] );
        }
        QString baseUrl = parameterMap.value( "URL" );
        QString format = parameterMap.value( "FORMAT", "PNG" );
        QString layers = parameterMap.value( "LAYERS" );
        QString styles = parameterMap.value( "STYLES" );
        QString href = baseUrl + "SERVICE=WMS&amp;VERSION=1.1.1&amp;SRS=EPSG:4326&amp;REQUEST=GetMap&amp;TRANSPARENT=TRUE&amp;WIDTH=512&amp;HEIGHT=512&amp;FORMAT=" + format + "&amp;LAYERS=" + layers + "&amp;STYLES=" + styles;
        writeGroundOverlay( outStream, ml->name(), href, layerExtent, ++drawingOrder );
      }
      else
      {
        writeTiles( ml, layerExtent, exportScale, outStream, ++drawingOrder, quaZip, &progress );
      }
#else
      writeTiles( ml, layerExtent, exportScale, outStream, ++drawingOrder, quaZip, &progress );
#endif

      outStream << "</Folder>" << "\n";
    }
    if ( progress.wasCanceled() )
    {
      delete quaZip;
      return false;
    }
  }

  engine.run( rc );

  outStream << "</Document>" << "\n";
  outStream << "</kml>";
  outStream.flush();

  // Write output
  bool success = false;
  if ( !kmz )
  {
    QFile outFile( filename );
    if ( outFile.open( QIODevice::WriteOnly ) )
    {
      outFile.write( outString.toUtf8() );
      success = true;
    }
  }
  else // KMZ
  {
    QuaZipFile outZipFile( quaZip );
    QuaZipNewInfo info( QFileInfo( filename ).baseName() + ".kml" );
    info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
    if ( outZipFile.open( QIODevice::WriteOnly, info ) )
    {
      outZipFile.write( outString.toUtf8() );
      outZipFile.close();
      success = true;
    }
    delete quaZip;
  }
  return success;
}

void KadasKMLExport::writeVectorLayerFeatures( QgsVectorLayer *vl, QTextStream &outStream, QgsRenderContext &rc, QgsLabelingEngine &labelingEngine, const QgsRectangle &exportRect )
{
  QgsFeatureRenderer *renderer = vl->renderer();
  if ( !renderer )
  {
    return;
  }
  renderer->startRender( rc, vl->fields() );

  QgsCoordinateTransform ct( vl->crs(), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );

  QgsFeatureRequest request;
  if ( !exportRect.isEmpty() )
  {
    request.setFilterRect( exportRect );
  }
  QgsFeatureIterator it = vl->getFeatures( request );
  QgsFeature f;
  QgsExpressionContext ectx;
  QgsExpression expr( QString( "\"%1\"" ).arg( vl->customProperty( "labeling/fieldName" ).toString() ) );

  while ( it.nextFeature( f ) )
  {
    outStream << "<Placemark>" << "\n";
    if ( f.geometry().get() )
    {
      // Name (from labeling expression, if possible)
      if ( vl->customProperty( "labeling/enabled", false ).toBool() && !vl->customProperty( "labeling/fieldName" ).isNull() )
      {
        ectx.setFeature( f );
        outStream << QString( "<name>%1</name>\n" ).arg( expr.evaluate( &ectx ).toString() );
      }
      else
      {
        outStream << QString( "<name>Feature %1</name>\n" ).arg( f.id() );
      }

      // Style
      addStyle( outStream, f, *renderer, rc );

      // Attributes
      outStream << QString( "<ExtendedData><SchemaData schemaUrl=\"#%1\">" ).arg( vl->name() ) << "\n";
      const QgsFields &fields = f.fields();
      const QgsAttributes &attributes = f.attributes();
      for ( int i = 0, n = attributes.size(); i < n; ++i )
      {
        outStream << QString( "<SimpleData name=\"%1\">%2</SimpleData>" ).arg( fields[i].name() ).arg( attributes[i].toString() ) << "\n";
      }
      outStream << QString( "</SchemaData></ExtendedData>" );

      // Segmentize immediately, since otherwise for instance circles are distorted if segmentation occurs after transformation
      QgsAbstractGeometry *geom = f.geometry().get()->segmentize();
      if ( geom )
      {
        geom->transform( ct ); //KML must be WGS84
        outStream << geom->asKML( 6 );
      }
      delete geom;
    }
    outStream << "</Placemark>" << "\n";

    const QgsAbstractVectorLayerLabeling *labeling = vl->labelsEnabled() ? vl->labeling() : nullptr;
    if ( labeling )
    {
      QgsPalLayerSettings settings = labeling->settings();
      labelingEngine.addProvider( new KadasKMLLabelProvider( &outStream, vl, &settings ) );
    }
  }
  renderer->stopRender( rc );
}

void KadasKMLExport::writeTiles( QgsMapLayer *mapLayer, const QgsRectangle &layerExtent, double exportScale, QTextStream &outStream, int drawingOrder, QuaZip *quaZip, QProgressDialog *progress )
{
  const int tileSize = 512;

  // Make extent square
  QgsPointXY center = layerExtent.center();
  double extension = qMax( layerExtent.width(), layerExtent.height() ) * 0.5;
  QgsRectangle renderExtent = QgsRectangle( center.x() - extension, center.y() - extension, center.x() + extension, center.y() + extension );

  // Compute pixels to match extent at scale
  // px / dpi * 0.0254 * scale = meters
  QImage image( tileSize, tileSize, QImage::Format_ARGB32 );
  double meters = QgsScaleCalculator().calculateGeographicDistance( renderExtent );
  int dpi = image.logicalDpiX();
  int totPixels = meters / ( exportScale * 0.0254 ) * dpi;
  double resolution = renderExtent.width() / totPixels;

  // Round up to next <tileSize> multiple
  totPixels = qCeil( ( totPixels ) / double( tileSize ) ) * tileSize;
  extension = totPixels * resolution * 0.5;
  renderExtent = QgsRectangle( center.x() - extension, center.y() - extension, center.x() + extension, center.y() + extension );

  progress->setRange( 0, ( totPixels / tileSize ) * ( totPixels / tileSize ) );
  QApplication::processEvents();

  // Render in <tileSize> blocks
  int tileCounter = 0;
  for ( int iy = 0; iy < totPixels; iy += tileSize )
  {
    for ( int ix = 0; ix < totPixels; ix += tileSize )
    {

      progress->setValue( tileCounter );
      QApplication::processEvents();
      if ( progress->wasCanceled() )
      {
        return;
      }

      QgsRectangle tileExtent( renderExtent.xMinimum() + ix * resolution, renderExtent.yMinimum() + iy * resolution,
                               renderExtent.xMinimum() + ( ix + tileSize ) * resolution, renderExtent.yMinimum() + ( iy + tileSize ) * resolution );
      if ( renderTile( image, tileExtent, mapLayer ) )
      {
        QString filename = QString( "%1_%2.png" ).arg( mapLayer->id() ).arg( tileCounter++ );
        QuaZipFile outputFile( quaZip );
        QuaZipNewInfo info( filename );
        info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
        if ( outputFile.open( QIODevice::WriteOnly, info ) && image.save( &outputFile, "PNG" ) )
          writeGroundOverlay( outStream, QString( "Tile %1" ).arg( tileExtent.toString( 3 ) ), filename, tileExtent, drawingOrder );
      }
    }
  }
}

void KadasKMLExport::writeGroundOverlay( QTextStream &outStream, const QString &name, const QString &href, const QgsRectangle &latLongBox, int drawingOrder )
{
  outStream << "<GroundOverlay>" << "\n";
  outStream << "<name>" << name << "</name>" << "\n";
  outStream << "<drawOrder>" << QString::number( drawingOrder ) << "</drawOrder>" << "\n";
  outStream << "<Icon><href>" << href << "</href></Icon>" << "\n";
  outStream << "<LatLonBox>" << "\n";
  outStream << "<north>" << latLongBox.yMaximum() << "</north>" << "\n";
  outStream << "<south>" << latLongBox.yMinimum() << "</south>" << "\n";
  outStream << "<east>" << latLongBox.xMaximum() << "</east>" << "\n";
  outStream << "<west>" << latLongBox.xMinimum() << "</west>" << "\n";
  outStream << "</LatLonBox>" << "\n";
  outStream << "</GroundOverlay>" << "\n";
}


bool KadasKMLExport::renderTile( QImage &img, const QgsRectangle &extent, QgsMapLayer *mapLayer )
{
  QgsCoordinateTransform crst = QgsCoordinateTransform( mapLayer->crs(), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
  QgsRenderContext context;
  img.fill( 0 );
  QPainter p( &img );
  context.setPainter( &p );
  context.setCoordinateTransform( crst );
  QgsPointXY centerPoint = extent.center();
  QgsMapToPixel mtp( extent.width() / img.width(), centerPoint.x(), centerPoint.y(), img.width(), img.height(), 0.0 );
  context.setMapToPixel( mtp );
  context.setExtent( crst.transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform ) );
  context.setCustomRenderFlags( QStringList() << "kml" );
  QgsMapLayerRenderer *layerRenderer = mapLayer->createMapRenderer( context );
  bool rendered = false;
  if ( layerRenderer )
  {
    rendered = layerRenderer->render();
  }
  delete layerRenderer;
  return rendered;
}

void KadasKMLExport::addStyle( QTextStream &outStream, QgsFeature &f, QgsFeatureRenderer &r, QgsRenderContext &rc )
{
  // Take first symbollayer
  QgsSymbolList symbolList = r.symbolsForFeature( f, rc );
  if ( symbolList.isEmpty() )
  {
    return;
  }

  QgsSymbol *s = symbolList.first();
  if ( !s || s->symbolLayerCount() < 1 )
  {
    return;
  }

  outStream << "<Style>";
  if ( s->type() == QgsSymbol::Line )
  {
    double width = 1;
    if ( dynamic_cast<QgsLineSymbolLayer *>( s->symbolLayer( 0 ) ) )
    {
      QgsLineSymbolLayer *lineSymbolLayer = static_cast<QgsLineSymbolLayer *>( s->symbolLayer( 0 ) );
      width = rc.convertToPainterUnits( lineSymbolLayer->width(), lineSymbolLayer->widthUnit() );
    }

    QColor c = s->symbolLayer( 0 )->color();

    outStream << QString( "<LineStyle><color>%1</color><width>%2</width></LineStyle>" ).arg( convertColor( c ) ).arg( QString::number( width ) );
  }
  else if ( s->type() == QgsSymbol::Fill )
  {
    double width = 1;

    QColor outlineColor = s->symbolLayer( 0 )->strokeColor();
    QColor fillColor = s->symbolLayer( 0 )->fillColor();

    int fill = 1; // TODO?

    outStream << QString( "<LineStyle><width>%1</width><color>%2</color></LineStyle><PolyStyle><fill>%3</fill><color>%4</color></PolyStyle>" )
              .arg( width ).arg( convertColor( outlineColor ) ).arg( fill ).arg( convertColor( fillColor ) );
  }
  outStream << "</Style>\n";
}

QString KadasKMLExport::convertColor( const QColor &c )
{
  return QString( "%1%2%3%4" )
         .arg( c.alpha(), 2, 16, QChar( '0' ) )
         .arg( c.blue(), 2, 16, QChar( '0' ) )
         .arg( c.green(), 2, 16, QChar( '0' ) )
         .arg( c.red(), 2, 16, QChar( '0' ) );
}
