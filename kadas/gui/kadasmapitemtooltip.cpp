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

#include <qgis/qgsmapcanvas.h>
#include <kadas/gui/kadasfeaturepicker.h>
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
  connect( &mHideTimer, &QTimer::timeout, this, &KadasMapItemTooltip::clearAndHide );
  setFixedSize( sWidth, sHeight );
  hide();
}

void KadasMapItemTooltip::updateForPos( const QPoint &canvasPos )
{
  KadasFeaturePicker::PickResult result = KadasFeaturePicker::pick( mCanvas, mCanvas->getCoordinateTransform()->toMapCoordinates( canvasPos ) );

  // If hovering over an item, update/show tooltip
  if ( result.itemId != KadasItemLayer::ITEM_ID_NULL )
  {
    KadasMapItem *item = static_cast<KadasItemLayer *>( result.layer )->items()[result.itemId];
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
      mShowTimer.start( 500 );
    }
  }
  else if ( isVisible() )
  {
    mHideTimer.start( 500 );
    mShowTimer.stop();
  }
}

void KadasMapItemTooltip::enterEvent( QEvent * )
{
  mHideTimer.stop();
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

void KadasMapItemTooltip::clearAndHide()
{
  hide();
  mItem = nullptr;
  setText( "" );
}
