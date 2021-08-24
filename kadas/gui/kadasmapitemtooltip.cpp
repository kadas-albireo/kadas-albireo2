/***************************************************************************
    kadasmapitemtooltip.cpp
    -----------------------
    copyright            : (C) 2021 by Sandro Mani
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

#include <QUrlQuery>

#include <qgis/qgsmapcanvas.h>

#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/kadasmapcanvasitem.h>
#include <kadas/gui/kadasmapitemtooltip.h>
#include <kadas/gui/mapitems/kadasmapitem.h>

KadasMapItemTooltip::KadasMapItemTooltip( QgsMapCanvas *canvas )
  : QTextBrowser( canvas )
  , mCanvas( canvas )
{
  setObjectName( "TooltipWidget" );
  setOpenExternalLinks( true );
  mShowTimer.setSingleShot( true );
  mHideTimer.setSingleShot( true );
  setReadOnly( true );
  connect( &mShowTimer, &QTimer::timeout, this, &KadasMapItemTooltip::positionAndShow );
  connect( &mHideTimer, &QTimer::timeout, this, &KadasMapItemTooltip::hide );
  setFixedSize( sWidth, sHeight );
  hide();
}

void KadasMapItemTooltip::updateForPos( const QPoint &canvasPos )
{
  const KadasMapItem *item = nullptr;

  QGraphicsItem *canvasItem = mCanvas->itemAt( canvasPos );
  if ( dynamic_cast<KadasMapCanvasItem *>( canvasItem ) )
  {
    KadasMapCanvasItem *mapCanvasItem = static_cast<KadasMapCanvasItem *>( canvasItem );
    item = mapCanvasItem->mapItem();
  }
  else
  {
    KadasFeaturePicker::PickResult result = KadasFeaturePicker::pick( mCanvas, mCanvas->getCoordinateTransform()->toMapCoordinates( canvasPos ), QgsWkbTypes::UnknownGeometry, KadasItemLayer::PICK_OBJECTIVE_TOOLTIP );
    if ( result.itemId != KadasItemLayer::ITEM_ID_NULL )
    {
      item = static_cast<KadasItemLayer *>( result.layer )->items()[result.itemId];
    }
  }

  // If hovering over an item, update/show tooltip
  if ( item )
  {
    mHideTimer.stop();
    mPos = canvasPos;
    if ( mItem != item )
    {
      mItem = item;
      setText( item->tooltip() );
      if ( isVisible() )
      {
        hide();
      }
    }
    if ( !item->tooltip().isEmpty() )
    {
      mShowTimer.start( 500 );
    }
  }
  else if ( isVisible() )
  {
    mHideTimer.start( 500 );
    mShowTimer.stop();
  }
  else
  {
    clear();
  }
}

QVariant KadasMapItemTooltip::loadResource( int type, const QUrl &url )
{
  if ( type == QTextDocument::ImageResource )
  {
    if ( url.scheme() == "attachment" )
    {
      QString path = url.path();
      int width = QUrlQuery( url.query() ).queryItemValue( "w" ).toInt();
      int height = QUrlQuery( url.query() ).queryItemValue( "h" ).toInt();
      QString attachmentId = QStringLiteral( "%1://%2" ).arg( url.scheme() ).arg( url.path() );
      QString attachmentFile = QgsProject::instance()->resolveAttachmentIdentifier( attachmentId );
      if ( !attachmentFile.isEmpty() )
      {
        return QImage( attachmentFile ).scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
      }
    }
  }
  return QTextEdit::loadResource( type, url );
}

void KadasMapItemTooltip::enterEvent( QEvent * )
{
  mHideTimer.stop();
}

void KadasMapItemTooltip::clear()
{
  mItem = nullptr;
  setText( "" );
  mShowTimer.stop();
  hide();
}

void KadasMapItemTooltip::positionAndShow()
{
  double x = mPos.x() + 5;
  double y = mPos.y() + 5;
  if ( x + sWidth > mCanvas->width() )
  {
    x = mCanvas->width() - sWidth;
  }
  if ( y + sHeight > mCanvas->height() )
  {
    y = mCanvas->height() - sHeight;
  }
  move( x, y );
  show();
}
