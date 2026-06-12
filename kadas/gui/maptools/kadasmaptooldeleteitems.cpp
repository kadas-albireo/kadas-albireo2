/***************************************************************************
    kadasmaptooldeleteitems.cpp
    ---------------------------
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

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsfeedback.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsrendercontext.h>
#include <qgis/qgsrubberband.h>

#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/maptools/kadasmaptooldeleteitems.h"


KadasMapToolDeleteItems::KadasMapToolDeleteItems( QgsMapCanvas *mapCanvas )
  : QgsMapToolExtent( mapCanvas )
{
  connect( this, &QgsMapToolExtent::extentChanged, this, &KadasMapToolDeleteItems::onExtentDrawn );
}

void KadasMapToolDeleteItems::onExtentDrawn( const QgsRectangle &extent )
{
  QgsRectangle rect = extent;
  rect.normalize();
  if ( !rect.isEmpty() )
  {
    deleteItems( rect );
  }
  clearRubberBand();
}

void KadasMapToolDeleteItems::activate()
{
  QgsMapToolExtent::activate();
  emit messageEmitted( tr( "Drag a rectangle around the items to delete" ) );
}

void KadasMapToolDeleteItems::deleteItems( const QgsRectangle &filterRect )
{
  QgsRenderContext rc = QgsRenderContext::fromMapSettings( canvas()->mapSettings() );
  QMap<QgsAnnotationLayer *, QStringList> delItems;

  for ( QgsMapLayer *layer : canvas()->layers() )
  {
    QgsAnnotationLayer *annoLayer = qobject_cast<QgsAnnotationLayer *>( layer );
    if ( !annoLayer )
    {
      continue;
    }
    // Parametric layers (bullseye, guide grid, ...) regenerate their items
    // from a configuration; deleting individual items makes no sense.
    if ( KadasAnnotationLayerHelpers::isParametricLayer( annoLayer ) )
    {
      continue;
    }
    const QgsRectangle layerBounds = canvas()->mapSettings().mapToLayerCoordinates( annoLayer, filterRect );
    QgsFeedback feedback;
    const QStringList hits = annoLayer->itemsInBounds( layerBounds, rc, &feedback );
    if ( !hits.isEmpty() )
    {
      delItems.insert( annoLayer, hits );
    }
  }

  if ( delItems.isEmpty() )
  {
    return;
  }

  // Highlight the candidate items on the canvas while the confirmation dialog is open
  QgsRubberBand highlightBand( canvas(), Qgis::GeometryType::Polygon );
  highlightBand.setStrokeColor( QColor( 255, 200, 0, 255 ) );
  highlightBand.setFillColor( QColor( 255, 200, 0, 63 ) );
  highlightBand.setWidth( 2 );
  for ( auto it = delItems.constBegin(), itEnd = delItems.constEnd(); it != itEnd; ++it )
  {
    QgsAnnotationLayer *annoLayer = it.key();
    for ( const QString &itemId : it.value() )
    {
      const QgsAnnotationItem *item = annoLayer->item( itemId );
      if ( !item )
      {
        continue;
      }
      const QgsRectangle bounds = canvas()->mapSettings().layerToMapCoordinates( annoLayer, item->boundingBox( rc ) );
      if ( !bounds.isEmpty() )
      {
        highlightBand.addGeometry( QgsGeometry::fromRect( bounds ), nullptr );
      }
    }
  }

  QMap<QgsAnnotationLayer *, QCheckBox *> checkboxes;
  QDialog confirmDialog;
  confirmDialog.setWindowTitle( tr( "Delete items" ) );
  confirmDialog.setLayout( new QVBoxLayout() );
  confirmDialog.layout()->addWidget( new QLabel( tr( "Do you want to delete the following items?" ) ) );
  for ( auto it = delItems.begin(), itEnd = delItems.end(); it != itEnd; ++it )
  {
    QCheckBox *checkbox = new QCheckBox( tr( "%1 item(s) from layer %2" ).arg( it.value().size() ).arg( it.key()->name() ) );
    checkbox->setChecked( true );
    confirmDialog.layout()->addWidget( checkbox );
    checkboxes.insert( it.key(), checkbox );
  }
  confirmDialog.layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
  QDialogButtonBox *bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  connect( bbox, &QDialogButtonBox::accepted, &confirmDialog, &QDialog::accept );
  connect( bbox, &QDialogButtonBox::rejected, &confirmDialog, &QDialog::reject );
  confirmDialog.layout()->addWidget( bbox );

  if ( confirmDialog.exec() != QDialog::Accepted )
  {
    return;
  }

  for ( auto it = checkboxes.begin(), itEnd = checkboxes.end(); it != itEnd; ++it )
  {
    if ( !it.value()->isChecked() )
    {
      continue;
    }
    QgsAnnotationLayer *layer = it.key();
    for ( const QString &itemId : delItems[layer] )
    {
      layer->removeItem( itemId );
    }
    layer->triggerRepaint();
  }
  canvas()->refresh();
}
