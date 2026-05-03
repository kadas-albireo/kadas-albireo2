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

#include <functional>

#include <QAction>
#include <QMenu>
#include <QShortcut>
#include <QToolButton>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerregistry.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"
#include "kadas/gui/maptools/kadasmaptoolcreateannotationitem.h"

#include "kadasapplication.h"
#include "kadasmainwindow.h"
#include "kadasredliningintegration.h"


namespace
{
  //! Builds a marker annotation item with a simple-marker symbol layer of
  //! the given shape, matching the legacy KadasPointItem default style.
  QgsAnnotationMarkerItem *makeShapedMarker( Qgis::MarkerShape shape )
  {
    auto *item = new QgsAnnotationMarkerItem( QgsPoint() );
    auto *sl = new QgsSimpleMarkerSymbolLayer( shape );
    sl->setColor( Qt::yellow );
    sl->setStrokeColor( Qt::red );
    sl->setStrokeWidth( 0.4 );
    sl->setSize( 4 );
    item->setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << sl ) );
    return item;
  }
} // namespace


KadasRedliningIntegration::KadasRedliningIntegration( QToolButton *buttonNewObject, QObject *parent )
  : QObject( parent )
  , mButtonNewObject( buttonNewObject )
{
  QAction *actionNewMarker = new QAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Marker" ), this );

  using V = AnnotationVariant;

  mActionNewPoint = new QAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::MarkerCircle ); } );

  mActionNewSquare = new QAction( QIcon( ":/kadas/icons/redlining_square" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::MarkerSquare ); } );

  mActionNewTriangle = new QAction( QIcon( ":/kadas/icons/redlining_triangle" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::MarkerTriangle ); } );

  mActionNewLine = new QAction( QIcon( ":/kadas/icons/redlining_line" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::Line ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_L ), kApp->mainWindow() ), &QShortcut::activated, mActionNewLine, &QAction::trigger );

  mActionNewRectangle = new QAction( QIcon( ":/kadas/icons/redlining_rectangle" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::Rectangle ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_R ), kApp->mainWindow() ), &QShortcut::activated, mActionNewRectangle, &QAction::trigger );

  mActionNewPolygon = new QAction( QIcon( ":/kadas/icons/redlining_polygon" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::Polygon ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_P ), kApp->mainWindow() ), &QShortcut::activated, mActionNewPolygon, &QAction::trigger );

  mActionNewCircle = new QAction( QIcon( ":/kadas/icons/redlining_circle" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::Circle ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_C ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCircle, &QAction::trigger );

  mActionNewText = new QAction( QIcon( ":/kadas/icons/redlining_text" ), tr( "Text" ), this );
  mActionNewText->setCheckable( true );
  connect( mActionNewText, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::Text ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_T ), kApp->mainWindow() ), &QShortcut::activated, mActionNewText, &QAction::trigger );

  // CoordinateCross has no annotation-item equivalent yet; keep it on the legacy tool.
  mActionNewCoordCross = new QAction( QIcon( ":/kadas/icons/coord_cross" ), tr( "Coordinate Cross" ), this );
  mActionNewCoordCross->setCheckable( true );
  connect( mActionNewCoordCross, &QAction::triggered, this, [this]( bool active ) { toggleAnnotation( active, V::CoordCross ); } );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_O ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCoordCross, &QAction::trigger );

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

QgsAnnotationLayer *KadasRedliningIntegration::getOrCreateAnnotationLayer()
{
  if ( !mLastAnnotationLayer )
  {
    mLastAnnotationLayer = KadasAnnotationLayerRegistry::getOrCreateAnnotationLayer( KadasAnnotationLayerRegistry::StandardLayer::RedliningLayer );
  }
  return mLastAnnotationLayer;
}

void KadasRedliningIntegration::toggleAnnotation( bool active, AnnotationVariant variant )
{
  QgsMapCanvas *canvas = kApp->mainWindow()->mapCanvas();
  QAction *action = qobject_cast<QAction *>( QObject::sender() );
  if ( !active )
  {
    if ( canvas->mapTool() && canvas->mapTool()->action() == action )
      canvas->unsetMapTool( canvas->mapTool() );
    return;
  }

  // Resolve the controller and (optionally) a factory override per variant.
  QString typeId;
  std::function<QgsAnnotationItem *()> factory;
  switch ( variant )
  {
    case AnnotationVariant::MarkerCircle:
      typeId = QStringLiteral( "marker" );
      factory = [] { return makeShapedMarker( Qgis::MarkerShape::Circle ); };
      break;
    case AnnotationVariant::MarkerSquare:
      typeId = QStringLiteral( "marker" );
      factory = [] { return makeShapedMarker( Qgis::MarkerShape::Square ); };
      break;
    case AnnotationVariant::MarkerTriangle:
      typeId = QStringLiteral( "marker" );
      factory = [] { return makeShapedMarker( Qgis::MarkerShape::Triangle ); };
      break;
    case AnnotationVariant::Line:
      typeId = QStringLiteral( "linestring" );
      break;
    case AnnotationVariant::Polygon:
      typeId = QStringLiteral( "polygon" );
      break;
    case AnnotationVariant::Rectangle:
      typeId = KadasRectangleAnnotationItem::itemTypeId();
      break;
    case AnnotationVariant::Circle:
      typeId = KadasCircleAnnotationItem::itemTypeId();
      break;
    case AnnotationVariant::Text:
      typeId = QStringLiteral( "pointtext" );
      break;
    case AnnotationVariant::CoordCross:
      typeId = KadasCoordCrossAnnotationItem::itemTypeId();
      break;
  }

  KadasAnnotationItemController *controller = KadasAnnotationControllerRegistry::instance()->controllerFor( typeId );
  if ( !controller )
    return;

  QgsAnnotationLayer *layer = getOrCreateAnnotationLayer();
  if ( !layer )
    return;

  KadasMapToolCreateAnnotationItem *tool = new KadasMapToolCreateAnnotationItem( canvas, controller, layer );
  if ( factory )
    tool->setItemFactory( factory );
  tool->setMultipart( false );
  tool->setAction( action );

  connect( tool, &QgsMapTool::activated, this, &KadasRedliningIntegration::activateNewButtonObject );
  connect( tool, &QgsMapTool::deactivated, this, &KadasRedliningIntegration::deactivateNewButtonObject );

  kApp->mainWindow()->layerTreeView()->setCurrentLayer( layer );
  kApp->mainWindow()->layerTreeView()->setLayerVisible( layer, true );
  canvas->setMapTool( tool );
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
