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
#include <QActionGroup>
#include <QIcon>
#include <QShortcut>

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
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"

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
    sl->setColor( Qt::white );
    sl->setStrokeColor( Qt::red );
    sl->setStrokeWidth( 0.4 );
    sl->setSize( 4 );
    item->setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << sl ) );
    return item;
  }
} // namespace


KadasRedliningIntegration::KadasRedliningIntegration( QObject *parent )
  : QObject( parent )
{
  using V = AnnotationVariant;

  mActionGroup = new QActionGroup( this );
  mActionGroup->setExclusionPolicy( QActionGroup::ExclusionPolicy::ExclusiveOptional );

  // Markers: a curated subset of the simple-marker shapes from the style editor.
  mActionNewPoint = createToolAction( QIcon( ":/kadas/icons/draw_point" ), tr( "Point" ), QStringLiteral( "draw-marker-circle" ), V::MarkerCircle );
  mActionNewSquare = createToolAction( QIcon( ":/kadas/icons/draw_square" ), tr( "Square" ), QStringLiteral( "draw-marker-square" ), V::MarkerSquare );
  mActionNewTriangle = createToolAction( QIcon( ":/kadas/icons/draw_triangle" ), tr( "Triangle" ), QStringLiteral( "draw-marker-triangle" ), V::MarkerTriangle );
  mActionNewDiamond = createToolAction( QIcon( ":/kadas/icons/draw_diamond" ), tr( "Diamond" ), QStringLiteral( "draw-marker-diamond" ), V::MarkerDiamond );
  mActionNewStar = createToolAction( QIcon( ":/kadas/icons/draw_star" ), tr( "Star" ), QStringLiteral( "draw-marker-star" ), V::MarkerStar );
  mActionNewCross = createToolAction( QIcon( ":/kadas/icons/draw_coordcross" ), tr( "Cross" ), QStringLiteral( "draw-marker-cross" ), V::MarkerCross );
  mMarkerActions = { mActionNewPoint, mActionNewSquare, mActionNewTriangle, mActionNewDiamond, mActionNewStar, mActionNewCross };

  // Shapes: lines and polygons.
  mActionNewLine = createToolAction( QIcon( ":/kadas/icons/draw_line" ), tr( "Line" ), QStringLiteral( "draw-line" ), V::Line );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_L ), kApp->mainWindow() ), &QShortcut::activated, mActionNewLine, &QAction::trigger );

  mActionNewPolygon = createToolAction( QIcon( ":/kadas/icons/draw_polygon" ), tr( "Polygon" ), QStringLiteral( "draw-polygon" ), V::Polygon );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_P ), kApp->mainWindow() ), &QShortcut::activated, mActionNewPolygon, &QAction::trigger );

  mActionNewRectangle = createToolAction( QIcon( ":/kadas/icons/draw_rectangle" ), tr( "Rectangle" ), QStringLiteral( "draw-rectangle" ), V::Rectangle );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_R ), kApp->mainWindow() ), &QShortcut::activated, mActionNewRectangle, &QAction::trigger );

  mActionNewCircle = createToolAction( QIcon( ":/kadas/icons/draw_circle" ), tr( "Circle" ), QStringLiteral( "draw-circle" ), V::Circle );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_C ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCircle, &QAction::trigger );
  mShapeActions = { mActionNewLine, mActionNewPolygon, mActionNewRectangle, mActionNewCircle };

  // Other annotation tools.
  mActionNewText = createToolAction( QIcon( ":/kadas/icons/draw_text" ), tr( "Text" ), QStringLiteral( "draw-text" ), V::Text );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_T ), kApp->mainWindow() ), &QShortcut::activated, mActionNewText, &QAction::trigger );

  // CoordinateCross has no annotation-item equivalent yet; keep it on the legacy tool.
  mActionNewCoordCross = createToolAction( QIcon( ":/kadas/icons/draw_coordcross" ), tr( "Coordinate Cross" ), QStringLiteral( "draw-coordcross" ), V::CoordCross );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_O ), kApp->mainWindow() ), &QShortcut::activated, mActionNewCoordCross, &QAction::trigger );
}

QAction *KadasRedliningIntegration::createToolAction( const QIcon &icon, const QString &text, const QString &objectName, AnnotationVariant variant )
{
  QAction *action = new QAction( icon, text, this );
  action->setObjectName( objectName );
  action->setCheckable( true );
  mActionGroup->addAction( action );
  connect( action, &QAction::toggled, this, [this, variant]( bool active ) { toggleAnnotation( active, variant ); } );
  return action;
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
    case AnnotationVariant::MarkerDiamond:
      typeId = QStringLiteral( "marker" );
      factory = [] { return makeShapedMarker( Qgis::MarkerShape::Diamond ); };
      break;
    case AnnotationVariant::MarkerStar:
      typeId = QStringLiteral( "marker" );
      factory = [] { return makeShapedMarker( Qgis::MarkerShape::Star ); };
      break;
    case AnnotationVariant::MarkerCross:
      typeId = QStringLiteral( "marker" );
      factory = [] { return makeShapedMarker( Qgis::MarkerShape::Cross ); };
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

  KadasMapToolEditAnnotationItem *tool = new KadasMapToolEditAnnotationItem( canvas, controller, layer );
  if ( factory )
    tool->setItemFactory( factory );
  tool->setMultipart( false );
  tool->setAction( action );

  // When the tool is deactivated (Escape, replaced by another tool, ...),
  // keep its action unchecked so the owning split button reflects it.
  connect( tool, &QgsMapTool::deactivated, action, [action] { action->setChecked( false ); } );

  kApp->mainWindow()->layerTreeView()->setCurrentLayer( layer );
  kApp->mainWindow()->layerTreeView()->setLayerVisible( layer, true );
  canvas->setMapTool( tool );
}
