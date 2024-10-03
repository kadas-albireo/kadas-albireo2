/***************************************************************************
    kadasredliningintegration.cpp
    -----------------------------
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

#include <QAction>
#include <QMenu>
#include <QShortcut>
#include <QSpinBox>
#include <QToolButton>

#include <qgis/qgsproject.h>

#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/mapitems/kadascircleitem.h"
#include "kadas/gui/mapitems/kadascoordinatecrossitem.h"
#include "kadas/gui/mapitems/kadaslineitem.h"
#include "kadas/gui/mapitems/kadaspointitem.h"
#include "kadas/gui/mapitems/kadaspolygonitem.h"
#include "kadas/gui/mapitems/kadasrectangleitem.h"
#include "kadas/gui/mapitems/kadastextitem.h"
#include "kadas/gui/mapitemeditors/kadasredliningitemeditor.h"
#include "kadas/gui/mapitemeditors/kadasredliningtexteditor.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"

#include "kadasapplication.h"
#include "kadasmainwindow.h"
#include "kadasredliningintegration.h"


KadasMapItem *KadasPointItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasPointItem( crs, KadasPointItem::IconType::ICON_CIRCLE );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
}

KadasMapItem *KadasSquareItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasPointItem( crs, KadasPointItem::IconType::ICON_FULL_BOX );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
};

KadasMapItem *KadasTriangleItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasPointItem( crs, KadasPointItem::IconType::ICON_FULL_TRIANGLE );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
};

KadasMapItem *KadasLineItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasLineItem( crs );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
};

KadasMapItem *KadasRectangleItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasRectangleItem( crs );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
};


KadasMapItem *KadasPolygonItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasPolygonItem( crs );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
};

KadasMapItem *KadasCircleItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasCircleItem( crs );
  item->setEditor( KadasMapItemEditor::REDLINING_ITEM );
  return item;
};

KadasMapItem *KadasTextItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasTextItem *item = new KadasTextItem( crs );
  item->setEditor( KadasMapItemEditor::REDLINING_TEXT );
  return item;
};

KadasMapItem *KadasCoordCrossItemInterface::createItem() const
{
  QgsCoordinateReferenceSystem crs = kApp->mainWindow()->mapCanvas()->mapSettings().destinationCrs();
  KadasMapItem* item = new KadasCoordinateCrossItem( crs );
  return item;
};


KadasRedliningIntegration::KadasRedliningIntegration( QToolButton *buttonNewObject, QObject *parent )
  : QObject( parent ), mButtonNewObject( buttonNewObject )
{
  QAction *actionNewMarker = new QAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Marker" ), this );

  mActionNewPoint = new QAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasPointItemInterface() ); } );

  mActionNewSquare = new QAction( QIcon( ":/kadas/icons/redlining_square" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasSquareItemInterface() ); } );

  mActionNewTriangle = new QAction( QIcon( ":/kadas/icons/redlining_triangle" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasTriangleItemInterface() ); } );

  mActionNewLine = new QAction( QIcon( ":/kadas/icons/redlining_line" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasLineItemInterface() ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_L ), kApp->mainWindow() ), &QShortcut::activated, mActionNewLine, &QAction::trigger );

  mActionNewRectangle = new QAction( QIcon( ":/kadas/icons/redlining_rectangle" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasRectangleItemInterface() ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_R ), kApp->mainWindow() ), &QShortcut::activated, mActionNewRectangle, &QAction::trigger );

  mActionNewPolygon = new QAction( QIcon( ":/kadas/icons/redlining_polygon" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasPolygonItemInterface() ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_P ), kApp->mainWindow() ), &QShortcut::activated, mActionNewPolygon, &QAction::trigger );

  mActionNewCircle = new QAction( QIcon( ":/kadas/icons/redlining_circle" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasCircleItemInterface() ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_C ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCircle, &QAction::trigger );

  mActionNewText = new QAction( QIcon( ":/kadas/icons/redlining_text" ), tr( "Text" ), this );
  mActionNewText->setCheckable( true );
  connect( mActionNewText, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasTextItemInterface() ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_T ), kApp->mainWindow() ), &QShortcut::activated, mActionNewText, &QAction::trigger );

  mActionNewCoordCross = new QAction( QIcon( ":/kadas/icons/coord_cross" ), tr( "Coordinate Cross" ), this );
  mActionNewCoordCross->setCheckable( true );
  connect( mActionNewCoordCross, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, new KadasCoordCrossItemInterface() ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_O ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCoordCross, &QAction::trigger );

  QMenu *menuNewMarker = new QMenu();
  menuNewMarker->addAction( mActionNewPoint );
  menuNewMarker->addAction( mActionNewSquare );
  menuNewMarker->addAction( mActionNewTriangle );
  actionNewMarker->setMenu( menuNewMarker );
  QMenu *menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewMarker );
  menuNewObject->addAction( mActionNewLine );
  menuNewObject->addAction( mActionNewRectangle );
  menuNewObject->addAction( mActionNewPolygon );
  menuNewObject->addAction( mActionNewCircle );
  menuNewObject->addAction( mActionNewText );
  menuNewObject->addAction( mActionNewCoordCross );
  mButtonNewObject->setMenu( menuNewObject );
  mButtonNewObject->setPopupMode( QToolButton::InstantPopup );
  mButtonNewObject->setIcon( QIcon( ":/kadas/icons/shape" ) );
  mButtonNewObject->setCheckable( true );
}

KadasItemLayer *KadasRedliningIntegration::getOrCreateLayer()
{
  if ( !mLastLayer )
  {
    mLastLayer = KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::StandardLayer::RedliningLayer );
  }
  return mLastLayer;
}

void KadasRedliningIntegration::toggleCreateItem(bool active, KadasMapItemInterface *interface )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  QAction *action = qobject_cast<QAction *> ( QObject::sender() );
  if ( active )
  {
    KadasMapToolCreateItem *tool = new KadasMapToolCreateItem( canvas, interface, getOrCreateLayer() );

    KadasLayerSelectionWidget::LayerFilter filter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasItemLayer *>( layer ) && static_cast<KadasItemLayer *>( layer )->layerTypeKey() == QString( "KadasItemLayer" ); };
    KadasLayerSelectionWidget::LayerCreator creator = []( const QString & name )
    {
      return QgsProject::instance()->addMapLayer( new KadasItemLayer( name, QgsCoordinateReferenceSystem( "EPSG:3857" ) ) );
    };
    tool->showLayerSelection( true, kApp->mainWindow()->layerTreeView(), filter, creator );
    tool->setAction( action );
    connect( tool, &QgsMapTool::activated, this, &KadasRedliningIntegration::activateNewButtonObject );
    connect( tool, &QgsMapTool::deactivated, this, &KadasRedliningIntegration::deactivateNewButtonObject );
    connect( tool, &KadasMapToolCreateItem::targetLayerChanged, this, &KadasRedliningIntegration::updateLastLayer );
    kApp->mainWindow()->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    kApp->mainWindow()->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    canvas->setMapTool( tool );
  }
  else if ( !active && canvas->mapTool() && canvas->mapTool()->action() == action )
  {
    canvas->unsetMapTool( canvas->mapTool() );
  }
}

void KadasRedliningIntegration::activateNewButtonObject()
{
  QAction *action = kApp->mainWindow()->mapCanvas()->mapTool()->action();
  mButtonNewObject->setText( action->text() );
  mButtonNewObject->setIcon( action->icon() );
  mButtonNewObject->setChecked( true );
}

void KadasRedliningIntegration::deactivateNewButtonObject()
{
  mButtonNewObject->setText( tr( "Drawing" ) );
  mButtonNewObject->setIcon( QIcon( ":/kadas/icons/shape" ) );
  mButtonNewObject->setChecked( false );
}

void KadasRedliningIntegration::updateLastLayer( QgsMapLayer *layer )
{
  mLastLayer = dynamic_cast<KadasItemLayer *>( layer );
}
