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

#include <QPushButton>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssnappingutils.h>


#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasbottombar.h>
#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadasgeometryitem.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>


KadasMapToolCreateItem::KadasMapToolCreateItem ( QgsMapCanvas* canvas, ItemFactory itemFactory, KadasItemLayer* layer )
  : QgsMapTool ( canvas )
  , mItemFactory ( itemFactory )
  , mLayer ( layer )
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
  mStateHistory = new KadasStateHistory ( this );
  connect ( mStateHistory, &KadasStateHistory::stateChanged, this, &KadasMapToolCreateItem::stateChanged );
  createItem();
  if ( QSettings().value ( "/kadas/showNumericInput", false ).toBool() ) {
    mInputWidget = new KadasFloatingInputWidget ( canvas() );

    KadasMapItem::AttribDefs attributes = mItem->drawAttribs();
    for ( auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it ) {
      const KadasMapItem::NumericAttribute& attribute = it.value();
      KadasFloatingInputWidgetField* attrEdit = new KadasFloatingInputWidgetField ( it.key(), attribute.decimals, attribute.min, attribute.max );
      connect ( attrEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolCreateItem::inputChanged );
      connect ( attrEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolCreateItem::acceptInput );
      mInputWidget->addInputField ( attribute.name + ":", attrEdit );
    }
    if ( !attributes.isEmpty() ) {
      mInputWidget->setFocusedInputField ( mInputWidget->inputField ( attributes.begin().key() ) );
    }
  }
  mBottomBar = new KadasBottomBar ( canvas() );
  mBottomBar->setLayout ( new QHBoxLayout() );
  if ( mItem->getEditorFactory() ) {
    mEditor = mItem->getEditorFactory() ( mItem );
    mEditor->syncWidgetToItem();
    mBottomBar->layout()->addWidget ( mEditor );
  }

  QPushButton* undoButton = new QPushButton();
  undoButton->setIcon ( QIcon ( ":/kadas/icons/undo" ) );
  undoButton->setToolTip ( tr ( "Undo" ) );
  undoButton->setEnabled ( false );
  connect ( undoButton, &QPushButton::clicked, this, [this] { mStateHistory->undo(); } );
  connect ( mStateHistory, &KadasStateHistory::canUndoChanged, undoButton, &QPushButton::setEnabled );
  mBottomBar->layout()->addWidget ( undoButton );

  QPushButton* redoButton = new QPushButton();
  redoButton->setIcon ( QIcon ( ":/kadas/icons/redo" ) );
  redoButton->setToolTip ( tr ( "Redo" ) );
  redoButton->setEnabled ( false );
  connect ( redoButton, &QPushButton::clicked, this, [this] { mStateHistory->redo(); } );
  connect ( mStateHistory, &KadasStateHistory::canRedoChanged, redoButton, &QPushButton::setEnabled );
  mBottomBar->layout()->addWidget ( redoButton );

  QPushButton* closeButton = new QPushButton();
  closeButton->setIcon ( QIcon ( ":/kadas/icons/close" ) );
  closeButton->setToolTip ( tr ( "Close" ) );
  connect ( closeButton, &QPushButton::clicked, this, [this] { canvas()->unsetMapTool ( this ); } );
  mBottomBar->layout()->addWidget ( closeButton );

  mBottomBar->show();
}

void KadasMapToolCreateItem::deactivate()
{
  QgsMapTool::deactivate();
  delete mBottomBar;
  mBottomBar = nullptr;
  mEditor = nullptr;
  if ( mItem && mItem->constState()->drawStatus == KadasMapItem::State::Finished ) {
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
  if ( mEditor ) {
    mEditor->setItem ( nullptr );
  }
  delete mItem;
  mItem = nullptr;
}

void KadasMapToolCreateItem::clear()
{
  commitItem();
  cleanup();
  createItem();
  if ( mEditor ) {
    mEditor->setItem ( mItem );
    mEditor->reset();
    mEditor->syncWidgetToItem();
  }
  emit cleared();
}

void KadasMapToolCreateItem::canvasPressEvent ( QgsMapMouseEvent* e )
{
  if ( e->button() == Qt::LeftButton ) {
    QgsPointXY pos = transformMousePoint ( e->mapPoint() );
    addPoint ( pos );
  } else if ( e->button() == Qt::RightButton ) {
    if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing ) {
      finishPart();
    } else {
      canvas()->unsetMapTool ( this );
    }
  }

}

void KadasMapToolCreateItem::canvasMoveEvent ( QgsMapMouseEvent* e )
{
  if ( mIgnoreNextMoveEvent ) {
    mIgnoreNextMoveEvent = false;
    return;
  }
  QgsPointXY pos = transformMousePoint ( e->mapPoint() );

  if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing ) {
    mItem->setCurrentPoint ( pos, &canvas()->mapSettings() );
  }
  if ( mInputWidget ) {
    mInputWidget->ensureFocus();
    KadasMapItem::AttribValues values = mItem->drawAttribsFromPosition ( pos );
    for ( auto it = values.begin(), itEnd = values.end(); it != itEnd; ++it ) {
      mInputWidget->inputField ( it.key() )->setValue ( it.value() );
    }
    mInputWidget->move ( e->x(), e->y() + 20 );
    mInputWidget->show();
    if ( mInputWidget->focusedInputField() ) {
      mInputWidget->focusedInputField()->setFocus();
      mInputWidget->focusedInputField()->selectAll();
    }
  }
}

void KadasMapToolCreateItem::canvasReleaseEvent ( QgsMapMouseEvent* e )
{
}

void KadasMapToolCreateItem::keyPressEvent ( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape ) {
    if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing ) {
      mItem->clear();
    } else {
      canvas()->unsetMapTool ( this );
    }
  } else if ( e->key() == Qt::Key_Z && e->modifiers() == Qt::ControlModifier ) {
    mStateHistory->undo();
  } else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::ControlModifier ) {
    mStateHistory->redo();
  }
}

QgsPointXY KadasMapToolCreateItem::transformMousePoint ( QgsPointXY mapPos ) const
{
  if ( mSnapping ) {
    QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToMap ( mapPos );
    if ( m.isValid() ) {
      mapPos = m.point();
    }
  }
  QgsCoordinateTransform crst ( canvas()->mapSettings().destinationCrs(), mItem->crs(), QgsProject::instance() );
  return crst.transform ( mapPos );
}

KadasMapItem* KadasMapToolCreateItem::takeItem()
{
  KadasMapItem* item = mItem;
  QObject* scope = new QObject;
  connect ( mCanvas, &QgsMapCanvas::mapCanvasRefreshed, scope, [item, scope] { KadasMapCanvasItemManager::removeItem ( item ); scope->deleteLater(); } );
  mItem = nullptr;
  clear();
  return item;
}

void KadasMapToolCreateItem::addPoint ( const QgsPointXY& pos )
{
  if ( mItem->constState()->drawStatus == KadasMapItem::State::Empty ) {
    startPart ( pos );
  } else if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing ) {
    // Add point, stop drawing if item does not accept further points
    if ( !mItem->continuePart() ) {
      finishPart();
    } else {
      mStateHistory->push ( mItem->constState()->clone() );
    }
  } else if ( mItem->constState()->drawStatus == KadasMapItem::State::Finished ) {
    if ( !mMultipart ) {
      clear();
    }
    startPart ( pos );
  }
}

void KadasMapToolCreateItem::createItem()
{
  mItem = mItemFactory();
  mItem->setSelected ( true );
  KadasMapCanvasItemManager::addItem ( mItem );
  mStateHistory->clear();
}

void KadasMapToolCreateItem::startPart ( const QgsPointXY& pos )
{
  if ( !mItem->startPart ( pos ) ) {
    finishPart();
  } else {
    mStateHistory->push ( mItem->constState()->clone() );
  }
}

void KadasMapToolCreateItem::startPart ( const KadasMapItem::AttribValues& attributes )
{
  if ( !mItem->startPart ( attributes ) ) {
    finishPart();
  } else {
    mStateHistory->push ( mItem->constState()->clone() );
  }
}

void KadasMapToolCreateItem::finishPart()
{
  mItem->endPart();
  mStateHistory->push ( mItem->constState()->clone() );
  emit partFinished();
}

void KadasMapToolCreateItem::addPartFromGeometry ( const QgsAbstractGeometry* geom, const QgsCoordinateReferenceSystem& crs )
{
  if ( dynamic_cast<KadasGeometryItem*> ( mItem ) ) {
    if ( crs != mItem->crs() ) {
      QgsAbstractGeometry* transformedGeom = geom->clone();
      transformedGeom->transform ( QgsCoordinateTransform ( crs, mItem->crs(), QgsProject::instance() ) );
      static_cast<KadasGeometryItem*> ( mItem )->addPartFromGeometry ( transformedGeom );
    } else {
      static_cast<KadasGeometryItem*> ( mItem )->addPartFromGeometry ( geom );
    }
    mStateHistory->push ( mItem->constState()->clone() );
    emit partFinished();
  }
}

void KadasMapToolCreateItem::commitItem()
{
  mItem->setSelected ( false );
  if ( mLayer ) {
    mLayer->addItem ( mItem );
    mLayer->triggerRepaint();
    KadasMapItem* item = mItem;
    QObject* scope = new QObject;
    connect ( mCanvas, &QgsMapCanvas::mapCanvasRefreshed, scope, [item, scope] { KadasMapCanvasItemManager::removeItem ( item ); scope->deleteLater(); } );
  }
  if ( !mLayer ) {
    KadasMapCanvasItemManager::removeItem ( mItem );
    delete mItem;
  }
  mItem = nullptr;
}

KadasMapItem::AttribValues KadasMapToolCreateItem::collectAttributeValues() const
{
  KadasMapItem::AttribValues attributes;
  for ( const KadasFloatingInputWidgetField* field : mInputWidget->inputFields() ) {
    attributes.insert ( field->id(), field->text().toDouble() );
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

  QgsPointXY newPos = mItem->positionFromDrawAttribs ( values );
  QgsCoordinateTransform crst ( mItem->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
  mInputWidget->adjustCursorAndExtent ( crst.transform ( newPos ) );

  if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing ) {
    mItem->setCurrentAttributes ( values );
  }
}

void KadasMapToolCreateItem::acceptInput()
{
  if ( mItem->constState()->drawStatus == KadasMapItem::State::Empty ) {
    startPart ( collectAttributeValues() );
  } else if ( mItem->constState()->drawStatus == KadasMapItem::State::Drawing ) {
    if ( !mItem->continuePart() ) {
      finishPart();
    } else {
      mStateHistory->push ( mItem->constState()->clone() );
    }
  } else if ( mItem->constState()->drawStatus == KadasMapItem::State::Finished ) {
    if ( !mMultipart ) {
      clear();
    }
    startPart ( collectAttributeValues() );
  }
}

void KadasMapToolCreateItem::stateChanged ( KadasStateHistory::State* state )
{
  mItem->setState ( static_cast<const KadasMapItem::State*> ( state ) );
}
