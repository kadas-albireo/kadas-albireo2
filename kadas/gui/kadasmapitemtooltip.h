/***************************************************************************
    kadasmapitemtooltip.h
    ---------------------
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

#ifndef KADASMAPITEMTOOLTIP_H
#define KADASMAPITEMTOOLTIP_H

#include <QPointer>
#include <QTimer>
#include <QTextBrowser>

#include "kadas/gui/kadas_gui.h"

class QgsAnnotationLayer;
class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasMapItemTooltip : public QTextEdit
{
    Q_OBJECT
  public:
    KadasMapItemTooltip( QgsMapCanvas *canvas );
    void updateForPos( const QPoint &canvasPos );

    /**
     * When \a interactive is FALSE the tooltip ignores the pointer entirely: its
     * links and image stop being clickable, and clicks and drags pass straight
     * through to the canvas beneath it.
     *
     * Used while a map tool owns the pointer, where the tooltip is a preview
     * rather than something to be operated. Interactive by default.
     */
    void setInteractive( bool interactive );

  public slots:
    void clear();

  protected:
    void enterEvent( QEnterEvent * ) override;
    void leaveEvent( QEvent * ) override;
    void mousePressEvent( QMouseEvent *ev ) override;
    void mouseMoveEvent( QMouseEvent *ev ) override;
    void mouseReleaseEvent( QMouseEvent *ev ) override;

  private:
    static constexpr int sWidth = 320;
    static constexpr int sHeight = 240;
    //! Hover dwell before the tooltip appears, and grace period before it goes.
    static constexpr int sShowDelayMs = 200;
    static constexpr int sHideDelayMs = 500;

    //! Live text from the item's controller, else the text stored on the layer.
    static QString tooltipFor( QgsAnnotationLayer *layer, const QString &itemId );

    QTimer mShowTimer;
    QTimer mHideTimer;
    QPoint mPos;
    QgsMapCanvas *mCanvas = nullptr;
    bool mMouseMoved = false;
    // Item under the pointer, and the one positionAndShow() last acted on. Every
    // transition runs off the two timers, so a pointer sweeping across items
    // composes nothing until it settles.
    QPointer<QgsAnnotationLayer> mLayer;
    QString mItemId;
    QString mShownItemId;

  private slots:
    void positionAndShow();
};

#endif // KADASMAPITEMTOOLTIP_H
