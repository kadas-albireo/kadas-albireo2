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

#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/mapitems/kadascircleitem.h>
#include <kadas/gui/mapitems/kadaslineitem.h>
#include <kadas/gui/mapitems/kadaspointitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadasrectangleitem.h>
#include <kadas/gui/mapitems/kadastextitem.h>
#include <kadas/gui/mapitemeditors/kadasredliningitemeditor.h>
#include <kadas/gui/mapitemeditors/kadasredliningtexteditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasredliningintegration.h>

KadasRedliningIntegration::KadasRedliningIntegration( QToolButton *buttonNewObject, QObject *parent )
  : QObject( parent ), mButtonNewObject( buttonNewObject )
{

  QgsCoordinateReferenceSystem crsMercator( "EPSG:3857" );

  KadasMapToolCreateItem::ItemFactory pointFactory = [ = ]
  {
    return setEditorFactory( new KadasPointItem( crsMercator, KadasPointItem::ICON_CIRCLE ) );
  };
  KadasMapToolCreateItem::ItemFactory squareFactory = [ = ]
  {
    return setEditorFactory( new KadasPointItem( crsMercator, KadasPointItem::ICON_FULL_BOX ) );
  };
  KadasMapToolCreateItem::ItemFactory triangleFactory = [ = ]
  {
    return setEditorFactory( new KadasPointItem( crsMercator, KadasPointItem::ICON_FULL_TRIANGLE ) );
  };
  KadasMapToolCreateItem::ItemFactory lineFactory = [ = ]
  {
    return setEditorFactory( new KadasLineItem( crsMercator ) );
  };
  KadasMapToolCreateItem::ItemFactory rectangleFactory = [ = ]
  {
    return setEditorFactory( new KadasRectangleItem( crsMercator ) );
  };
  KadasMapToolCreateItem::ItemFactory polygonFactory = [ = ]
  {
    return setEditorFactory( new KadasPolygonItem( crsMercator ) );
  };
  KadasMapToolCreateItem::ItemFactory circleFactory = [ = ]
  {
    return setEditorFactory( new KadasCircleItem( crsMercator ) );
  };
  KadasMapToolCreateItem::ItemFactory textFactory = [ = ]
  {
    KadasTextItem *textItem = new KadasTextItem( crsMercator );
    textItem->setEditor( "KadasRedliningTextEditor" );
    return textItem;
  };


  QAction *actionNewMarker = new QAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Marker" ), this );

  mActionNewPoint = new QAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, pointFactory ); } );

  mActionNewSquare = new QAction( QIcon( ":/kadas/icons/redlining_square" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, squareFactory ); } );

  mActionNewTriangle = new QAction( QIcon( ":/kadas/icons/redlining_triangle" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, triangleFactory ); } );

  mActionNewLine = new QAction( QIcon( ":/kadas/icons/redlining_line" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, lineFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_L ), kApp->mainWindow() ), &QShortcut::activated, mActionNewLine, &QAction::trigger );

  mActionNewRectangle = new QAction( QIcon( ":/kadas/icons/redlining_rectangle" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, rectangleFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_R ), kApp->mainWindow() ), &QShortcut::activated, mActionNewRectangle, &QAction::trigger );

  mActionNewPolygon = new QAction( QIcon( ":/kadas/icons/redlining_polygon" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, polygonFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_P ), kApp->mainWindow() ), &QShortcut::activated, mActionNewPolygon, &QAction::trigger );

  mActionNewCircle = new QAction( QIcon( ":/kadas/icons/redlining_circle" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, circleFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_C ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCircle, &QAction::trigger );

  mActionNewText = new QAction( QIcon( ":/kadas/icons/redlining_text" ), tr( "Text" ), this );
  mActionNewText->setCheckable( true );
  connect( mActionNewText, &QAction::triggered, this, [ = ]( bool active ) { toggleCreateItem( active, textFactory ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_T ), kApp->mainWindow() ), &QShortcut::activated, mActionNewText, &QAction::trigger );

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
  mButtonNewObject->setMenu( menuNewObject );
  mButtonNewObject->setPopupMode( QToolButton::InstantPopup );
  mButtonNewObject->setIcon( QIcon( ":/kadas/icons/shape" ) );
  mButtonNewObject->setCheckable( true );
}

KadasItemLayer *KadasRedliningIntegration::getOrCreateLayer()
{
  return kApp->getOrCreateItemLayer( tr( "Redlining" ) );
}

KadasMapItem *KadasRedliningIntegration::setEditorFactory( KadasMapItem *item ) const
{
  item->setEditor( "KadasRedliningItemEditor" );
  return item;
}

void KadasRedliningIntegration::toggleCreateItem( bool active, const std::function<KadasMapItem*() > &itemFactory )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  QAction *action = qobject_cast<QAction *> ( QObject::sender() );
  if ( active )
  {
    KadasMapToolCreateItem *tool = new KadasMapToolCreateItem( canvas, itemFactory, getOrCreateLayer() );
    KadasLayerSelectionWidget::LayerFilter filter = []( QgsMapLayer * layer ) { return dynamic_cast<KadasItemLayer *>( layer ); };
    KadasLayerSelectionWidget::LayerCreator creator = []( const QString & name ) { return kApp->getOrCreateItemLayer( name ); };
    tool->showLayerSelection( true, kApp->mainWindow()->layerTreeView(), filter, creator );
    tool->setAction( action );
    connect( tool, &QgsMapTool::activated, this, &KadasRedliningIntegration::activateNewButtonObject );
    connect( tool, &QgsMapTool::deactivated, this, &KadasRedliningIntegration::deactivateNewButtonObject );
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
