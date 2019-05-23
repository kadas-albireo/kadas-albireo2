/***************************************************************************
    kadasmaptoolcreateitem.cpp
    --------------------------
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

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>

#include <kadas/core/kadasitemlayer.h>
#include <kadas/core/mapitems/kadasmapitem.h>

#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>


KadasMapToolCreateItem::KadasMapToolCreateItem(QgsMapCanvas *canvas, ItemFactory itemFactory, KadasItemLayer *layer)
  : QgsMapTool(canvas)
  , mItemFactory(itemFactory)
  , mLayer(layer)
{
}

KadasMapToolCreateItem::~KadasMapToolCreateItem()
{
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolCreateItem::activate()
{
  QgsMapTool::activate();
  mStateHistory = new KadasStateHistory(this);
  connect(mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolCreateItem::stateChanged);
  createItem();
  if ( QSettings().value( "/kadas/showNumericInput", false ).toBool() )
  {
    mInputWidget = new KadasFloatingInputWidget( canvas() );

    KadasMapItem::AttribDefs attributes = mItem->drawAttribs();
    for(auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it){
      const KadasMapItem::NumericAttribute& attribute = it.value();
      KadasFloatingInputWidgetField* attrEdit = new KadasFloatingInputWidgetField(it.key(), attribute.decimals, attribute.min, attribute.max);
      connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolCreateItem::inputChanged);
      connect( attrEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolCreateItem::acceptInput);
      mInputWidget->addInputField( attribute.name + ":", attrEdit );
    }
    if(!attributes.isEmpty()) {
      mInputWidget->setFocusedInputField(mInputWidget->inputFields()[0]);
    }
  }
}

void KadasMapToolCreateItem::deactivate()
{
  QgsMapTool::deactivate();
  if(mItem->state()->drawStatus == KadasMapItem::State::Finished) {
    commitItem();
  }
  cleanup();
  delete mStateHistory;
  mStateHistory = nullptr;
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolCreateItem::cleanup()
{
  delete mItem;
  mItem = nullptr;
}

void KadasMapToolCreateItem::reset()
{
  commitItem();
  cleanup();
  createItem();
}

void KadasMapToolCreateItem::canvasPressEvent( QgsMapMouseEvent* e )
{
  if(e->button() == Qt::LeftButton)
  {
    addPoint(e->mapPoint());
  }
  else if(e->button() == Qt::RightButton)
  {
    if(mItem->state()->drawStatus == KadasMapItem::State::Drawing) {
      finishItem();
    } else {
      canvas()->unsetMapTool(this);
    }
  }

}

void KadasMapToolCreateItem::canvasMoveEvent( QgsMapMouseEvent* e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  QgsCoordinateTransform crst(canvas()->mapSettings().destinationCrs(), mItem->crs(), QgsProject::instance());
  QgsPointXY pos = crst.transform(e->mapPoint());

  if(mItem->state()->drawStatus == KadasMapItem::State::Drawing) {
    mItem->setCurrentPoint(pos, &canvas()->mapSettings());
  }
  if(mInputWidget) {
    KadasMapItem::AttribValues values = mItem->drawAttribsFromPosition(pos);
    for(int i = 0, n = values.size(); i < n; ++i) {
      mInputWidget->inputFields()[i]->setValue(values[i]);
    }
    mInputWidget->move( e->x(), e->y() + 20 );
    mInputWidget->show();
    if ( mInputWidget->focusedInputField() ) {
      mInputWidget->focusedInputField()->setFocus();
      mInputWidget->focusedInputField()->selectAll();
    }
  }
}

void KadasMapToolCreateItem::canvasReleaseEvent( QgsMapMouseEvent* e )
{
}

void KadasMapToolCreateItem::keyPressEvent( QKeyEvent *e )
{
  if(e->key() == Qt::Key_Escape) {
    if(mItem->state()->drawStatus == KadasMapItem::State::Drawing) {
      mItem->clear();
    } else {
      canvas()->unsetMapTool(this);
    }
  } else if(e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier) {
    mStateHistory->undo();
  } else if(e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier) {
    mStateHistory->redo();
  }
}

void KadasMapToolCreateItem::addPoint(const QgsPointXY &mapPos)
{
  QgsCoordinateTransform crst(canvas()->mapSettings().destinationCrs(), mItem->crs(), QgsProject::instance());
  QgsPointXY pos = crst.transform(mapPos);

  if(mItem->state()->drawStatus == KadasMapItem::State::Empty)
  {
    startItem(pos);
  }
  else if(mItem->state()->drawStatus == KadasMapItem::State::Drawing)
  {
    // Add point, stop drawing if item does not accept further points
    if(!mItem->continuePart()) {
      finishItem();
    } else {
      mStateHistory->push(mItem->state()->clone());
    }
  } else if(mItem->state()->drawStatus == KadasMapItem::State::Finished) {
    reset();
    startItem(pos);
  }
}

void KadasMapToolCreateItem::createItem()
{
  mItem = mItemFactory();
  KadasMapCanvasItemManager::addItem(mItem);
  mStateHistory->clear();
  emit startedCreatingItem(mItem);
}

void KadasMapToolCreateItem::startItem(const QgsPointXY& pos)
{
  if(!mItem->startPart(pos)) {
    finishItem();
  } else {
    mStateHistory->push(mItem->state()->clone());
  }
}

void KadasMapToolCreateItem::startItem(const KadasMapItem::AttribValues &attributes)
{
  if(!mItem->startPart(attributes)) {
    finishItem();
  } else {
    mStateHistory->push(mItem->state()->clone());
  }
}

void KadasMapToolCreateItem::finishItem()
{
  mItem->endPart();
  mStateHistory->push(mItem->state()->clone());
  emit finishedCreatingItem(mItem);
}

void KadasMapToolCreateItem::commitItem()
{
  mLayer->addItem(mItem);
  mLayer->triggerRepaint();
  KadasMapCanvasItemManager::removeItem(mItem);
  mItem = nullptr;
}

KadasMapItem::AttribValues KadasMapToolCreateItem::collectAttributeValues() const
{
  KadasMapItem::AttribValues attributes;
  for(const KadasFloatingInputWidgetField* field : mInputWidget->inputFields()) {
    attributes.insert(field->id(), field->text().toDouble());
  }
  return attributes;
}

void KadasMapToolCreateItem::inputChanged()
{
  KadasMapItem::AttribValues values = collectAttributeValues();

  // Ignore the move event emitted by re-positioning the mouse cursor:
  // The widget mouse coordinates (stored in a integer QPoint) loses precision,
  // and mapping it back to map coordinates in the mouseMove event handler
  // results in a position different from geoPos, and hence the user-input
  // may get altered
  mIgnoreNextMoveEvent = true;

  QgsPointXY newPos = mItem->positionFromDrawAttribs(values);
  mInputWidget->adjustCursorAndExtent( newPos );

  if(mItem->state()->drawStatus == KadasMapItem::State::Drawing) {
    mItem->setCurrentAttributes(values);
  }
}

void KadasMapToolCreateItem::acceptInput()
{
  if(mItem->state()->drawStatus == KadasMapItem::State::Empty) {
    startItem(collectAttributeValues());
  } else if(mItem->state()->drawStatus == KadasMapItem::State::Drawing) {
    if(!mItem->continuePart()) {
      finishItem();
    } else {
      mStateHistory->push(mItem->state()->clone());
    }
  } else if(mItem->state()->drawStatus == KadasMapItem::State::Finished){
    reset();
    startItem(collectAttributeValues());
  }
}

void KadasMapToolCreateItem::stateChanged(KadasStateHistory::State* state)
{
  mItem->setState(static_cast<const KadasMapItem::State*>(state));
}
