/***************************************************************************
    kadasshapecapturemaptool.h
    --------------------------
    copyright            : (C) 2026 by Denis Rouzaud
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

#ifndef KADASSHAPECAPTUREMAPTOOL_H
#define KADASSHAPECAPTUREMAPTOOL_H

#include <QVector>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsgeometry.h>
#include <qgis/qgsmaptool.h>
#include <qgis/qgspointxy.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapCanvas;
class QgsMapMouseEvent;
class QgsRubberBand;


/**
 * Lightweight shape-capture map tool.
 *
 * Captures one of: rectangle, circle (center + radius), polyline, polygon.
 * Rendering relies on QgsRubberBand. On completion, emits shapeCaptured()
 * with the resulting geometry in the canvas destination CRS.
 *
 * The rubber band stays visible after capture until clear() is called or a
 * new capture starts; this allows callers (e.g. viewshed) to keep showing
 * the captured shape while a configuration dialog is open and to update it
 * via setCircleRadius() / setCapturedGeometry().
 */
class KADAS_GUI_EXPORT KadasShapeCaptureMapTool : public QgsMapTool
{
    Q_OBJECT

  public:
    enum class Shape
    {
      Rectangle,
      Circle,
      Polyline,
      Polygon,
    };

    KadasShapeCaptureMapTool( QgsMapCanvas *canvas, Shape shape );
    ~KadasShapeCaptureMapTool() override;

    void setShape( Shape shape );
    Shape shape() const { return mShape; }

    //! Removes any rubber band and resets capture state.
    void clear();

    //! Replaces the displayed circle preview (Circle mode only). New radius is in canvas map units.
    void setCircleRadius( double radius );

    //! Center of the last captured circle (Circle mode), or last anchor point.
    QgsPointXY circleCenter() const { return mAnchor; }
    double circleRadius() const { return mCircleRadius; }

    //! Builds a closed polygon ring approximating a circle. Coordinates are in the same CRS as \a center.
    static QgsGeometry circlePolygon( const QgsPointXY &center, double radius, int segments = 64 );

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void canvasDoubleClickEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    void deactivate() override;

  signals:
    void shapeCaptured( const QgsGeometry &geometry, const QgsCoordinateReferenceSystem &crs );
    void cleared();

  private:
    Shape mShape;
    QgsRubberBand *mRubberBand = nullptr;

    // Rect / circle drag state
    bool mDragging = false;
    QgsPointXY mAnchor;
    QgsPointXY mCurrent;
    double mCircleRadius = 0.0;

    // Polyline / polygon vertex state
    QVector<QgsPointXY> mVertices;
    bool mCapturing = false;

    void resetRubberBand();
    void updateRectRubberBand();
    void updateCircleRubberBand();
    void updatePolyRubberBand( const QgsPointXY &cursor, bool hasCursor );

    QgsGeometry buildRectGeometry() const;
    QgsGeometry buildCircleGeometry() const;
    QgsGeometry buildPolylineGeometry() const;
    QgsGeometry buildPolygonGeometry() const;
};

#endif // KADASSHAPECAPTUREMAPTOOL_H
