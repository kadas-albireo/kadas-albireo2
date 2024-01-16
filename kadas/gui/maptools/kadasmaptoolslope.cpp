/***************************************************************************
    kadasmaptoolslope.h
    -------------------
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

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrastershader.h>
#include <qgis/qgssettings.h>
#include <qgis/qgssinglebandpseudocolorrenderer.h>

#include <kadas/analysis/kadasslopefilter.h>
#include <kadas/core/kadas.h>
#include <kadas/gui/mapitems/kadasrectangleitem.h>
#include <kadas/gui/maptools/kadasmaptoolslope.h>


KadasMapToolSlope::KadasMapToolSlope( QgsMapCanvas *mapCanvas )
  : KadasMapToolCreateItem( mapCanvas, itemFactory( mapCanvas ) )
{
  setCursor( Qt::ArrowCursor );
  setUndoRedoVisible( false );
  setToolLabel( tr( "Compute slope" ) );
  connect( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolSlope::drawFinished );
}

KadasMapToolCreateItem::ItemFactory KadasMapToolSlope::itemFactory( const QgsMapCanvas *canvas ) const
{
  return [ = ]
  {
    KadasRectangleItem *item = new KadasRectangleItem( canvas->mapSettings().destinationCrs() );
    return item;
  };
}

void KadasMapToolSlope::drawFinished()
{
  const KadasRectangleItem *rectItem = dynamic_cast<const KadasRectangleItem *>( currentItem() );
  if ( !rectItem )
  {
    return;
  }
  const QgsPointXY &p1 = rectItem->constState()->p1.front();
  const QgsPointXY &p2 = rectItem->constState()->p2.front();
  QgsRectangle rect( p1, p2 );
  rect.normalize();
  if ( rect.isEmpty() )
  {
    clear();
    return;
  }

  QgsCoordinateReferenceSystem rectCrs = canvas()->mapSettings().destinationCrs();

  compute( rect, rectCrs );

  clear();
}

void KadasMapToolSlope::compute( const QgsRectangle &extent, const QgsCoordinateReferenceSystem &crs )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != Qgis::LayerType::Raster )
  {
    emit messageEmitted( tr( "No heightmap is defined in the project. Right-click a raster layer in the layer tree and select it to be used as heightmap." ), Qgis::Warning );
    return;
  }

  QString outputFileName = QString( "slope_%1-%2_%3-%4.tif" ).arg( extent.xMinimum() ).arg( extent.xMaximum() ).arg( extent.yMinimum() ).arg( extent.yMaximum() );
  QString outputFile = QgsProject::instance()->createAttachedFile( outputFileName );

  KadasSlopeFilter slope( static_cast<QgsRasterLayer *>( layer ), outputFile, "GTiff", extent, crs );
  QProgressDialog p( tr( "Calculating slope..." ), tr( "Abort" ), 0, 0 );
  p.setWindowTitle( tr( "Slope" ) );
  p.setWindowModality( Qt::ApplicationModal );
  QApplication::setOverrideCursor( Qt::WaitCursor );
  QString errorMsg;
  int status = slope.processRaster( &p, errorMsg );
  QApplication::restoreOverrideCursor();

  if ( status != 0 && !errorMsg.isEmpty() )
  {
    emit messageEmitted( tr( "Error: %1" ).arg( errorMsg ), Qgis::Critical );
  }
  if ( status == 0 )
  {
    QgsRasterLayer *layer = new QgsRasterLayer( outputFile, tr( "Slope [%1]" ).arg( extent.toString( true ) ) );
    QgsColorRampShader *rampShader = new QgsColorRampShader();
    QList<QgsColorRampShader::ColorRampItem> colorRampItems = QList<QgsColorRampShader::ColorRampItem>()
        << QgsColorRampShader::ColorRampItem( 0, QColor( 43, 131, 186 ), QString::fromUtf8( "0°" ) )
        << QgsColorRampShader::ColorRampItem( 5, QColor( 99, 171, 176 ), QString::fromUtf8( "5°" ) )
        << QgsColorRampShader::ColorRampItem( 10, QColor( 156, 211, 166 ), QString::fromUtf8( "10°" ) )
        << QgsColorRampShader::ColorRampItem( 15, QColor( 199, 232, 173 ), QString::fromUtf8( "15°" ) )
        << QgsColorRampShader::ColorRampItem( 20, QColor( 236, 247, 185 ), QString::fromUtf8( "20°" ) )
        << QgsColorRampShader::ColorRampItem( 25, QColor( 254, 237, 170 ), QString::fromUtf8( "25°" ) )
        << QgsColorRampShader::ColorRampItem( 30, QColor( 253, 201, 128 ), QString::fromUtf8( "30°" ) )
        << QgsColorRampShader::ColorRampItem( 35, QColor( 248, 157, 89 ), QString::fromUtf8( "35°" ) )
        << QgsColorRampShader::ColorRampItem( 40, QColor( 231, 91, 58 ), QString::fromUtf8( "40°" ) )
        << QgsColorRampShader::ColorRampItem( 45, QColor( 215, 25, 28 ), QString::fromUtf8( "45°" ) );
    rampShader->setColorRampItemList( colorRampItems );
    QgsColorRampLegendNodeSettings *legendSettings = new QgsColorRampLegendNodeSettings( *rampShader->legendSettings() );
    legendSettings->setUseContinuousLegend( false );
    rampShader->setLegendSettings( legendSettings );
    QgsRasterShader *shader = new QgsRasterShader();
    shader->setRasterShaderFunction( rampShader );
    QgsSingleBandPseudoColorRenderer *renderer = new QgsSingleBandPseudoColorRenderer( 0, 1, shader );
    renderer->setClassificationMin( 0 );
    renderer->setClassificationMin( 255 );
    layer->setRenderer( renderer );
    QgsProject::instance()->addMapLayer( layer );
  }
}
