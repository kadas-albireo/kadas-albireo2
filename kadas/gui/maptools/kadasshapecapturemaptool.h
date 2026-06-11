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
#include "kadas/gui/kadasattributetypes.h"

class QgsMapCanvas;
class QgsMapMouseEvent;
class QgsRubberBand;
class KadasFloatingInputWidget;


/**
 * Lightweight shape-capture map tool.
 *
 * Captures one of: rectangle, circle (center + radius), circular sector
 * (center + radius + sweep), polyline, polygon. Rendering relies on
 * QgsRubberBand. On completion, emits shapeCaptured() with the resulting
 * geometry in the canvas destination CRS.
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
      Sector,
      Polyline,
      Polygon,
    };

    KadasShapeCaptureMapTool( QgsMapCanvas *canvas, Shape shape );
    ~KadasShapeCaptureMapTool() override;

    void setShape( Shape shape );
    Shape shape() const { return mShape; }

    //! Removes any rubber band and resets capture state.
    void clear();

    //! Replaces the displayed circle / sector preview (Circle and Sector modes). New radius is in canvas map units.
    void setCircleRadius( double radius );

    //! Center of the last captured circle / sector, or last anchor point.
    QgsPointXY circleCenter() const { return mAnchor; }
    double circleRadius() const { return mCircleRadius; }

    //! Start angle of the last captured sector, in radians CCW from east (Sector mode).
    double sectorStartAngle() const { return mSectorStartAngle; }
    //! Stop angle of the last captured sector, in radians; stopAngle - startAngle >= 2*pi means full circle.
    double sectorStopAngle() const { return mSectorStopAngle; }

    //! True while the user is actively capturing (mid-drag for rect/circle, mid-vertex-stream for poly).
    bool isCapturing() const { return mDragging || mCapturing || mSectorStage != SectorStage::None; }

    /**
     * Replaces the displayed polyline/polygon preview with the given vertices in canvas CRS.
     * Used by callers that seed the tool from an externally picked feature. Does not emit shapeCaptured().
     */
    void setCapturedPolyline( const QVector<QgsPointXY> &vertices );

    //! Builds a closed polygon ring approximating a circle. Coordinates are in the same CRS as \a center.
    static QgsGeometry circlePolygon( const QgsPointXY &center, double radius, int segments = 64 );

    /**
     * Builds a pie-slice polygon (center, arc from \a startAngle to \a stopAngle in radians CCW from east).
     * A sweep of 2*pi or more yields a full circle. Coordinates are in the same CRS as \a center.
     */
    static QgsGeometry sectorPolygon( const QgsPointXY &center, double radius, double startAngle, double stopAngle, int segments = 64 );

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void canvasDoubleClickEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;
    void activate() override;
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

    // Sector click state
    enum class SectorStage
    {
      None,
      HaveCenter,
      HaveRadius,
    };
    SectorStage mSectorStage = SectorStage::None;
    double mSectorStartAngle = 0.0;
    double mSectorStopAngle = 0.0;

    // Polyline / polygon vertex state
    QVector<QgsPointXY> mVertices;
    bool mCapturing = false;

    // Numeric attribute input (floating x/y/r/α1/α2 box, enabled via /kadas/showNumericInput)
    enum NumericAttr
    {
      AttrX,
      AttrY,
      AttrR,
      AttrA1,
      AttrA2,
    };
    KadasFloatingInputWidget *mInputWidget = nullptr;
    bool mIgnoreNextMoveEvent = false;

    void setupNumericInput();
    void clearNumericInput();
    void updateNumericInput( QgsMapMouseEvent *e );
    KadasAttribValues collectAttributeValues() const;
    KadasAttribValues attribsFromState( const QgsPointXY &cursorPos ) const;
    void numericInputChanged();
    void acceptNumericInput();

    void resetRubberBand();
    void updateRectRubberBand();
    void updateCircleRubberBand();
    void updateSectorRubberBand();
    void updatePolyRubberBand( const QgsPointXY &cursor, bool hasCursor );

    QgsGeometry buildRectGeometry() const;
    QgsGeometry buildCircleGeometry() const;
    QgsGeometry buildSectorGeometry() const;
    QgsGeometry buildPolylineGeometry() const;
    QgsGeometry buildPolygonGeometry() const;
};

#endif // KADASSHAPECAPTUREMAPTOOL_H
