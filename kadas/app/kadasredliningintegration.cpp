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
#include <qgis/qgsapplication.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssvgcache.h>

#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationitemcontroller.h"
#include "kadas/gui/annotationitems/kadasannotationlayerregistry.h"
#include "kadas/gui/annotationitems/kadascircleannotationitem.h"
#include "kadas/gui/annotationitems/kadascoordcrossannotationitem.h"
#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasrectangleannotationitem.h"
#include "kadas/gui/annotationitems/kadassvgmarkerannotationcontroller.h"
#include "kadas/gui/annotationitems/kadassvgmarkerannotationitem.h"
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
  mActionNewCustomSvg = createToolAction( QIcon( KadasSvgMarkerAnnotationItem::placeholderIconPath() ), tr( "Custom SVG" ), QStringLiteral( "draw-marker-svg" ), V::MarkerCustomSvg );
  // Show the actual SVG (in colour) on the ribbon tile and gallery, and keep it
  // live-updated as the user picks a different SVG, rather than a monochrome
  // silhouette shared by the built-in marker shapes.
  mActionNewCustomSvg->setProperty( "kadasFullColorIcon", true );
  updateCustomSvgActionIcon();
  mMarkerActions = { mActionNewPoint, mActionNewSquare, mActionNewTriangle, mActionNewDiamond, mActionNewStar, mActionNewCross, mActionNewCustomSvg };

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

  mActionNewTextAlongLine = createToolAction( QIcon( ":/kadas/icons/draw_text" ), tr( "Text along Line" ), QStringLiteral( "draw-linetext" ), V::TextAlongLine );
  connect( new QShortcut( QKeySequence( Qt::CTRL | Qt::Key_D, Qt::CTRL | Qt::Key_E ), kApp->mainWindow() ), &QShortcut::activated, mActionNewTextAlongLine, &QAction::trigger );

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
      KadasMarkerAnnotationController::settingsShape->setValue( static_cast<int>( Qgis::MarkerShape::Circle ) );
      break;
    case AnnotationVariant::MarkerSquare:
      typeId = QStringLiteral( "marker" );
      KadasMarkerAnnotationController::settingsShape->setValue( static_cast<int>( Qgis::MarkerShape::Square ) );
      break;
    case AnnotationVariant::MarkerTriangle:
      typeId = QStringLiteral( "marker" );
      KadasMarkerAnnotationController::settingsShape->setValue( static_cast<int>( Qgis::MarkerShape::Triangle ) );
      break;
    case AnnotationVariant::MarkerDiamond:
      typeId = QStringLiteral( "marker" );
      KadasMarkerAnnotationController::settingsShape->setValue( static_cast<int>( Qgis::MarkerShape::Diamond ) );
      break;
    case AnnotationVariant::MarkerStar:
      typeId = QStringLiteral( "marker" );
      KadasMarkerAnnotationController::settingsShape->setValue( static_cast<int>( Qgis::MarkerShape::Star ) );
      break;
    case AnnotationVariant::MarkerCross:
      typeId = QStringLiteral( "marker" );
      KadasMarkerAnnotationController::settingsShape->setValue( static_cast<int>( Qgis::MarkerShape::Cross ) );
      break;
    case AnnotationVariant::MarkerCustomSvg:
      typeId = KadasSvgMarkerAnnotationItem::itemTypeId();
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
    case AnnotationVariant::TextAlongLine:
      typeId = QStringLiteral( "linetext" );
      break;
    case AnnotationVariant::CoordCross:
      typeId = KadasCoordCrossAnnotationItem::itemTypeId();
      break;
  }

  // Marker variants share one factory that honours the last-used shape (set by
  // the ribbon button above and updated by style edits), so a newly placed
  // marker keeps the most recently used shape.
  if ( typeId == QLatin1String( "marker" ) )
  {
    factory = [] { return makeShapedMarker( static_cast<Qgis::MarkerShape>( KadasMarkerAnnotationController::settingsShape->value() ) ); };
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

  // Once a custom-SVG marker has been placed/edited, the last-used SVG is
  // persisted; reflect it on the ribbon tile so it becomes the visible default.
  if ( variant == AnnotationVariant::MarkerCustomSvg )
  {
    connect( tool, &QgsMapTool::deactivated, this, [this] { updateCustomSvgActionIcon(); } );
    connect( tool, &KadasMapToolEditAnnotationItem::stylePersisted, this, [this] { updateCustomSvgActionIcon(); } );
  }

  kApp->mainWindow()->layerTreeView()->setCurrentLayer( layer );
  kApp->mainWindow()->layerTreeView()->setLayerVisible( layer, true );
  canvas->setMapTool( tool );
}

void KadasRedliningIntegration::updateCustomSvgActionIcon()
{
  if ( !mActionNewCustomSvg )
    return;
  QString path = KadasSvgMarkerAnnotationController::settingsSvgPath->value();
  if ( path.isEmpty() )
    path = KadasSvgMarkerAnnotationItem::placeholderIconPath();

  // Render the SVG to a square thumbnail that fully fits the tile. Rendering a
  // real marker symbol would clip, since its size is expressed in map units and
  // easily overflows a small icon canvas.
  bool fitsInCache = false;
  const QImage img = QgsApplication::svgCache()->svgAsImage( path, 64, KadasSvgMarkerAnnotationController::settingsSvgFillColor->value(), QColor( 0, 0, 0 ), 0.0, 1.0, fitsInCache );
  QIcon icon;
  if ( !img.isNull() )
    icon = QIcon( QPixmap::fromImage( img ) );
  if ( icon.isNull() )
    icon = QIcon( KadasSvgMarkerAnnotationItem::placeholderIconPath() );
  mActionNewCustomSvg->setIcon( icon );
}
