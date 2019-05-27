/***************************************************************************
    kadasredliningintergration.cpp
    ------------------------------
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

#include <kadas/core/kadasitemlayer.h>
#include <kadas/core/mapitems/kadascircleitem.h>
#include <kadas/core/mapitems/kadaslineitem.h>
#include <kadas/core/mapitems/kadaspointitem.h>
#include <kadas/core/mapitems/kadaspolygonitem.h>
#include <kadas/core/mapitems/kadasrectangleitem.h>

#include <kadas/gui/mapitemeditors/kadasredliningitemeditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasredliningintergration.h>

KadasRedliningIntegration::KadasRedliningIntegration(QToolButton *buttonNewObject, KadasMainWindow *main )
  : mButtonNewObject( buttonNewObject )
{
  mMainWindow = main;
  mCanvas = main->mapCanvas();

  KadasMapToolCreateItem::ItemFactory pointFactory = [=] {
    return setEditorFactory(new KadasPointItem(mCanvas->mapSettings().destinationCrs(), KadasPointItem::ICON_CIRCLE));
  };
  KadasMapToolCreateItem::ItemFactory squareFactory = [=] {
    return setEditorFactory(new KadasPointItem(mCanvas->mapSettings().destinationCrs(), KadasPointItem::ICON_FULL_BOX));
  };
  KadasMapToolCreateItem::ItemFactory triangleFactory = [=] {
    return setEditorFactory(new KadasPointItem(mCanvas->mapSettings().destinationCrs(), KadasPointItem::ICON_FULL_TRIANGLE));
  };
  KadasMapToolCreateItem::ItemFactory lineFactory = [=] {
    return setEditorFactory(new KadasLineItem(mCanvas->mapSettings().destinationCrs()));
  };
  KadasMapToolCreateItem::ItemFactory rectangleFactory = [=] {
    return setEditorFactory(new KadasRectangleItem(mCanvas->mapSettings().destinationCrs()));
  };
  KadasMapToolCreateItem::ItemFactory polygonFactory = [=] {
    return setEditorFactory(new KadasPolygonItem(mCanvas->mapSettings().destinationCrs()));
  };
  KadasMapToolCreateItem::ItemFactory circleFactory = [=] {
    return setEditorFactory(new KadasCircleItem(mCanvas->mapSettings().destinationCrs()));
  };


  QAction* actionNewMarker = new QAction( QIcon( ":/images/icons/redlining_point" ), tr( "Marker" ), this );

  mActionNewPoint = new QAction( QIcon( ":/images/icons/redlining_point" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, pointFactory); });

  mActionNewSquare = new QAction( QIcon( ":/images/icons/redlining_square" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, squareFactory); });

  mActionNewTriangle = new QAction( QIcon( ":/images/icons/redlining_triangle" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, triangleFactory); });

  mActionNewLine = new QAction( QIcon( ":/images/icons/redlining_line" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, lineFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_L ), main ), &QShortcut::activated, mActionNewLine, &QAction::trigger );

  mActionNewRectangle = new QAction( QIcon( ":/images/icons/redlining_rectangle" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, rectangleFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_R ), main ), &QShortcut::activated, mActionNewRectangle, &QAction::trigger );

  mActionNewPolygon = new QAction( QIcon( ":/images/icons/redlining_polygon" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, polygonFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_P ), main ), &QShortcut::activated, mActionNewPolygon, &QAction::trigger );

  mActionNewCircle = new QAction( QIcon( ":/images/icons/redlining_circle" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, &QAction::triggered, this, [=](bool active){ toggleCreateItem(active, circleFactory); });
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_C ), main ), &QShortcut::activated, mActionNewCircle, &QAction::trigger );

  // TODO
//  mActionNewText = new QAction( QIcon( ":/images/icons/redlining_text" ), tr( "Text" ), this );
//  mActionNewText->setCheckable( true );
//  connect( mActionNewText, &QAction::triggered, this, &KadasRedliningIntegration::setTextTool );
//  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_T ), main ), &QShortcut::activated, mActionNewText, &QAction::trigger );

  QMenu* menuNewMarker = new QMenu();
  menuNewMarker->addAction( mActionNewPoint );
  menuNewMarker->addAction( mActionNewSquare );
  menuNewMarker->addAction( mActionNewTriangle );
  actionNewMarker->setMenu( menuNewMarker );
  QMenu* menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewMarker );
  menuNewObject->addAction( mActionNewLine );
  menuNewObject->addAction( mActionNewRectangle );
  menuNewObject->addAction( mActionNewPolygon );
  menuNewObject->addAction( mActionNewCircle );
  menuNewObject->addAction( mActionNewText );
  mButtonNewObject->setMenu( menuNewObject );
  mButtonNewObject->setPopupMode( QToolButton::InstantPopup );
  mButtonNewObject->setIcon( QIcon( ":/images/icons/shape" ) );
}

KadasItemLayer* KadasRedliningIntegration::getOrCreateLayer()
{
  if(!getLayer()) {
    KadasItemLayer* layer = new KadasItemLayer( tr( "Redlining" ) );
    mLayerId = layer->id();
    QgsProject::instance()->addMapLayer( layer );
  }
  return getLayer();
}

KadasItemLayer* KadasRedliningIntegration::getLayer() const
{
  return qobject_cast<KadasItemLayer*>(QgsProject::instance()->mapLayer(mLayerId));
}

KadasMapItem* KadasRedliningIntegration::setEditorFactory(KadasMapItem *item) const
{
  item->setEditorFactory(KadasRedliningItemEditor::factory);
  return item;
}

void KadasRedliningIntegration::toggleCreateItem(bool active, const std::function<KadasMapItem*()>& itemFactory)
{
  QAction* action = qobject_cast<QAction*>(QObject::sender());
  if ( active )
  {
    KadasMapToolCreateItem* tool = new KadasMapToolCreateItem(mCanvas, itemFactory, getOrCreateLayer());
    tool->setAction( action );
    connect( tool, &QgsMapTool::deactivated, tool, &QObject::deleteLater );
    mMainWindow->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    mMainWindow->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    mCanvas->setMapTool( tool );
  }
  else if ( !active && mCanvas->mapTool() && mCanvas->mapTool()->action() == action )
  {
    mCanvas->unsetMapTool( mCanvas->mapTool() );
  }
}
