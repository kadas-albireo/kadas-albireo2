/***************************************************************************
    kadasmaptoolminmax.h
    ----------------------
    copyright            : (C) 2022 by Sandro Mani
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

#ifndef KADASMAPTOOLMINMAX_H
#define KADASMAPTOOLMINMAX_H

#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/maptools/kadasshapecapturemaptool.h"

class QAction;
class QComboBox;
class QgsRubberBand;

class KadasSidePanel;


class KADAS_GUI_EXPORT KadasMapToolMinMax : public KadasShapeCaptureMapTool
{
    Q_OBJECT
  public:
    KadasMapToolMinMax( QgsMapCanvas *mapCanvas, QAction *actionViewshed, QAction *actionProfile );
    ~KadasMapToolMinMax() override;

    enum class FilterType
    {
      FilterRect,
      FilterPoly,
      FilterCircle
    };
    Q_ENUM( FilterType )

    void setFilterType( FilterType filterType );
    //! Runs the min/max computation against an externally-supplied geometry (e.g. picked from a feature).
    void runMinMax( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &crs );

    void activate() override;
    void deactivate() override;
    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;

  private slots:
    void requestPick();
    void onShapeCaptured( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &crs );

  private:
    FilterType mFilterType = FilterType::FilterRect;
    QComboBox *mFilterTypeCombo = nullptr;
    KadasSidePanel *mBottomBar = nullptr;
    QgsRubberBand *mPinMinBand = nullptr;
    QgsRubberBand *mPinMaxBand = nullptr;
    QgsPointXY mPinMinPos;
    QgsPointXY mPinMaxPos;
    bool mPinsValid = false;
    bool mPickFeature = false;
    QAction *mActionViewshed = nullptr;
    QAction *mActionProfile = nullptr;

    static KadasShapeCaptureMapTool::Shape shapeFor( FilterType t );
    void showContextMenu( const QgsPointXY &mapPos, bool isMaxPin ) const;
};


#endif // KADASMAPTOOLMINMAX_H
