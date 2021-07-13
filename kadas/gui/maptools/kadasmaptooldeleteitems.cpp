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

#include <qgis/qgsmapcanvas.h>

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasmapcanvasitem.h>
#include <kadas/gui/mapitems/kadasrectangleitem.h>
#include <kadas/gui/maptools/kadasmaptooldeleteitems.h>


KadasMapToolDeleteItems::KadasMapToolDeleteItems( QgsMapCanvas *mapCanvas )
  : KadasMapToolCreateItem( mapCanvas, itemFactory( mapCanvas ) )
{
  connect( this, &KadasMapToolCreateItem::partFinished, this, &KadasMapToolDeleteItems::drawFinished );

  setToolLabel( tr( "Delete map items" ) );
  setUndoRedoVisible( false );
}

KadasMapToolDeleteItems::ItemFactory KadasMapToolDeleteItems::itemFactory( QgsMapCanvas *canvas ) const
{
  return [ = ]
  {
    KadasRectangleItem *item = new KadasRectangleItem( canvas->mapSettings().destinationCrs() );
    item->setFill( Qt::NoBrush );
    item->setOutline( QPen( Qt::black, 2, Qt::DashLine ) );
    return item;
  };
}

void KadasMapToolDeleteItems::drawFinished()
{
  const KadasRectangleItem *item = dynamic_cast<const KadasRectangleItem *>( currentItem() );
  if ( !item || item->constState()->p1.isEmpty() || item->constState()->p2.isEmpty() )
  {
    return;
  }
  QgsRectangle filterRect( item->constState()->p1.front(), item->constState()->p2.front() );
  filterRect.normalize();
  if ( !filterRect.isEmpty() )
  {
    KadasMapRect rect( filterRect.xMinimum(), filterRect.yMinimum(), filterRect.xMaximum(), filterRect.yMaximum() );
    deleteItems( rect );
  }
  clear();
}

void KadasMapToolDeleteItems::activate()
{
  KadasMapToolCreateItem::activate();
  emit messageEmitted( tr( "Drag a rectangle around the items to delete" ) );
}

void KadasMapToolDeleteItems::deleteItems( const KadasMapRect &filterRect )
{
  QMap<KadasItemLayer *, QList<KadasItemLayer::ItemId>> delItems;
  QList<KadasMapItem *> mapItems;
  QList<KadasMapCanvasItem *> mapCanvasItems;

  for ( QgsMapLayer *layer : canvas()->layers() )
  {
    KadasItemLayer *itemLayer = dynamic_cast<KadasItemLayer *>( layer );
    if ( !itemLayer )
    {
      continue;
    }
    for ( auto it = itemLayer->items().begin(), itEnd = itemLayer->items().end(); it != itEnd; ++it )
    {
      KadasMapItem *item = it.value();
      if ( item->intersects( filterRect, canvas()->mapSettings(), true ) )
      {
        delItems[itemLayer].append( it.key() );
        item->setSelected( true );
        mapItems.append( item );
        mapCanvasItems.append( new KadasMapCanvasItem( item, canvas() ) );
      }
    }
  }

  if ( !delItems.isEmpty() )
  {
    QMap<KadasItemLayer *, QCheckBox *> checkboxes;
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

    int response = confirmDialog.exec();
    qDeleteAll( mapCanvasItems );
    if ( response == QDialog::Accepted )
    {
      for ( auto it = checkboxes.begin(), itEnd = checkboxes.end(); it != itEnd; ++it )
      {
        if ( it.value()->isChecked() )
        {
          KadasItemLayer *layer = it.key();
          for ( const KadasItemLayer::ItemId &itemId : delItems[layer] )
          {
            delete layer->takeItem( itemId );
          }
          layer->triggerRepaint( true );
        }
      }
      canvas()->refresh();
    }
    else
    {
      for ( KadasMapItem *item : mapItems )
      {
        item->setSelected( false );
      }
    }
  }
}
