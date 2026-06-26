/***************************************************************************
    kadassidepanelhost.cpp
    ----------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QEvent>
#include <QLayout>
#include <QTimer>
#include <QVBoxLayout>

#include <qgis/qgsmapcanvas.h>

#include "kadas/gui/kadassidepanelhost.h"

QString KadasSidePanelHost::objectNameForEdge( Edge edge )
{
  return edge == Edge::Left ? QStringLiteral( "KadasSidePanelHostLeft" ) : QStringLiteral( "KadasSidePanelHostRight" );
}

KadasSidePanelHost::KadasSidePanelHost( Edge edge, QWidget *parent )
  : QWidget( parent )
  , mEdge( edge )
{
  setObjectName( objectNameForEdge( edge ) );
  // Auto width from the panel size hint, full canvas height.
  setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Expanding );

  mLayout = new QVBoxLayout( this );
  mLayout->setContentsMargins( 4, 4, 4, 4 );
  mLayout->setSpacing( 4 );

  // Hidden until a panel is added; collapses to zero width when empty.
  hide();
}

void KadasSidePanelHost::setMapCanvas( QgsMapCanvas *canvas )
{
  if ( mCanvas )
    mCanvas->removeEventFilter( this );
  mCanvas = canvas;
  if ( mCanvas )
  {
    // Watch the canvas so every reflow-driven resize that follows a panel
    // toggle can be re-anchored, not just the first one.
    mCanvas->installEventFilter( this );

    if ( !mSettleTimer )
    {
      // The toggle triggers a short burst of resizes as the layout settles;
      // each one restarts this timer. A zero interval coalesces the burst yet
      // thaws on the very next event-loop turn after the last resize, so the
      // canvas is never left frozen long enough to show a stale/blank strip
      // when a narrower panel grows the canvas.
      mSettleTimer = new QTimer( this );
      mSettleTimer->setSingleShot( true );
      mSettleTimer->setInterval( 0 );
      connect( mSettleTimer, &QTimer::timeout, this, &KadasSidePanelHost::finishCanvasAnchor );
    }
  }
}

void KadasSidePanelHost::addPanel( QWidget *panel )
{
  if ( !panel )
    return;
  // Capture the pre-reflow state and freeze before touching the layout, then
  // defer the visibility/reflow to the next event-loop turn (see
  // scheduleReflow): switching tools destroys one panel and creates another,
  // so coalescing both into a single reflow avoids the host momentarily
  // emptying and the canvas flashing to full width and back.
  scheduleReflow();
  // Panels expand vertically to fill the host, so simply append them.
  mLayout->addWidget( panel );
}

void KadasSidePanelHost::removePanel( QWidget *panel )
{
  if ( !panel )
    return;
  scheduleReflow();
  mLayout->removeWidget( panel );
}

void KadasSidePanelHost::scheduleReflow()
{
  if ( mReflowPending )
    return; // Anchor already captured before this burst's first layout change.
  mReflowPending = true;
  // Capture the anchor before any layout change so a remove+add in the same
  // turn is measured against the original, settled extent.
  mPendingAnchor = captureCanvasAnchor();
  if ( mPendingAnchor.valid && mCanvas )
    mCanvas->freeze( true );
  QMetaObject::invokeMethod( this, &KadasSidePanelHost::reconcileReflow, Qt::QueuedConnection );
}

void KadasSidePanelHost::reconcileReflow()
{
  mReflowPending = false;
  // The layout now holds the final set of panels for this burst; toggle
  // visibility once and re-anchor against the captured extent.
  updateVisibility();
  armCanvasAnchor( mPendingAnchor );
  mPendingAnchor = CanvasAnchor();
}

void KadasSidePanelHost::updateVisibility()
{
  // Visible as soon as at least one panel is present.
  setVisible( mLayout->count() > 0 );
}

KadasSidePanelHost::CanvasAnchor KadasSidePanelHost::captureCanvasAnchor() const
{
  CanvasAnchor anchor;
  if ( !mCanvas )
    return anchor;

  const QgsRectangle extent = mCanvas->extent();
  const double mapUnitsPerPixel = mCanvas->mapUnitsPerPixel();
  if ( extent.isEmpty() || mapUnitsPerPixel <= 0.0 )
    return anchor;

  anchor.valid = true;
  anchor.mapUnitsPerPixel = mapUnitsPerPixel;
  anchor.top = extent.yMaximum();
  anchor.bottom = extent.yMinimum();
  // A right-edge panel grows/shrinks the canvas from the right, so the left
  // edge stays put; a left-edge panel anchors the right edge instead.
  anchor.anchorLeft = mEdge == Edge::Right;
  anchor.anchorX = anchor.anchorLeft ? extent.xMinimum() : extent.xMaximum();
  return anchor;
}

void KadasSidePanelHost::armCanvasAnchor( const CanvasAnchor &anchor )
{
  if ( !anchor.valid || !mCanvas )
    return;

  // The panel is shown empty and then populated synchronously, so the canvas
  // reflows to its final width over a burst of resizes once control returns to
  // the event loop. Freeze rendering for the whole burst so the user never
  // sees the intermediate, rescaled extent, and re-anchor on each resize until
  // it settles (see eventFilter / finishCanvasAnchor).
  mArmedAnchor = anchor;
  mCanvas->freeze( true );
  mSettleTimer->start();

  // Re-anchor once now in case the reflow already happened synchronously and
  // no further resize event is delivered.
  QMetaObject::invokeMethod( this, &KadasSidePanelHost::applyArmedAnchor, Qt::QueuedConnection );
}

bool KadasSidePanelHost::eventFilter( QObject *watched, QEvent *event )
{
  if ( watched == mCanvas && event->type() == QEvent::Resize && mArmedAnchor.valid && !mApplying )
  {
    // Re-anchor after QGIS has handled the resize and updated the output size.
    QMetaObject::invokeMethod( this, &KadasSidePanelHost::applyArmedAnchor, Qt::QueuedConnection );
  }
  return QWidget::eventFilter( watched, event );
}

void KadasSidePanelHost::applyArmedAnchor()
{
  if ( !mArmedAnchor.valid || !mCanvas )
    return;

  const QSize outputSize = mCanvas->mapSettings().outputSize();
  if ( outputSize.isEmpty() )
    return;

  // Keep the scale and the anchored edge fixed; the vertical extent is
  // unchanged because the panel only affects the canvas width.
  const double widthMu = outputSize.width() * mArmedAnchor.mapUnitsPerPixel;
  double xMin;
  double xMax;
  if ( mArmedAnchor.anchorLeft )
  {
    xMin = mArmedAnchor.anchorX;
    xMax = mArmedAnchor.anchorX + widthMu;
  }
  else
  {
    xMax = mArmedAnchor.anchorX;
    xMin = mArmedAnchor.anchorX - widthMu;
  }

  mApplying = true;
  mCanvas->setExtent( QgsRectangle( xMin, mArmedAnchor.bottom, xMax, mArmedAnchor.top ) );
  mApplying = false;

  // Keep the canvas frozen until the resizes stop; restart the settle timer so
  // a later reflow in the same burst is still corrected.
  mSettleTimer->start();
}

void KadasSidePanelHost::finishCanvasAnchor()
{
  mArmedAnchor.valid = false;
  if ( mCanvas )
  {
    mCanvas->freeze( false );
    mCanvas->refresh();
  }
}
