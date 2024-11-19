/***************************************************************************
    kadasmapswipetool.h
    -----------------
    copyright            : (C) 2024 Denis Rouzaud
    email                : denis@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KADASMAPSWIPETOOL_H
#define KADASMAPSWIPETOOL_H

#include <qgsmaptool.h>

class QgsMapCanvas;
class KadasMapSwipeCanvasItem;
class QgsMessageBarItem;


class KadasMapSwipeMapTool : public QgsMapTool
{
    Q_OBJECT

  public:
    static void addContextMenuAction( QgsMapLayer *layer, QgsMapCanvas *canvas, QMenu *menu, QObject *parent = nullptr );

    KadasMapSwipeMapTool( QgsMapCanvas *mapCanvas );

    ~KadasMapSwipeMapTool();

    void addLayers( const QList<QgsMapLayer *> &layers );
    void removeLayers( const QList<QgsMapLayer *> &layers );

    bool isActive() const;


    virtual void activate() override;
    virtual void deactivate() override;
    virtual void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    virtual void canvasPressEvent( QgsMapMouseEvent *e ) override;
    virtual void canvasReleaseEvent( QgsMapMouseEvent *e ) override;

  private:
    void updateMessageBar();

    bool mIsActive;
    QSet<QgsMapLayer *> mLayers;
    KadasMapSwipeCanvasItem *mMapCanvasItem = nullptr;
    QgsMessageBarItem *mMessageBarItem = nullptr;
    QPoint mFirstPoint;
    QCursor mCursorV = QCursor( Qt::SplitVCursor );
    QCursor mCursorH = QCursor( Qt::SplitHCursor );
    bool mIsSwiping = false;
    bool mDirectionDefined = false;
};

#endif // KADASMAPSWIPETOOL_H
