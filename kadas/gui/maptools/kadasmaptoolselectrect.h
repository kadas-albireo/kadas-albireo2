/***************************************************************************
    kadasmaptoolselectrect.h
    ------------------------
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

#ifndef KADASMAPTOOLSELECTRECT_H
#define KADASMAPTOOLSELECTRECT_H

#include <qgis/qgsmaptool.h>
#include <qgis/qgsrubberband.h>

#include "kadas/gui/kadas_gui.h"

class KADAS_GUI_EXPORT KadasMapToolSelectRect : public QgsMapTool
{
    Q_OBJECT
  public:
    KadasMapToolSelectRect( QgsMapCanvas *mapCanvas );

    void setRect( const QgsRectangle &rect );
    const QgsRectangle &rect() const { return mRect; }

    void clear();

    void setAllowResize( bool allowResize ) { mResizeAllowed = allowResize; }
    void setShowReferenceWhenMoving( bool showReference ) { mShowReferenceWhenMoving = showReference; }

    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;

    void deactivate() override;

  signals:
    void rectChanged( const QgsRectangle &rect );
    void rectChangeComplete( const QgsRectangle &rect );

  private:
    bool mResizeAllowed = true;
    bool mShowReferenceWhenMoving = true;
    QgsRectangle mRect;
    QgsRectangle mOldRect;

    QgsRubberBand *mRubberband = nullptr;
    QgsRubberBand *mOldRubberband = nullptr;
    enum {InteractionNone, InteractionMoving, InteractionResizing} mInteraction = InteractionNone;

    QList<QgsPointXY> mResizePoints;
    QList<std::function<void( const QgsPointXY )>> mResizeHandlers;
    QgsPointXY mResizeMoveOffset;

    QRect canvasRect( const QgsRectangle &rect ) const;
};

#endif // KADASMAPTOOLSELECTRECT_H
