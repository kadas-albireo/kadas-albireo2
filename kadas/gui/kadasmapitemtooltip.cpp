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

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QUrlQuery>

#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/kadasmapcanvasitem.h"
#include "kadas/gui/kadasmapitemtooltip.h"
#include "kadas/gui/mapitems/kadasmapitem.h"

KadasMapItemTooltip::KadasMapItemTooltip( QgsMapCanvas *canvas )
  : QTextEdit( canvas )
  , mCanvas( canvas )
{
  setObjectName( "TooltipWidget" );
  mShowTimer.setSingleShot( true );
  mHideTimer.setSingleShot( true );
  setReadOnly( true );
  connect( &mShowTimer, &QTimer::timeout, this, &KadasMapItemTooltip::positionAndShow );
  connect( &mHideTimer, &QTimer::timeout, this, &KadasMapItemTooltip::clear );
  setFixedSize( sWidth, sHeight );
  hide();
}

void KadasMapItemTooltip::updateForPos( const QPoint &canvasPos )
{
  const KadasMapItem *item = nullptr;

  QGraphicsItem *canvasItem = mCanvas->itemAt( canvasPos );
  if ( dynamic_cast<KadasMapCanvasItem *>( canvasItem ) && static_cast<KadasMapCanvasItem *>( canvasItem )->isVisible() )
  {
    KadasMapCanvasItem *mapCanvasItem = static_cast<KadasMapCanvasItem *>( canvasItem );
    item = mapCanvasItem->mapItem();
  }
  else
  {
    KadasFeaturePicker::PickResult result = KadasFeaturePicker::pick( mCanvas, mCanvas->getCoordinateTransform()->toMapCoordinates( canvasPos ), Qgis::GeometryType::Unknown, KadasItemLayer::PickObjective::PICK_OBJECTIVE_TOOLTIP );
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
  mShowTimer.stop();
}

void KadasMapItemTooltip::mousePressEvent( QMouseEvent *ev )
{
  mMouseMoved = false;
  QTextEdit::mousePressEvent( ev );
}

void KadasMapItemTooltip::mouseMoveEvent( QMouseEvent *ev )
{
  mMouseMoved = true;
  QString anchor = document()->documentLayout()->anchorAt( ev->pos() );
  QString image = document()->documentLayout()->imageAt( ev->pos() );
  if ( ev->button() == Qt::NoButton && ( !anchor.isEmpty() || !image.isEmpty() ) )
  {
    viewport()->setCursor( Qt::PointingHandCursor );
  }
  else
  {
    viewport()->setCursor( Qt::IBeamCursor );
  }
  QTextEdit::mouseMoveEvent( ev );
}

void KadasMapItemTooltip::mouseReleaseEvent( QMouseEvent *ev )
{
  if ( ev->button() == Qt::LeftButton && !mMouseMoved )
  {
    QString anchor = document()->documentLayout()->anchorAt( ev->pos() );
    QString image = document()->documentLayout()->imageAt( ev->pos() );
    if ( !anchor.isEmpty() )
    {
      QDesktopServices::openUrl( QUrl( anchor ) );
    }
    else if ( !image.isEmpty() )
    {
      QUrl url( image );
      if ( url.scheme() == "attachment" )
      {
        QString path = url.path();
        QString attachmentId = QStringLiteral( "%1://%2" ).arg( url.scheme() ).arg( url.path() );
        image = QgsProject::instance()->resolveAttachmentIdentifier( attachmentId );
      }
      if ( QFile::exists( image ) )
      {
        QString path = QString( image ).replace( QStringLiteral( "\\" ), QStringLiteral( "/" ) );
        if ( !path.startsWith( QStringLiteral( "/" ) ) )
        {
          path.prepend( QStringLiteral( "/" ) );
        }
        QDesktopServices::openUrl( QUrl( QStringLiteral( "file://%1" ).arg( path ) ) );
      }
    }
  }
  else
  {
    QTextEdit::mouseReleaseEvent( ev );
  }
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
