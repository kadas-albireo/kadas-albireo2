/***************************************************************************
    kadasmaptoolhillshade.cpp
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

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QProgressDialog>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrasterlayer.h>
#include <qgis/qgsrasterrenderer.h>
#include <qgis/qgssettings.h>

#include <kadas/analysis/kadashillshadefilter.h>
#include <kadas/gui/mapitems/kadasrectangleitem.h>
#include <kadas/gui/maptools/kadasmaptoolhillshade.h>



KadasMapToolHillshade::KadasMapToolHillshade( QgsMapCanvas *mapCanvas )
  : KadasMapToolCreateItem( mapCanvas, itemFactory( mapCanvas ) )
{
  setCursor( Qt::ArrowCursor );
  setUndoRedoVisible( false );
  setToolLabel( tr( "Compute hillshade" ) );
  connect( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolHillshade::drawFinished );
}

KadasMapToolCreateItem::ItemFactory KadasMapToolHillshade::itemFactory( const QgsMapCanvas *canvas ) const
{
  return [ = ]
  {
    KadasRectangleItem *item = new KadasRectangleItem( canvas->mapSettings().destinationCrs() );
    return item;
  };
}

void KadasMapToolHillshade::drawFinished()
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

void KadasMapToolHillshade::compute( const QgsRectangle &extent, const QgsCoordinateReferenceSystem &crs )
{
  QString layerid = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  QgsMapLayer *layer = QgsProject::instance()->mapLayer( layerid );
  if ( !layer || layer->type() != QgsMapLayerType::RasterLayer )
  {
    emit messageEmitted( tr( "No heightmap is defined in the project." ), Qgis::Warning );
    return;
  }

  QDialog anglesDialog;
  anglesDialog.setWindowTitle( tr( "Hillshade setup" ) );
  QGridLayout *anglesDialogLayout = new QGridLayout();
  anglesDialogLayout->addWidget( new QLabel( tr( "Azimuth (horizontal angle):" ) ), 0, 0, 1, 1 );
  anglesDialogLayout->addWidget( new QLabel( tr( "Vertical angle:" ) ), 1, 0, 1, 1 );
  QDoubleSpinBox *spinHorAngle = new QDoubleSpinBox();
  spinHorAngle ->setRange( 0, 359.9 );
  spinHorAngle ->setDecimals( 1 );
  spinHorAngle ->setValue( 315 );
  spinHorAngle->setWrapping( true );
  spinHorAngle ->setSuffix( QChar( 0x00B0 ) );
  anglesDialogLayout->addWidget( spinHorAngle, 0, 1, 1, 1 );
  QDoubleSpinBox *spinVerAngle = new QDoubleSpinBox();
  spinVerAngle->setRange( 0, 90. );
  spinVerAngle->setDecimals( 1 );
  spinVerAngle->setValue( 60. );
  spinVerAngle->setSuffix( QChar( 0x00B0 ) );
  anglesDialogLayout->addWidget( spinVerAngle, 1, 1, 1, 1 );
  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, &QDialogButtonBox::accepted, &anglesDialog, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, &anglesDialog, &QDialog::reject );
  anglesDialogLayout->addWidget( bbox, 2, 0, 1, 2 );
  anglesDialog.setLayout( anglesDialogLayout );
  anglesDialog.setFixedSize( anglesDialog.sizeHint() );
  if ( anglesDialog.exec() == QDialog::Rejected )
  {
    return;
  }

  QString outputFileName = QString( "hillshade_%1-%2_%3-%4.tif" ).arg( extent.xMinimum() ).arg( extent.xMaximum() ).arg( extent.yMinimum() ).arg( extent.yMaximum() );
  QString outputFile = QgsProject::instance()->createAttachedFile( outputFileName );

  KadasHillshadeFilter hillshade( layer->source(), outputFile, "GTiff", spinHorAngle->value(), spinVerAngle->value(), extent, crs );
  QProgressDialog p( tr( "Calculating hillshade..." ), tr( "Abort" ), 0, 0 );
  p.setWindowTitle( tr( "Hillshade" ) );
  p.setWindowModality( Qt::ApplicationModal );
  QApplication::setOverrideCursor( Qt::WaitCursor );
  hillshade.processRaster( &p );
  QApplication::restoreOverrideCursor();
  if ( !p.wasCanceled() )
  {
    QgsRasterLayer *layer = new QgsRasterLayer( outputFile, tr( "Hillshade [%1]" ).arg( extent.toString( true ) ) );
    if ( layer->isValid() && layer->renderer() )
    {
      layer->renderer()->setOpacity( 0.6 );
      QgsProject::instance()->addMapLayer( layer );
    }
  }
}
