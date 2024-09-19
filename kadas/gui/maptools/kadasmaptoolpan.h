/***************************************************************************
    kadasmaptoolpan.h
    -----------------
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

#ifndef KADASMAPTOOLPAN_H
#define KADASMAPTOOLPAN_H

#include <qgis/qgsmaptool.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadasfeaturepicker.h"

class QPinchGesture;
class QgsFeature;
class QgsLabelPosition;
class QgsMapCanvas;
class QgsRubberBand;
class QgsVectorLayer;
class KadasMapItemTooltip;


/** \ingroup gui
 * A map tool for panning the map.
 * @see QgsMapTool
 */
class KADAS_GUI_EXPORT KadasMapToolPan : public QgsMapTool
{
    Q_OBJECT
  public:
    //! constructor
    KadasMapToolPan( QgsMapCanvas *canvas, bool allowItemInteraction = true );

    ~KadasMapToolPan();

    void activate() override;
    void deactivate() override;

    //! Overridden mouse press event
    virtual void canvasPressEvent( QgsMapMouseEvent *e ) override;

    //! Overridden mouse move event
    virtual void canvasMoveEvent( QgsMapMouseEvent *e ) override;

    //! Overridden mouse release event
    virtual void canvasReleaseEvent( QgsMapMouseEvent *e ) override;

    bool gestureEvent( QGestureEvent *event ) override;

  signals:
    void contextMenuRequested( QPoint screenPos, QgsPointXY mapPos );
    void itemPicked( const KadasFeaturePicker::PickResult &result );

  protected:

    //! Flag to indicate whether interaction with map items is allowed
    bool mAllowItemInteraction;

    //! Flag to indicate a map canvas drag operation is taking place
    bool mDragging;

    //! Flag to indicate a pinch gesture is taking place
    bool mPinching;

    //! Zoom are rubberband for shift+select mode
    QgsRubberBand *mExtentRubberBand;

    //! Stores zoom rect for shift+select mode
    QRect mExtentRect;

    //!Flag to indicate whether mouseRelease is a click (i.e. no moves inbetween)
    bool mPickClick;

    KadasMapItemTooltip *mTooltipWidget = nullptr;

    void pinchTriggered( QPinchGesture *gesture );

};

#endif // KADASMAPTOOLPAN_H
