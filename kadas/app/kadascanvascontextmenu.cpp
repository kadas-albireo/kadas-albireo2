/***************************************************************************
    kadascanvascontextmenu.cpp
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

#include <QClipboard>
#include <QInputDialog>

#include <qgis/qgsgeometryrubberband.h>
#include <qgis/qgsgeometrycollection.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsvectorlayer.h>

#include "kadas/core/kadascoordinateformat.h"
#include "kadas/gui/kadasclipboard.h"
#include "kadas/gui/kadasitemcontextmenuactions.h"
#include "kadas/gui/kadasitemlayer.h"
#include "kadas/gui/kadasmapcanvasitemmanager.h"
#include "kadas/gui/mapitems/kadasselectionrectitem.h"
#include "kadas/gui/maptools/kadasmaptoolcreateitem.h"
#include "kadas/gui/maptools/kadasmaptooledititem.h"
#include "kadas/gui/maptools/kadasmaptoolhillshade.h"
#include "kadas/gui/maptools/kadasmaptoolminmax.h"
#include "kadas/gui/maptools/kadasmaptoolslope.h"
#include "kadasapplication.h"
#include "kadascanvascontextmenu.h"
#include "kadasmainwindow.h"
#include "kadasmapidentifydialog.h"
#include "kadasredliningintegration.h"

const QString KadasCanvasContextMenu::ACTION_PROPERTY_MAP_POSITION("MapPosition");

QMap<QAction *, KadasCanvasContextMenu::Menu> KadasCanvasContextMenu::sRegisteredActions;

KadasCanvasContextMenu::KadasCanvasContextMenu( QgsMapCanvas *canvas, const QgsPointXY &mapPos )
  : mMapPos( mapPos ), mCanvas( canvas )
{
  mPickResult = KadasFeaturePicker::pick( mCanvas, mapPos );
  KadasMapItem *pickedItem = mPickResult.itemId != KadasItemLayer::ITEM_ID_NULL ? static_cast<KadasItemLayer *>( mPickResult.layer )->items()[mPickResult.itemId] : nullptr;
  Qgis::GeometryType geomType = Qgis::GeometryType::Unknown;
  if ( mPickResult.geom )
  {
    geomType = QgsWkbTypes::geometryType( mPickResult.geom->wkbType() );
  }

  if ( !pickedItem )
  {
    addAction( QgsApplication::getThemeIcon( "/mActionIdentify.svg" ), tr( "Identify" ), this, SLOT( identify() ) );
    addSeparator();
  }

  if ( pickedItem )
  {
    addAction( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ), tr( "Edit" ), this, &KadasCanvasContextMenu::editItem );
    addSeparator();
    addAction( QIcon( ":/kadas/icons/lower" ), tr( "Lower" ), this, &KadasCanvasContextMenu::lowerItem );
    addAction( QIcon( ":/kadas/icons/raise" ), tr( "Raise" ), this, &KadasCanvasContextMenu::raiseItem );
    KadasItemLayer *itemLayer = static_cast< KadasItemLayer * >( mPickResult.layer );
    mItemActions = new KadasItemContextMenuActions( mCanvas, this, pickedItem, itemLayer, mPickResult.itemId, this );
    mSelRect = new KadasSelectionRectItem( mCanvas->mapSettings().destinationCrs() );
    mSelRect->setSelectedItems( QList<KadasMapItem *>() << pickedItem );
    KadasMapCanvasItemManager::addItem( mSelRect );
  }
  else if ( mPickResult.feature.isValid() && mPickResult.layer )
  {
    addAction( QgsApplication::getThemeIcon( "/mActionEditCopy.svg" ), tr( "Copy" ), this, &KadasCanvasContextMenu::copyFeature );
    QgsCoordinateTransform ct( mPickResult.layer->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance()->transformContext() );
    QgsAbstractGeometry *geom = mPickResult.feature.geometry().constGet()->clone();
    geom->transform( ct );
    mGeomSel = new QgsGeometryRubberBand( mCanvas, mPickResult.feature.geometry().type() );
    mGeomSel->setIconType( QgsGeometryRubberBand::ICON_NONE );
    mGeomSel->setStrokeWidth( 2 );
    mGeomSel->setGeometry( geom );

  }

  if ( mPickResult.isEmpty() )
  {
    if ( !KadasClipboard::instance()->isEmpty() )
    {
      addAction( QgsApplication::getThemeIcon( "/mActionEditPaste.svg" ), tr( "Paste" ), this, &KadasCanvasContextMenu::paste );
    }
    QMenu *drawMenu = new QMenu();
    addAction( tr( "Draw" ) )->setMenu( drawMenu );
    drawMenu->addAction( QIcon( ":/kadas/icons/pin_red" ), tr( "Pin marker" ), this, &KadasCanvasContextMenu::drawPin );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_point" ), tr( "Point marker" ), this, &KadasCanvasContextMenu::drawPointMarker );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_square" ), tr( "Square marker" ), this, &KadasCanvasContextMenu::drawSquareMarker );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_triangle" ), tr( "Triangle marker" ), this, &KadasCanvasContextMenu::drawTriangleMarker );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_line" ), tr( "Line" ), this, &KadasCanvasContextMenu::drawLine );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_rectangle" ), tr( "Rectangle" ), this, &KadasCanvasContextMenu::drawRectangle );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_polygon" ), tr( "Polygon" ), this, &KadasCanvasContextMenu::drawPolygon );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_circle" ), tr( "Circle" ), this, &KadasCanvasContextMenu::drawCircle );
    drawMenu->addAction( QIcon( ":/kadas/icons/redlining_text" ), tr( "Text" ), this, &KadasCanvasContextMenu::drawText );
    drawMenu->addAction( QIcon( ":/kadas/icons/coord_cross" ), tr( "Coordinate Cross" ), this, &KadasCanvasContextMenu::drawCoordinateCross );
    addAction( QgsApplication::getThemeIcon( "/mIconSelectRemove.svg" ), tr( "Delete items" ), this, &KadasCanvasContextMenu::deleteItems );

    const QList<QAction *> registeredDrawActions = sRegisteredActions.keys( Menu::DRAW );
    for ( QAction *action : registeredDrawActions )
    {
      action->setProperty(ACTION_PROPERTY_MAP_POSITION.toUtf8().constData(), mMapPos);
      drawMenu->addAction( action );
    }
  }
  addSeparator();

  if ( mPickResult.isEmpty() || geomType == Qgis::GeometryType::Line || geomType == Qgis::GeometryType::Polygon )
  {
    QMenu *measureMenu = new QMenu();
    addAction( tr( "Measure" ) )->setMenu( measureMenu );

    if ( mPickResult.isEmpty() || geomType == Qgis::GeometryType::Line )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_line" ), tr( "Distance / Azimuth" ), this, &KadasCanvasContextMenu::measureLine );
    }
    if ( mPickResult.isEmpty() || geomType == Qgis::GeometryType::Polygon )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_area" ), tr( "Area" ), this, &KadasCanvasContextMenu::measurePolygon );
    }
    if ( mPickResult.isEmpty() || geomType == Qgis::GeometryType::Polygon )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_circle" ), tr( "Circle" ), this, SLOT( measureCircle() ) );
    }
    if ( mPickResult.isEmpty() || geomType == Qgis::GeometryType::Line )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_height_profile" ), tr( "Height profile" ), this, &KadasCanvasContextMenu::measureHeightProfile );
    }

    const QList<QAction *> registeredMeasureActions = sRegisteredActions.keys( Menu::MEASURE );
    for ( QAction *action : registeredMeasureActions )
    {
      action->setProperty(ACTION_PROPERTY_MAP_POSITION.toUtf8().constData(), mMapPos);
      measureMenu->addAction( action );
    }

    QMenu *analysisMenu = new QMenu();
    addAction( tr( "Terrain analysis" ) )->setMenu( analysisMenu );
    analysisMenu->addAction( QIcon( ":/kadas/icons/slope_color" ), tr( "Slope" ), this, &KadasCanvasContextMenu::terrainSlope );
    analysisMenu->addAction( QIcon( ":/kadas/icons/hillshade_color" ), tr( "Hillshade" ), this, &KadasCanvasContextMenu::terrainHillshade );
    if ( mPickResult.isEmpty() )
    {
      analysisMenu->addAction( QIcon( ":/kadas/icons/viewshed_color" ), tr( "Viewshed" ), this, &KadasCanvasContextMenu::terrainViewshed );
    }
    if ( mPickResult.isEmpty() || geomType == Qgis::GeometryType::Line )
    {
      analysisMenu->addAction( QIcon( ":/kadas/icons/measure_height_profile" ), tr( "Line of sight" ), this, &KadasCanvasContextMenu::measureHeightProfile );
    }
    if ( mPickResult.isEmpty() || ( geomType == Qgis::GeometryType::Polygon && mPickResult.geom->partCount() == 1 ) )
    {
      analysisMenu->addAction( QIcon( ":/kadas/icons/measure_min_max" ), tr( "Min/max" ), this, &KadasCanvasContextMenu::measureMinMax );
    }

    const QList<QAction *> registeredAnalysisActions = sRegisteredActions.keys( Menu::TERRAIN_ANALYSIS );
    for ( QAction *action : registeredAnalysisActions )
    {
      action->setProperty(ACTION_PROPERTY_MAP_POSITION.toUtf8().constData(), mMapPos);
      analysisMenu->addAction( action );
    }
  }

  if ( mPickResult.isEmpty() )
  {
    addAction( QIcon( ":/kadas/icons/copy_coordinates" ), tr( "Copy coordinates" ), this, &KadasCanvasContextMenu::copyCoordinates );
    addAction( QIcon( ":/kadas/icons/copy_map" ), tr( "Copy map" ), this, &KadasCanvasContextMenu::copyMap );
    addAction( QgsApplication::getThemeIcon( "/mActionFilePrint.svg" ), tr( "Print" ), this, &KadasCanvasContextMenu::print );
  }

  // Remaining actions
  const QList<QAction *> registeredActions = sRegisteredActions.keys( Menu::NONE );
  for ( QAction *action : registeredActions )
  {
    action->setProperty(ACTION_PROPERTY_MAP_POSITION.toUtf8().constData(), mMapPos);
    addAction( action );
  }
}

KadasCanvasContextMenu::~KadasCanvasContextMenu()
{
  for ( QAction *action : sRegisteredActions.keys() )
  {
    action->setProperty(ACTION_PROPERTY_MAP_POSITION.toUtf8().constData(), QVariant());
  }

  delete mGeomSel;
  delete mSelRect;
}

void KadasCanvasContextMenu::registerAction( QAction *action, Menu insertMenu )
{
  sRegisteredActions.insert( action, insertMenu );
}

void KadasCanvasContextMenu::unRegisterAction( QAction *action )
{
  sRegisteredActions.remove( action );
}

void KadasCanvasContextMenu::identify()
{
  KadasMapIdentifyDialog::popup( mCanvas, mMapPos );
}

void KadasCanvasContextMenu::copyCoordinates()
{
  const QgsCoordinateReferenceSystem &mapCrs = mCanvas->mapSettings().destinationCrs();
  QString posStr = KadasCoordinateFormat::instance()->getDisplayString( mMapPos, mapCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mMapPos.toString() ).arg( mapCrs.authid() );
  }
  QString text = QString( "%1\n%2" )
                 .arg( posStr )
                 .arg( KadasCoordinateFormat::instance()->getHeightAtPos( mMapPos, mapCrs ) );
  QApplication::clipboard()->setText( text );
}

void KadasCanvasContextMenu::copyFeature()
{
  QgsFeatureStore featureStore( static_cast<QgsVectorLayer *>( mPickResult.layer )->fields(), mPickResult.layer->crs() );
  featureStore.addFeature( mPickResult.feature );
  KadasClipboard::instance()->setStoredFeatures( featureStore );
}

void KadasCanvasContextMenu::copyMap()
{
  kApp->saveMapToClipboard();
}

void KadasCanvasContextMenu::deleteItems()
{
  kApp->mainWindow()->actionDeleteItems()->trigger();
}

void KadasCanvasContextMenu::editItem()
{
  mCanvas->setMapTool( new KadasMapToolEditItem( mCanvas, mPickResult.itemId, static_cast<KadasItemLayer *>( mPickResult.layer ) ) );
}

void KadasCanvasContextMenu::raiseItem()
{
  static_cast< KadasItemLayer * >( mPickResult.layer )->raiseItem( mPickResult.itemId );
}

void KadasCanvasContextMenu::lowerItem()
{
  static_cast< KadasItemLayer * >( mPickResult.layer )->lowerItem( mPickResult.itemId );
}

void KadasCanvasContextMenu::paste()
{
  mCanvas->setMapTool( kApp->paste( &mMapPos ) );
}

void KadasCanvasContextMenu::drawPin()
{
  kApp->mainWindow()->actionPin()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawPointMarker()
{
  kApp->mainWindow()->redliningIntegration()->actionNewPoint()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawSquareMarker()
{
  kApp->mainWindow()->redliningIntegration()->actionNewSquare()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawTriangleMarker()
{
  kApp->mainWindow()->redliningIntegration()->actionNewTriangle()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawLine()
{
  kApp->mainWindow()->redliningIntegration()->actionNewLine()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawRectangle()
{
  kApp->mainWindow()->redliningIntegration()->actionNewRectangle()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawPolygon()
{
  kApp->mainWindow()->redliningIntegration()->actionNewPolygon()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawCircle()
{
  kApp->mainWindow()->redliningIntegration()->actionNewCircle()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawText()
{
  kApp->mainWindow()->redliningIntegration()->actionNewText()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::drawCoordinateCross()
{
  kApp->mainWindow()->redliningIntegration()->actionNewCoordinateCross()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::measureLine()
{
  kApp->mainWindow()->actionMeasureLine()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  if ( mPickResult.geom )
  {
    tool->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
  else
  {
    tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
  }
}

void KadasCanvasContextMenu::measurePolygon()
{
  kApp->mainWindow()->actionMeasureArea()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  if ( mPickResult.geom )
  {
    tool->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
  else
  {
    tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
  }
}

void KadasCanvasContextMenu::measureCircle()
{
  kApp->mainWindow()->actionMeasureCircle()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  if ( mPickResult.geom )
  {
    tool->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
  else
  {
    tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
  }
}

void KadasCanvasContextMenu::measureHeightProfile()
{
  kApp->mainWindow()->actionMeasureHeightProfile()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  if ( mPickResult.geom )
  {
    static_cast<KadasMapToolCreateItem *>( tool )->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
  else
  {
    tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
  }
}

void KadasCanvasContextMenu::measureMinMax()
{
  kApp->mainWindow()->actionMeasureMinMax()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  if ( mPickResult.geom )
  {
    QgsAbstractGeometry *geom = dynamic_cast<QgsGeometryCollection *>( mPickResult.geom ) ? static_cast<QgsGeometryCollection *>( mPickResult.geom )->geometryN( 0 ) : mPickResult.geom;
    if ( QgsWkbTypes::flatType( geom->wkbType() ) == Qgis::WkbType::CurvePolygon )
    {
      static_cast<KadasMapToolMinMax *>( tool )->setFilterType( KadasMapToolMinMax::FilterType::FilterCircle );
    }
    else
    {
      static_cast<KadasMapToolMinMax *>( tool )->setFilterType( KadasMapToolMinMax::FilterType::FilterPoly );
    }
    tool->addPartFromGeometry( *geom, mPickResult.crs );
  }
  else
  {
    tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
  }
}

void KadasCanvasContextMenu::terrainSlope()
{
  if ( mPickResult.geom )
  {
    KadasMapToolSlope( kApp->mainWindow()->mapCanvas() ).compute( mPickResult.geom->boundingBox(), mPickResult.crs );
  }
  else
  {
    kApp->mainWindow()->actionTerrainSlope()->trigger();
  }
}

void KadasCanvasContextMenu::terrainHillshade()
{
  if ( mPickResult.geom )
  {
    KadasMapToolHillshade( kApp->mainWindow()->mapCanvas() ).compute( mPickResult.geom->boundingBox(), mPickResult.crs );
  }
  else
  {
    kApp->mainWindow()->actionTerrainHillshade()->trigger();
  }
}

void KadasCanvasContextMenu::terrainViewshed()
{
  kApp->mainWindow()->actionTerrainViewshed()->trigger();

  KadasMapToolCreateItem *tool = dynamic_cast<KadasMapToolCreateItem *>( kApp->mainWindow()->mapCanvas()->mapTool() );
  if ( !tool )
    return;

  tool->addPoint( KadasMapPos::fromPoint( mMapPos ) );
}

void KadasCanvasContextMenu::print()
{
  QAction *printAction = kApp->mainWindow()->findChild<QAction *>( "mActionPrint" );
  if ( printAction && !printAction->isChecked() )
  {
    printAction->trigger();
  }
}
