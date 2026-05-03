/***************************************************************************
    kadasredliningintegration.h
    ---------------------------
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

#ifndef KADASREDLININGINTEGRATION_H
#define KADASREDLININGINTEGRATION_H

#include <QObject>
#include <QPointer>


class QAction;
class QToolButton;

class QgsAnnotationLayer;
class QgsMapCanvas;
class QgsMapLayer;

class KadasMainWindow;


class KadasRedliningIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasRedliningIntegration( QToolButton *buttonNewObject, QObject *parent );

    //! Returns (creating if needed) the redlining QgsAnnotationLayer used
    //! by the redlining toolbar actions.
    QgsAnnotationLayer *getOrCreateAnnotationLayer();

    QAction *actionNewPoint() const { return mActionNewPoint; }
    QAction *actionNewSquare() const { return mActionNewSquare; }
    QAction *actionNewTriangle() const { return mActionNewTriangle; }
    QAction *actionNewLine() const { return mActionNewLine; }
    QAction *actionNewRectangle() const { return mActionNewRectangle; }
    QAction *actionNewPolygon() const { return mActionNewPolygon; }
    QAction *actionNewCircle() const { return mActionNewCircle; }
    QAction *actionNewText() const { return mActionNewText; }
    QAction *actionNewCoordinateCross() const { return mActionNewCoordCross; }

  private:
    //! The set of annotation-item kinds the redlining toolbar can create.
    enum class AnnotationVariant
    {
      MarkerCircle,
      MarkerSquare,
      MarkerTriangle,
      Line,
      Rectangle,
      Polygon,
      Circle,
      Text,
      CoordCross,
    };

    QToolButton *mButtonNewObject = nullptr;

    QAction *mActionNewPoint = nullptr;
    QAction *mActionNewSquare = nullptr;
    QAction *mActionNewTriangle = nullptr;
    QAction *mActionNewLine = nullptr;
    QAction *mActionNewRectangle = nullptr;
    QAction *mActionNewPolygon = nullptr;
    QAction *mActionNewCircle = nullptr;
    QAction *mActionNewText = nullptr;
    QAction *mActionNewCoordCross = nullptr;

    QPointer<QgsAnnotationLayer> mLastAnnotationLayer;

    void toggleAnnotation( bool active, AnnotationVariant variant );

  private slots:
    void activateNewButtonObject();
    void deactivateNewButtonObject();
};

#endif // KADASREDLININGINTEGRATION_H
