/***************************************************************************
    kadasmaptooledititem.cpp
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

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>

#include <kadas/core/kadasitemlayer.h>
#include <kadas/core/mapitems/kadasmapitem.h>
#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>

KadasMapToolEditItem::KadasMapToolEditItem(QgsMapCanvas* canvas, const QString& itemId, KadasItemLayer* layer)
  : QgsMapTool(canvas)
  , mLayer(layer)
{
  mItem = layer->takeItem(itemId);
  layer->triggerRepaint();
  KadasMapCanvasItemManager::addItem(mItem);
}

KadasMapToolEditItem::~KadasMapToolEditItem()
{

}

void KadasMapToolEditItem::activate()
{
  QgsMapTool::activate();
  setCursor(Qt::ArrowCursor);
  mStateHistory = new KadasStateHistory(this);
  mStateHistory->push(mItem->state()->clone());
  connect(mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolEditItem::stateChanged);
}

void KadasMapToolEditItem::deactivate()
{
  QgsMapTool::deactivate();
  if(mItem) {
    mLayer->addItem(mItem);
    mLayer->triggerRepaint();
    KadasMapCanvasItemManager::removeItem(mItem);
  }
  delete mStateHistory;
  mStateHistory = nullptr;
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolEditItem::canvasPressEvent( QgsMapMouseEvent* e )
{
  if(e->button() == Qt::RightButton)
  {
    if(mEditContext.isValid()) {
      // Context menu
    } else {
      canvas()->unsetMapTool(this);
    }
  }
}

void KadasMapToolEditItem::canvasMoveEvent( QgsMapMouseEvent* e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  QgsCoordinateTransform crst(canvas()->mapSettings().destinationCrs(), mItem->crs(), QgsProject::instance());
  QgsPointXY pos = crst.transform(e->mapPoint());

  if(e->buttons() == Qt::LeftButton) {
    if(mEditContext.isValid()) {
      mItem->edit(mEditContext, pos, canvas()->mapSettings());
    }
  } else {
    KadasMapItem::EditContext oldContext = mEditContext;
    mEditContext = mItem->getEditContext(pos, canvas()->mapSettings());
    if(!mEditContext.isValid()) {
      setCursor(Qt::ArrowCursor);
      clearNumericInput();
    } else if(mEditContext != oldContext) {
      setCursor(mEditContext.cursor);
      setupNumericInput();
    }
  }
  if(mInputWidget && mEditContext.isValid()) {
    KadasMapItem::AttribValues values = mItem->drawAttribsFromPosition(pos);
    for(int i = 0, n = values.size(); i < n; ++i) {
      const KadasMapItem::NumericAttribute& attribute = mItem->drawAttribs()[i];
      mInputWidget->inputFields()[i]->setText(QString::number(values[i], 'f', attribute.decimals));
    }
    mInputWidget->move( e->x(), e->y() + 20 );
    mInputWidget->show();
    if ( mInputWidget->focusedInputField() ) {
      mInputWidget->focusedInputField()->setFocus();
      mInputWidget->focusedInputField()->selectAll();
    }
  }
}

void KadasMapToolEditItem::canvasReleaseEvent( QgsMapMouseEvent* e )
{
  if(e->button() == Qt::LeftButton && mEditContext.isValid()) {
    mStateHistory->push(mItem->state()->clone());
  }
}

void KadasMapToolEditItem::keyPressEvent( QKeyEvent *e )
{
  if(e->key() == Qt::Key_Escape) {
    canvas()->unsetMapTool(this);
  } else if(e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier) {
    mStateHistory->undo();
  } else if(e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier) {
    mStateHistory->redo();
  }
}

void KadasMapToolEditItem::setupNumericInput()
{
  clearNumericInput();
  if ( QSettings().value( "/kadas/showNumericInput", false ).toBool() && mEditContext.isValid() && !mEditContext.attributes.isEmpty() )
  {
    mInputWidget = new KadasFloatingInputWidget( canvas() );

    for(int i = 0, n = mEditContext.attributes.size(); i < n; ++i){
      const KadasMapItem::NumericAttribute& attribute = mEditContext.attributes[i];
      KadasFloatingInputWidgetField* attrEdit = new KadasFloatingInputWidgetField();
      connect( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolEditItem::inputChanged);
      connect( attrEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolEditItem::acceptInput);
      mInputWidget->addInputField( attribute.name + ":", attrEdit );
      if(i == 0) {
        mInputWidget->setFocusedInputField( attrEdit );
      }
    }
  }
}

void KadasMapToolEditItem::clearNumericInput()
{
  delete mInputWidget;
  mInputWidget = nullptr;
}

void KadasMapToolEditItem::stateChanged(KadasStateHistory::State *state)
{
  mItem->setState(static_cast<const KadasMapItem::State*>(state));
}

void KadasMapToolEditItem::inputChanged()
{

}

void KadasMapToolEditItem::acceptInput()
{

}
