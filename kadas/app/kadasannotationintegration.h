/***************************************************************************
    kadasannotationintegration.h
    ----------------------------
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

#ifndef KADASANNOTATIONINTEGRATION_H
#define KADASANNOTATIONINTEGRATION_H

#include <QList>
#include <QObject>
#include <QPointer>


class QAction;
class QActionGroup;
class QToolButton;

class QgsAnnotationLayer;
class QgsMapCanvas;
class QgsMapLayer;

class KadasMainWindow;


class KadasAnnotationIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasAnnotationIntegration( QObject *parent );

    //! Returns (creating if needed) the QgsAnnotationLayer used by the
    //! drawing toolbar actions.
    QgsAnnotationLayer *getOrCreateAnnotationLayer();

    //! Shared exclusive (optional) group holding every checkable drawing
    //! tool action, so at most one drawing tool is active at a time.
    QActionGroup *actionGroup() const { return mActionGroup; }

    //! Marker shape tool actions (circle, square, triangle, diamond, star, cross).
    QList<QAction *> markerActions() const { return mMarkerActions; }

    //! Open-shape tool actions (line, arrow).
    QList<QAction *> lineActions() const { return mLineActions; }

    //! Closed-shape tool actions (polygon, rectangle, circle).
    QList<QAction *> polygonActions() const { return mPolygonActions; }

    QAction *actionNewPoint() const { return mActionNewPoint; }
    QAction *actionNewSquare() const { return mActionNewSquare; }
    QAction *actionNewTriangle() const { return mActionNewTriangle; }
    QAction *actionNewDiamond() const { return mActionNewDiamond; }
    QAction *actionNewStar() const { return mActionNewStar; }
    QAction *actionNewCross() const { return mActionNewCross; }
    QAction *actionNewLine() const { return mActionNewLine; }
    QAction *actionNewArrow() const { return mActionNewArrow; }
    QAction *actionNewRectangle() const { return mActionNewRectangle; }
    QAction *actionNewPolygon() const { return mActionNewPolygon; }
    QAction *actionNewCircle() const { return mActionNewCircle; }
    QAction *actionNewText() const { return mActionNewText; }
    QAction *actionNewTextAlongLine() const { return mActionNewTextAlongLine; }
    QAction *actionNewCoordinateCross() const { return mActionNewCoordCross; }
    QAction *actionNewCustomSvg() const { return mActionNewCustomSvg; }

  private:
    //! The set of annotation-item kinds the drawing toolbar can create.
    enum class AnnotationVariant
    {
      MarkerCircle,
      MarkerSquare,
      MarkerTriangle,
      MarkerDiamond,
      MarkerStar,
      MarkerCross,
      MarkerCustomSvg,
      Line,
      Arrow,
      Rectangle,
      Polygon,
      Circle,
      Text,
      TextAlongLine,
      CoordCross,
    };

    QActionGroup *mActionGroup = nullptr;

    QAction *mActionNewPoint = nullptr;
    QAction *mActionNewSquare = nullptr;
    QAction *mActionNewTriangle = nullptr;
    QAction *mActionNewDiamond = nullptr;
    QAction *mActionNewStar = nullptr;
    QAction *mActionNewCross = nullptr;
    QAction *mActionNewCustomSvg = nullptr;
    QAction *mActionNewLine = nullptr;
    QAction *mActionNewArrow = nullptr;
    QAction *mActionNewRectangle = nullptr;
    QAction *mActionNewPolygon = nullptr;
    QAction *mActionNewCircle = nullptr;
    QAction *mActionNewText = nullptr;
    QAction *mActionNewTextAlongLine = nullptr;
    QAction *mActionNewCoordCross = nullptr;

    QList<QAction *> mMarkerActions;
    QList<QAction *> mLineActions;
    QList<QAction *> mPolygonActions;

    QPointer<QgsAnnotationLayer> mLastAnnotationLayer;

    QAction *createToolAction( const QIcon &icon, const QString &text, const QString &objectName, AnnotationVariant variant );
    void toggleAnnotation( bool active, AnnotationVariant variant );

    //! Refreshes the custom-SVG-marker tile icon to the last-used SVG (or the question-mark placeholder).
    void updateCustomSvgActionIcon();
};

#endif // KADASANNOTATIONINTEGRATION_H
