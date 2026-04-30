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

#include <memory>

#include <QObject>
#include <QPointer>

#include "kadas/gui/kadasmapiteminterface.h"


class QAction;
class QToolButton;

class QgsAnnotationLayer;
class QgsMapCanvas;
class QgsMapLayer;

class KadasItemLayer;
class KadasMapItem;
class KadasMainWindow;


/**
 * Legacy KadasMapItemInterface used by the CoordinateCross action, which
 * has no annotation-item equivalent yet and therefore still drives the
 * legacy KadasMapToolCreateItem.
 */
class KadasCoordCrossItemInterface : public KadasMapItemInterface
{
  public:
    KadasCoordCrossItemInterface() = default;
    KadasMapItem *createItem() const override;
};


class KadasRedliningIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasRedliningIntegration( QToolButton *buttonNewObject, QObject *parent );

    //! Returns (creating if needed) the legacy redlining KadasItemLayer.
    //! Used by the CoordinateCross action while it remains on the legacy tool.
    KadasItemLayer *getOrCreateLayer();

    //! Returns (creating if needed) the redlining QgsAnnotationLayer used
    //! by all migrated actions.
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

    QPointer<KadasItemLayer> mLastLayer;
    QPointer<QgsAnnotationLayer> mLastAnnotationLayer;

    void toggleAnnotation( bool active, AnnotationVariant variant );
    void toggleLegacyCreateItem( bool active, std::unique_ptr<KadasMapItemInterface> interface );

  private slots:
    void activateNewButtonObject();
    void deactivateNewButtonObject();
    void updateLastLayer( QgsMapLayer *layer );
};

#endif // KADASREDLININGINTEGRATION_H
