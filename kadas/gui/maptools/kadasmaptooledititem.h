/***************************************************************************
    kadasmaptooledititem.h
    ----------------------
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

#ifndef KADASMAPTOOLEDITITEM_H
#define KADASMAPTOOLEDITITEM_H

#include <qgis/qgsmaptool.h>

#include <kadas/core/kadasstatehistory.h>
#include <kadas/core/mapitems/kadasmapitem.h>
#include <kadas/gui/kadas_gui.h>

class KadasFloatingInputWidget;
class KadasItemLayer;


class KADAS_GUI_EXPORT KadasMapToolEditItem : public QgsMapTool
{
public:
  KadasMapToolEditItem(QgsMapCanvas* canvas, const QString& itemId, KadasItemLayer* layer);
  ~KadasMapToolEditItem();

  void activate() override;
  void deactivate() override;

  void canvasPressEvent( QgsMapMouseEvent* e ) override;
  void canvasMoveEvent( QgsMapMouseEvent* e ) override;
  void canvasReleaseEvent( QgsMapMouseEvent* e ) override;
  void keyPressEvent( QKeyEvent *e ) override;

private:
  KadasStateHistory* mStateHistory = nullptr;
  KadasItemLayer* mLayer = nullptr;
  KadasMapItem* mItem = nullptr;
  KadasMapItem::EditContext mEditContext;

  KadasFloatingInputWidget* mInputWidget = nullptr;
  bool mIgnoreNextMoveEvent = false;

  void setupNumericInput();
  void clearNumericInput();

public slots:
  void inputChanged();
  void acceptInput();
  void stateChanged(KadasStateHistory::State *state);
};

#endif // KADASMAPTOOLEDITITEM_H
