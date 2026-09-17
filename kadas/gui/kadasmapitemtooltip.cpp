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
#include <QApplication>
#include <QContextMenuEvent>
#include <algorithm>
#include <QDesktopServices>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/kadasattachmentutils.h"
#include "kadas/gui/kadasfeaturepicker.h"
#include "kadas/gui/kadasmapitemtooltip.h"

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
  // Images in an annotation's rich text live in the project archive, which a
  // QTextDocument cannot resolve unaided.
  KadasAttachmentUtils::installResourceProvider( document() );
  setFixedSize( sWidth, sHeight );
  canvas->installEventFilter( this );
  hide();
}

void KadasMapItemTooltip::updateForPos( const QPoint &canvasPos )
{
  const KadasFeaturePicker::PickResult result
    = KadasFeaturePicker::pick( mCanvas, mCanvas->getCoordinateTransform()->toMapCoordinates( canvasPos ), Qgis::GeometryType::Unknown, KadasFeaturePicker::PickObjective::PICK_OBJECTIVE_TOOLTIP );
  QgsAnnotationLayer *layer = result.annotationLayer;
  const QString itemId = result.annotationItemId;
  mPos = canvasPos;

  if ( layer == mLayer && itemId == mItemId )
  {
    // Still on whatever we were on. The pointer may have been away in the
    // meantime — over the tooltip window itself, which cancels both timers —
    // so cancel any pending dismissal and re-arm the reveal if this item is
    // not the one already on display.
    if ( layer && !itemId.isEmpty() )
    {
      mHideTimer.stop();
      if ( !mShowTimer.isActive() && itemId != mShownItemId )
        mShowTimer.start( sShowDelayMs );
    }
    return;
  }

  // The hovered item changed — onto nothing, or from one item straight onto the
  // next. Everything from here happens on a timer and at the same pace, so a
  // pointer crossing several items neither flickers through their tooltips nor
  // pays to compose them. The countdown is never restarted mid-move either: it
  // would then only run out once the pointer came to rest.
  mLayer = layer;
  mItemId = itemId;
  mShowTimer.stop();
  mHideTimer.stop();
  if ( layer && !itemId.isEmpty() )
    mShowTimer.start( sShowDelayMs );
  else
    // Leave a shown tooltip up briefly: its links and image are clickable, so
    // the pointer needs a chance to travel onto it.
    mHideTimer.start( sHideDelayMs );
}

void KadasMapItemTooltip::showForItem( QgsAnnotationLayer *layer, const QString &itemId, const QPoint &itemPos )
{
  if ( !layer || itemId.isEmpty() )
    return;
  mShowTimer.stop();
  mHideTimer.stop();
  mLayer = layer;
  mItemId = itemId;
  // Force a recompose: unlike a hover, this is also how an edit in progress is
  // reflected, and the item on display may well be the one that just changed.
  mShownItemId.clear();
  const QString text = tooltipFor( layer, itemId );
  if ( text.isEmpty() )
  {
    hide();
    return;
  }
  mShownItemId = itemId;
  setText( text );
  positionBeside( itemPos );
  show();
}

void KadasMapItemTooltip::positionBeside( const QPoint &itemPos )
{
  // A hover tooltip sits under the cursor, which is fine because the cursor is
  // on the item. Anchored to an item nobody is pointing at, it has to leave the
  // item visible instead: take whichever side has room.
  constexpr int gap = 16;
  int x = itemPos.x() + gap;
  if ( x + sWidth > mCanvas->width() )
    x = itemPos.x() - gap - sWidth;
  if ( x < 0 )
    x = std::max( 0, std::min( mCanvas->width() - sWidth, itemPos.x() - sWidth / 2 ) );

  int y = itemPos.y() - sHeight / 2;
  y = std::max( 0, std::min( mCanvas->height() - sHeight, y ) );
  move( x, y );
}

void KadasMapItemTooltip::setInteractive( bool interactive )
{
  // Deliberately not Qt::WA_TransparentForMouseEvents, which is all or nothing:
  // it would hand the wheel to the canvas too, so pointing at a scrollable
  // tooltip would zoom the map instead of scrolling it. Ignoring the button
  // events individually passes those to the canvas and keeps the wheel here.
  mInteractive = interactive;
}

QString KadasMapItemTooltip::tooltipFor( QgsAnnotationLayer *layer, const QString &itemId )
{
  if ( QgsAnnotationItem *item = layer->item( itemId ) )
  {
    if ( KadasAnnotationItemController *controller = KadasAnnotationControllerRegistry::instance()->controllerFor( item->type() ) )
    {
      const QString live = controller->tooltip( item, layer->crs() );
      if ( !live.isEmpty() )
        return live;
    }
  }
  return KadasAnnotationLayerHelpers::tooltip( layer, itemId );
}

void KadasMapItemTooltip::enterEvent( QEnterEvent * )
{
  mHideTimer.stop();
  mShowTimer.stop();
}

void KadasMapItemTooltip::leaveEvent( QEvent * )
{
  // The pointer can leave for somewhere that sends the canvas no move events at
  // all (another widget, off-window), so the tooltip has to time itself out
  // rather than wait to be told.
  mHideTimer.start( sHideDelayMs );
}

bool KadasMapItemTooltip::eventFilter( QObject *watched, QEvent *event )
{
  // Leaving the canvas for a neighbouring widget — the layer tree, the ribbon —
  // ends the hover, but the canvas sends no further move events to say so. The
  // pointer moving onto the tooltip, or onto a map tool's panel, does not come
  // through here: those are children of the canvas, and Qt sends a widget no
  // leave event when the pointer merely descends into one of its own children.
  if ( watched == mCanvas && event->type() == QEvent::Leave )
    clear();
  return QTextEdit::eventFilter( watched, event );
}

void KadasMapItemTooltip::mousePressEvent( QMouseEvent *ev )
{
  if ( !mInteractive )
  {
    ev->ignore();
    return;
  }
  mPressPos = ev->pos();
  QTextEdit::mousePressEvent( ev );
}

void KadasMapItemTooltip::mouseDoubleClickEvent( QMouseEvent *ev )
{
  if ( !mInteractive )
  {
    ev->ignore();
    return;
  }
  QTextEdit::mouseDoubleClickEvent( ev );
}

void KadasMapItemTooltip::contextMenuEvent( QContextMenuEvent *ev )
{
  // Right-click is how drawing is finished, so it belongs to the map tool.
  if ( !mInteractive )
  {
    ev->ignore();
    return;
  }
  QTextEdit::contextMenuEvent( ev );
}

void KadasMapItemTooltip::mouseMoveEvent( QMouseEvent *ev )
{
  if ( !mInteractive )
  {
    ev->ignore();
    return;
  }
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
  if ( !mInteractive )
  {
    ev->ignore();
    return;
  }
  // A press that wandered was a text selection, not a click on the link under
  // it. Judge that by how far it travelled rather than by whether any movement
  // arrived at all: a trackpad or a touch screen puts a pixel or two into every
  // click, which used to be enough to swallow the link entirely.
  if ( ev->button() == Qt::LeftButton && ( ev->pos() - mPressPos ).manhattanLength() <= QApplication::startDragDistance() )
  {
    QString anchor = document()->documentLayout()->anchorAt( ev->pos() );
    QString image = document()->documentLayout()->imageAt( ev->pos() );
    if ( !anchor.isEmpty() )
    {
      QDesktopServices::openUrl( QUrl( anchor ) );
    }
    else if ( !image.isEmpty() )
    {
      // The image is drawn small to fit the tooltip; opening it shows the file
      // behind it at the resolution it was stored with.
      const QUrl url( image );
      const QString file = KadasAttachmentUtils::isIdentifier( image ) ? KadasAttachmentUtils::resolve( image ) : ( url.isLocalFile() ? url.toLocalFile() : image );
      if ( !file.isEmpty() && QFile::exists( file ) )
      {
        QDesktopServices::openUrl( QUrl::fromLocalFile( file ) );
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
  mLayer = nullptr;
  mItemId.clear();
  mShownItemId.clear();
  setText( "" );
  mShowTimer.stop();
  mHideTimer.stop();
  hide();
}

void KadasMapItemTooltip::positionAndShow()
{
  // Composed here rather than on hover, so only the item the pointer settles on
  // is paid for (a pin samples the project heightmap).
  const QString text = mLayer && !mItemId.isEmpty() ? tooltipFor( mLayer, mItemId ) : QString();
  // Records the item as dealt with even when it has nothing to say, so the
  // re-arm above does not keep retrying it.
  mShownItemId = mItemId;
  if ( text.isEmpty() )
  {
    hide();
    return;
  }
  setText( text );

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
