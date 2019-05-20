/***************************************************************************
    kadasmaptoolcreateitem.h
    ------------------------
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

#ifndef KADASMAPTOOLCREATEITEM_H
#define KADASMAPTOOLCREATEITEM_H

#include <qgis/qgsmaptool.h>

#include <kadas/gui/kadas_gui.h>
#include <kadas/core/mapitems/kadasmapitem.h>

class KadasFloatingInputWidget;
class KadasFloatingInputWidgetField;
class KadasItemLayer;

class KADAS_GUI_EXPORT KadasMapToolCreateItem : public QgsMapTool
{
  Q_OBJECT
public:
  typedef std::function<KadasMapItem*()> ItemFactory;


  KadasMapToolCreateItem(QgsMapCanvas* canvas, ItemFactory itemFactory, KadasItemLayer* layer);
  ~KadasMapToolCreateItem();

  void activate() override;
  void deactivate() override;

  void canvasPressEvent( QgsMapMouseEvent* e ) override;
  void canvasMoveEvent( QgsMapMouseEvent* e ) override;
  void canvasReleaseEvent( QgsMapMouseEvent* e ) override;
  void keyPressEvent( QKeyEvent *e ) override;

  void setShowInputWidget( bool showInput ) { mShowInput = showInput; }

  KadasMapItem* currentItem() const{ return mItem; }

private:
  enum Status {StatusEmpty, StatusDrawing, StatusFinished} mStatus = StatusEmpty;
  ItemFactory mItemFactory = nullptr;
  KadasMapItem* mItem = nullptr;
  KadasItemLayer* mLayer = nullptr;

  bool mShowInput = false;
  KadasFloatingInputWidget* mInputWidget = nullptr;
  bool mIgnoreNextMoveEvent = false;

  void createItem();
  void addPoint(const QgsPointXY& mapPos);
  void commitItem();
  void cleanup();
  void reset();
  QList<double> collectAttributeValues() const;

private slots:
  void inputChanged();
  void acceptInput();

};

#endif // KADASMAPTOOLCREATEITEM_H
