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

#include <qgis/qgsannotation.h>
#include <qgis/qgsannotationmanager.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapcanvasannotationitem.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrenderer.h>

#include <kadas/core/kadaspluginlayer.h>
#include <kadas/core/kadasredlininglayer.h>

#include "kadasmaptooldeleteitems.h"


KadasMapToolDeleteItems::KadasMapToolDeleteItems( QgsMapCanvas* mapCanvas )
    : KadasMapToolDrawRectangle( mapCanvas )
{
  setCursor( Qt::ArrowCursor );
  connect( this, SIGNAL( finished() ), this, SLOT( drawFinished() ) );
}

void KadasMapToolDeleteItems::drawFinished()
{
  QgsPointXY p1, p2;
  getPart( 0, p1, p2 );
  QgsRectangle filterRect( p1, p2 );
  filterRect.normalize();
  if ( filterRect.isEmpty() )
  {
    reset();
    return;
  }

  QgsCoordinateReferenceSystem filterRectCrs = canvas()->mapSettings().destinationCrs();

  deleteItems( filterRect, filterRectCrs );
  reset();
}

void KadasMapToolDeleteItems::activate()
{
  emit messageEmitted(tr( "Drag a rectangle around the items to delete" ));
}

void KadasMapToolDeleteItems::deleteItems( const QgsRectangle &filterRect, const QgsCoordinateReferenceSystem &filterRectCrs )
{
  // Search annotation items
  QList<QgsAnnotation*> delAnnotations;
  for ( QGraphicsItem* item : canvas()->scene()->items() )
  {
    QgsMapCanvasAnnotationItem* aitem = dynamic_cast<QgsMapCanvasAnnotationItem*>( item );
    if ( aitem && aitem->annotation()->hasFixedMapPosition() )
    {
      QgsAnnotation* annotation = aitem->annotation();
      QgsPointXY p = QgsCoordinateTransform( annotation->mapPositionCrs(), filterRectCrs, QgsProject::instance() ).transform( annotation->mapPosition() );
      if ( filterRect.contains( p ) )
      {
        delAnnotations.append( annotation );
      }
    }
  }

  // Search redlining and plugin layers
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas()->mapSettings() );
  QMap<KadasRedliningLayer*, QgsFeatureIds> delRedliningItems;
  QMap<KadasPluginLayer*, QVariantList> delPluginItems;
  for ( QgsMapLayer* layer : canvas()->layers() )
  {
    if ( qobject_cast<KadasRedliningLayer*>(layer) )
    {
      KadasRedliningLayer* rlayer = static_cast<KadasRedliningLayer*>( layer );

      if ( rlayer->hasScaleBasedVisibility() &&
           ( rlayer->minimumScale() > canvas()->mapSettings().scale() ||
             rlayer->maximumScale() <= canvas()->mapSettings().scale() ) )
      {
        continue;
      }


      QgsFeatureRenderer* renderer = rlayer->renderer();
      bool filteredRendering = false;
      if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
      {
        // setup scale for scale dependent visibility (rule based)
        renderer->startRender( renderContext, rlayer->fields() );
        filteredRendering = renderer->capabilities() & QgsFeatureRenderer::Filter;
      }

      QgsRectangle layerFilterRect = QgsCoordinateTransform( filterRectCrs, rlayer->crs(), QgsProject::instance() ).transform( filterRect );
      QgsFeatureIterator fit = rlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
      QgsFeature feature;
      while ( fit.nextFeature( feature ) )
      {
        if ( filteredRendering && !renderer->willRenderFeature( feature, renderContext ) )
        {
          continue;
        }
        delRedliningItems[rlayer].insert( feature.id() );
      }
      if ( renderer && renderer->capabilities() & QgsFeatureRenderer::ScaleDependent )
      {
        renderer->stopRender( renderContext );
      }
    }
    else if ( qobject_cast<KadasPluginLayer*>(layer) )
    {
      KadasPluginLayer* player = static_cast<KadasPluginLayer*>( layer );
      QgsRectangle layerFilterRect = QgsCoordinateTransform( filterRectCrs, player->crs(), QgsProject::instance() ).transform( filterRect );
      QVariantList items = player->getItems( layerFilterRect );
      if ( !items.isEmpty() )
      {
        delPluginItems[player] = items;
      }
    }
  }

  if ( !delAnnotations.isEmpty() || !delRedliningItems.isEmpty() || !delPluginItems.isEmpty() )
  {
    QCheckBox* checkBoxAnnotationItems = 0;
    QMap<KadasRedliningLayer*, QCheckBox*> checkBoxRedliningItems;
    QMap<KadasPluginLayer*, QCheckBox*> checkBoxPluginItems;
    QDialog confirmDialog;
    confirmDialog.setWindowTitle( tr( "Delete items" ) );
    confirmDialog.setLayout( new QVBoxLayout() );
    confirmDialog.layout()->addWidget( new QLabel( tr( "Do you want to delete the following items?" ) ) );
    if ( !delAnnotations.isEmpty() )
    {
      checkBoxAnnotationItems = new QCheckBox( tr( "%1 annotation item(s)" ).arg( delAnnotations.size() ) );
      checkBoxAnnotationItems->setChecked( true );
      confirmDialog.layout()->addWidget( checkBoxAnnotationItems );
    }
    for( KadasRedliningLayer* layer : delRedliningItems.keys() )
    {
      QCheckBox* checkBox = new QCheckBox( tr( "%1 items(s) from layer %2" ).arg( delRedliningItems[layer].size() ).arg( layer->name() ) );
      checkBox->setChecked( true );
      checkBoxRedliningItems[layer] = checkBox;
      confirmDialog.layout()->addWidget( checkBox );
    }
    for ( KadasPluginLayer* layer : delPluginItems.keys() )
    {
      QCheckBox* checkBox = new QCheckBox( tr( "%1 items(s) from layer %2" ).arg( delPluginItems[layer].size() ).arg( layer->name() ) );
      checkBox->setChecked( true );
      checkBoxPluginItems[layer] = checkBox;
      confirmDialog.layout()->addWidget( checkBox );
    }
    confirmDialog.layout()->addItem( new QSpacerItem( 1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
    connect( bbox, SIGNAL( accepted() ), &confirmDialog, SLOT( accept() ) );
    connect( bbox, SIGNAL( rejected() ), &confirmDialog, SLOT( reject() ) );
    confirmDialog.layout()->addWidget( bbox );
    if ( confirmDialog.exec() == QDialog::Accepted )
    {
      if ( checkBoxAnnotationItems && checkBoxAnnotationItems->isChecked() )
      {
        for(QgsAnnotation* annotation : delAnnotations) {
          QgsProject::instance()->annotationManager()->removeAnnotation(annotation);
        }
      }
      for ( KadasRedliningLayer* layer : checkBoxRedliningItems.keys() )
      {
        if ( checkBoxRedliningItems[layer]->isChecked() )
        {
          layer->dataProvider()->deleteFeatures( delRedliningItems[layer] );
        }
        layer->triggerRepaint();
      }
      for ( KadasPluginLayer* layer : checkBoxPluginItems.keys() )
      {
        if ( checkBoxPluginItems[layer]->isChecked() )
        {
          layer->deleteItems( delPluginItems[layer] );
        }
        layer->triggerRepaint();
      }
    }
  }
}
