/***************************************************************************
    kadasredliningintergration.h
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

#ifndef KADASREDLININGINTEGRATION_H
#define KADASREDLININGINTEGRATION_H

#include <functional>

#include <QObject>

class QAction;
class QComboBox;
class QSpinBox;
class QToolButton;

class QgsColorButton;
class QgsMapCanvas;

class KadasGeometryItem;
class KadasItemLayer;
class KadasMapItem;
class KadasMainWindow;

class KadasRedliningIntegration : public QObject
{
  Q_OBJECT
public:
  struct UI
  {
    QToolButton* buttonNewObject;
    QSpinBox* spinBoxSize;
    QgsColorButton* colorButtonOutlineColor;
    QgsColorButton* colorButtonFillColor;
    QComboBox* comboOutlineStyle;
    QComboBox* comboFillStyle;
  };

  KadasRedliningIntegration( const UI &ui, KadasMainWindow* main );
  KadasItemLayer *getOrCreateLayer();
  KadasItemLayer *getLayer() const;

signals:
  void styleChanged();

private:
  UI mUi;
  KadasMainWindow* mMainWindow = nullptr;
  QgsMapCanvas* mCanvas = nullptr;
  QString mLayerId;

  QAction* mActionNewPoint = nullptr;
  QAction* mActionNewSquare = nullptr;
  QAction* mActionNewTriangle = nullptr;
  QAction* mActionNewLine = nullptr;
  QAction* mActionNewRectangle = nullptr;
  QAction* mActionNewPolygon = nullptr;
  QAction* mActionNewCircle = nullptr;
  QAction* mActionNewText = nullptr;

  void toggleCreateItem(bool active, const std::function<KadasMapItem*()>& itemFactory);
  KadasGeometryItem *applyStyle(KadasGeometryItem* item);

  static QIcon createOutlineStyleIcon( Qt::PenStyle style );
  static QIcon createFillStyleIcon( Qt::BrushStyle style );

private slots:
  void saveColor();
  void saveOutlineWidth();
  void saveStyle();
  void updateItemStyle();

};

#endif // KADASREDLININGINTEGRATION_H
