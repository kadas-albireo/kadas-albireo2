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

#include <functional>

#include <QObject>

#include "kadas/gui/kadasmapiteminterface.h"



class QAction;
class QToolButton;

class QgsMapCanvas;
class QgsMapLayer;

class KadasItemLayer;
class KadasMapItem;
class KadasMainWindow;


class KadasPointItemInterface : public KadasMapItemInterface
{
  public:
    KadasPointItemInterface() = default;
    KadasMapItem* createItem() const override;
};

class KadasSquareItemInterface : public KadasMapItemInterface
{
public:
  KadasSquareItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasTriangleItemInterface : public KadasMapItemInterface
{
public:
  KadasTriangleItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasLineItemInterface : public KadasMapItemInterface
{
public:
  KadasLineItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasRectangleItemInterface : public KadasMapItemInterface
{
public:
  KadasRectangleItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasPolygonItemInterface : public KadasMapItemInterface
{
public:
  KadasPolygonItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasCircleItemInterface : public KadasMapItemInterface
{
public:
  KadasCircleItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasTextItemInterface : public KadasMapItemInterface
{
public:
  KadasTextItemInterface() = default;
  KadasMapItem* createItem() const override;
};
class KadasCoordCrossItemInterface : public KadasMapItemInterface
{
public:
  KadasCoordCrossItemInterface() = default;
  KadasMapItem* createItem() const override;
};


class KadasRedliningIntegration : public QObject
{
    Q_OBJECT
  public:
    KadasRedliningIntegration( QToolButton *buttonNewObject, QObject *parent );
    KadasItemLayer *getOrCreateLayer();

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

    void toggleCreateItem(bool active, KadasMapItemInterface *interface );

  private slots:
    void activateNewButtonObject();
    void deactivateNewButtonObject();
    void updateLastLayer( QgsMapLayer *layer );
};

#endif // KADASREDLININGINTEGRATION_H
