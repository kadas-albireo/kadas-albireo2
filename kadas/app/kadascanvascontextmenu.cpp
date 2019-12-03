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
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadascircleitem.h>
#include <kadas/gui/mapitems/kadasgeometryitem.h>
#include <kadas/gui/mapitems/kadasgpxwaypointitem.h>
#include <kadas/gui/mapitems/kadaspolygonitem.h>
#include <kadas/gui/mapitems/kadasselectionrectitem.h>
#include <kadas/gui/mapitems/kadassymbolitem.h>
#include <kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h>
#include <kadas/gui/mapitemeditors/kadassymbolattributeseditor.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/gui/maptools/kadasmaptoolhillshade.h>
#include <kadas/gui/maptools/kadasmaptoolslope.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadascanvascontextmenu.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasmapidentifydialog.h>
#include <kadas/app/kadasredliningintegration.h>

KadasCanvasContextMenu::KadasCanvasContextMenu( QgsMapCanvas *canvas, const QPoint &canvasPos, const QgsPointXY &mapPos )
  : mMapPos( mapPos ), mCanvas( canvas )
{
  mPickResult = KadasFeaturePicker::pick( mCanvas, canvasPos, mapPos );
  KadasMapItem *pickedItem = mPickResult.itemId != KadasItemLayer::ITEM_ID_NULL ? static_cast<KadasItemLayer *>( mPickResult.layer )->items()[mPickResult.itemId] : nullptr;
  QgsWkbTypes::GeometryType geomType = QgsWkbTypes::UnknownGeometry;
  if ( mPickResult.geom )
  {
    geomType = QgsWkbTypes::geometryType( mPickResult.geom->wkbType() );
  }

  addAction( QIcon( ":/images/themes/default/mActionIdentify.svg" ), tr( "Identify" ), this, SLOT( identify() ) );
  addSeparator();

  if ( pickedItem )
  {
    addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, &KadasCanvasContextMenu::editItem );
    if ( dynamic_cast<KadasPinItem *>( pickedItem ) )
    {
      addAction( QIcon( ":/kadas/icons/copy_coordinates" ), tr( "Copy position" ), this, &KadasCanvasContextMenu::copyItemPosition );
      addAction( QIcon( ":/images/themes/default/mIconPointLayer.svg" ), tr( "Convert to waypoint" ), this, &KadasCanvasContextMenu::convertPinToWaypoint );
    }
    else if ( dynamic_cast<KadasPointItem *>( pickedItem ) )
    {
      addAction( QIcon( ":/kadas/icons/pin_red" ), tr( "Convert to pin" ), this, &KadasCanvasContextMenu::convertWaypointToPin );
    }
    else if ( dynamic_cast<KadasCircleItem *>( pickedItem ) )
    {
      addAction( QIcon( ":/kadas/icons/polygon" ), tr( "Convert to polygon" ), this, &KadasCanvasContextMenu::convertCircleToPolygon );
    }
    addAction( QIcon( ":/images/themes/default/mActionEditCut.svg" ), tr( "Cut" ), this, &KadasCanvasContextMenu::cutItem );
    addAction( QIcon( ":/images/themes/default/mActionEditCopy.svg" ), tr( "Copy" ), this, &KadasCanvasContextMenu::copyItem );
    addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, &KadasCanvasContextMenu::deleteItem );
    mSelRect = new KadasSelectionRectItem( mCanvas->mapSettings().destinationCrs(), this );
    mSelRect->setSelectedItems( QList<KadasMapItem *>() << pickedItem );
    KadasMapCanvasItemManager::addItem( mSelRect );
  }
  else if ( mPickResult.feature.isValid() && mPickResult.layer )
  {
    addAction( QIcon( ":/images/themes/default/mActionEditCopy.svg" ), tr( "Copy" ), this, &KadasCanvasContextMenu::copyFeature );
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
      addAction( QIcon( ":/images/themes/default/mActionEditPaste.svg" ), tr( "Paste" ), this, &KadasCanvasContextMenu::paste );
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
    addAction( QIcon( ":/images/themes/default/mIconSelectRemove.svg" ), tr( "Delete items" ), this, &KadasCanvasContextMenu::deleteItems );
  }
  addSeparator();
  if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::LineGeometry || geomType == QgsWkbTypes::PolygonGeometry )
  {
    QMenu *measureMenu = new QMenu();
    addAction( tr( "Measure" ) )->setMenu( measureMenu );

    if ( mPickResult.isEmpty() || ( geomType == QgsWkbTypes::LineGeometry ) )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_line" ), tr( "Distance" ), this, &KadasCanvasContextMenu::measureLine );
    }
    if ( mPickResult.isEmpty() || ( geomType == QgsWkbTypes::PolygonGeometry ) )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_area" ), tr( "Area" ), this, &KadasCanvasContextMenu::measurePolygon );
    }
    if ( mPickResult.isEmpty() || ( geomType == QgsWkbTypes::PolygonGeometry ) )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_circle" ), tr( "Circle" ), this, SLOT( measureCircle() ) );
    }
    if ( mPickResult.isEmpty() )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_angle" ), tr( "Azimuth" ), this, &KadasCanvasContextMenu::measureAzimuth );
    }
    if ( mPickResult.isEmpty() || ( geomType == QgsWkbTypes::LineGeometry ) )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_height_profile" ), tr( "Height profile" ), this, &KadasCanvasContextMenu::measureHeightProfile );
    }

    QMenu *analysisMenu = new QMenu();
    addAction( tr( "Terrain analysis" ) )->setMenu( analysisMenu );
    analysisMenu->addAction( QIcon( ":/kadas/icons/slope_color" ), tr( "Slope" ), this, &KadasCanvasContextMenu::terrainSlope );
    analysisMenu->addAction( QIcon( ":/kadas/icons/hillshade_color" ), tr( "Hillshade" ), this, &KadasCanvasContextMenu::terrainHillshade );
    if ( mPickResult.isEmpty() )
    {
      analysisMenu->addAction( QIcon( ":/kadas/icons/viewshed_color" ), tr( "Viewshed" ), this, &KadasCanvasContextMenu::terrainViewshed );
    }
    if ( mPickResult.isEmpty() || ( geomType == QgsWkbTypes::LineGeometry ) )
    {
      analysisMenu->addAction( QIcon( ":/kadas/icons/measure_height_profile" ), tr( "Line of sight" ), this, &KadasCanvasContextMenu::measureHeightProfile );
    }
  }

  if ( mPickResult.isEmpty() )
  {
    addAction( QIcon( ":/kadas/icons/copy_coordinates" ), tr( "Copy coordinates" ), this, [this] { copyCoordinates( mMapPos ); } );
    addAction( QIcon( ":/kadas/icons/copy_map" ), tr( "Copy map" ), this, &KadasCanvasContextMenu::copyMap );
    addAction( QIcon( ":/images/themes/default/mActionFilePrint.svg" ), tr( "Print" ), this, &KadasCanvasContextMenu::print );
  }
}

KadasCanvasContextMenu::~KadasCanvasContextMenu()
{
  delete mGeomSel;
  delete mSelRect;
}

void KadasCanvasContextMenu::identify()
{
  KadasMapIdentifyDialog::popup( mCanvas, mMapPos );
}

void KadasCanvasContextMenu::convertWaypointToPin()
{
  KadasMapItem *pickedItem = static_cast<KadasItemLayer *>( mPickResult.layer )->takeItem( mPickResult.itemId );
  KadasPointItem *pointItem = dynamic_cast<KadasPointItem *>( pickedItem );

  KadasPinItem *pin = new KadasPinItem( QgsCoordinateReferenceSystem( "EPSG:3857" ) );
  pin->setEditor( "KadasSymbolAttributesEditor" );
  if ( dynamic_cast<KadasGpxWaypointItem *>( pointItem ) )
  {
    pin->setName( static_cast<KadasGpxWaypointItem *>( pointItem )->name() );
  }
  QgsCoordinateTransform crst( pointItem->crs(), pin->crs(), QgsProject::instance()->transformContext() );
  pin->setPosition( KadasItemPos::fromPoint( crst.transform( pointItem->position() ) ) );
  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::PinsLayer )->addItem( pin );

  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::PinsLayer )->triggerRepaint();
  mPickResult.layer->triggerRepaint();
  delete pointItem;
}

void KadasCanvasContextMenu::convertCircleToPolygon()
{
  KadasMapItem *pickedItem = static_cast<KadasItemLayer *>( mPickResult.layer )->items()[mPickResult.itemId];
  KadasCircleItem *circleItem = static_cast<KadasCircleItem *>( pickedItem );
  if ( circleItem->constState()->centers.isEmpty() )
  {
    return;
  }
  bool ok = false;
  int num = QInputDialog::getInt( kApp->mainWindow(), tr( "Vertex Count" ), tr( "Number of polygon vertices:" ), 10, 3, 10000, 1, &ok );
  if ( !ok )
  {
    return;
  }
  KadasPolygonItem *polygonitem = new KadasPolygonItem( circleItem->crs() );
  KadasItemPos pos = circleItem->constState()->centers.front();
  double r = circleItem->constState()->radii.front();

  QgsLineString *ring = new QgsLineString();
  for ( int i = 0; i < num; ++i )
  {
    ring->addVertex( QgsPoint( pos.x() + r * qCos( ( 2. * i ) / num * M_PI ), pos.y() + r * qSin( ( 2. * i ) / num * M_PI ) ) );
  }
  ring->addVertex( QgsPoint( pos.x() + r, pos.y() ) );
  QgsPolygon poly;
  poly.setExteriorRing( ring );

  polygonitem->addPartFromGeometry( poly );
  polygonitem->setOutline( circleItem->outline() );
  polygonitem->setFill( circleItem->fill() );

  static_cast<KadasItemLayer *>( mPickResult.layer )->addItem( polygonitem );
  delete static_cast<KadasItemLayer *>( mPickResult.layer )->takeItem( mPickResult.itemId );
  mPickResult.layer->triggerRepaint();
}

void KadasCanvasContextMenu::convertPinToWaypoint()
{
  KadasMapItem *pickedItem = static_cast<KadasItemLayer *>( mPickResult.layer )->takeItem( mPickResult.itemId );
  KadasPinItem *pin = dynamic_cast<KadasPinItem *>( pickedItem );

  KadasGpxWaypointItem *waypoint = new KadasGpxWaypointItem();
  waypoint->setEditor( "KadasGpxWaypointEditor" );
  waypoint->setName( pin->name() );
  QgsCoordinateTransform crst( pin->crs(), waypoint->crs(), QgsProject::instance()->transformContext() );
  waypoint->setPosition( KadasItemPos::fromPoint( crst.transform( pin->position() ) ) );
  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::RoutesLayer )->addItem( waypoint );

  KadasItemLayerRegistry::getOrCreateItemLayer( KadasItemLayerRegistry::RoutesLayer )->triggerRepaint();
  mPickResult.layer->triggerRepaint();
  delete pin;
}

void KadasCanvasContextMenu::copyItemPosition()
{
  KadasMapItem *item = static_cast<KadasItemLayer *>( mPickResult.layer )->items()[mPickResult.itemId];
  QgsCoordinateTransform crst( item->crs(), mCanvas->mapSettings().destinationCrs(), QgsProject::instance() );
  copyCoordinates( crst.transform( item->position() ) );
}

void KadasCanvasContextMenu::copyCoordinates( const QgsPointXY &mapPos )
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

void KadasCanvasContextMenu::cutItem()
{
  KadasMapItem *item = static_cast<KadasItemLayer *>( mPickResult.layer )->takeItem( mPickResult.itemId );
  KadasClipboard::instance()->setStoredMapItems( QList<KadasMapItem *>() << item );
  mPickResult.layer->triggerRepaint();
}

void KadasCanvasContextMenu::copyFeature()
{
  QgsFeatureStore featureStore( static_cast<QgsVectorLayer *>( mPickResult.layer )->fields(), mPickResult.layer->crs() );
  featureStore.addFeature( mPickResult.feature );
  KadasClipboard::instance()->setStoredFeatures( featureStore );
}

void KadasCanvasContextMenu::copyItem()
{
  KadasMapItem *item = static_cast<KadasItemLayer *>( mPickResult.layer )->items()[mPickResult.itemId]->clone();
  KadasClipboard::instance()->setStoredMapItems( QList<KadasMapItem *>() << item );
}

void KadasCanvasContextMenu::copyMap()
{
  kApp->saveMapToClipboard();
}

void KadasCanvasContextMenu::deleteItem()
{
  delete static_cast<KadasItemLayer *>( mPickResult.layer )->takeItem( mPickResult.itemId );
  mPickResult.layer->triggerRepaint();
}

void KadasCanvasContextMenu::deleteItems()
{
  kApp->mainWindow()->actionDeleteItems()->trigger();
}

void KadasCanvasContextMenu::editItem()
{
  mCanvas->setMapTool( new KadasMapToolEditItem( mCanvas, mPickResult.itemId, static_cast<KadasItemLayer *>( mPickResult.layer ) ) );
}

void KadasCanvasContextMenu::paste()
{
  mCanvas->setMapTool( kApp->paste( &mMapPos ) );
}

void KadasCanvasContextMenu::drawPin()
{
  kApp->mainWindow()->actionPin()->trigger();
}

void KadasCanvasContextMenu::drawPointMarker()
{
  kApp->mainWindow()->redliningIntegration()->actionNewPoint()->trigger();
}

void KadasCanvasContextMenu::drawSquareMarker()
{
  kApp->mainWindow()->redliningIntegration()->actionNewSquare()->trigger();
}

void KadasCanvasContextMenu::drawTriangleMarker()
{
  kApp->mainWindow()->redliningIntegration()->actionNewTriangle()->trigger();
}

void KadasCanvasContextMenu::drawLine()
{
  kApp->mainWindow()->redliningIntegration()->actionNewLine()->trigger();
}

void KadasCanvasContextMenu::drawRectangle()
{
  kApp->mainWindow()->redliningIntegration()->actionNewRectangle()->trigger();
}

void KadasCanvasContextMenu::drawPolygon()
{
  kApp->mainWindow()->redliningIntegration()->actionNewPolygon()->trigger();
}

void KadasCanvasContextMenu::drawCircle()
{
  kApp->mainWindow()->redliningIntegration()->actionNewCircle()->trigger();
}

void KadasCanvasContextMenu::drawText()
{
  kApp->mainWindow()->redliningIntegration()->actionNewText()->trigger();
}

void KadasCanvasContextMenu::measureLine()
{
  kApp->mainWindow()->actionMeasureLine()->trigger();
  QgsMapTool *tool = kApp->mainWindow()->mapCanvas()->mapTool();
  if ( mPickResult.geom && dynamic_cast<KadasMapToolCreateItem *>( tool ) )
  {
    static_cast<KadasMapToolCreateItem *>( tool )->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
}

void KadasCanvasContextMenu::measurePolygon()
{
  kApp->mainWindow()->actionMeasureArea()->trigger();
  QgsMapTool *tool = kApp->mainWindow()->mapCanvas()->mapTool();
  if ( mPickResult.geom && dynamic_cast<KadasMapToolCreateItem *>( tool ) )
  {
    static_cast<KadasMapToolCreateItem *>( tool )->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
}

void KadasCanvasContextMenu::measureCircle()
{
  kApp->mainWindow()->actionMeasureCircle()->trigger();
  QgsMapTool *tool = kApp->mainWindow()->mapCanvas()->mapTool();
  if ( mPickResult.geom && dynamic_cast<KadasMapToolCreateItem *>( tool ) )
  {
    static_cast<KadasMapToolCreateItem *>( tool )->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
}

void KadasCanvasContextMenu::measureAzimuth()
{
  kApp->mainWindow()->actionMeasureAzimuth()->trigger();
  QgsMapTool *tool = kApp->mainWindow()->mapCanvas()->mapTool();
  if ( mPickResult.geom && dynamic_cast<KadasMapToolCreateItem *>( tool ) )
  {
    static_cast<KadasMapToolCreateItem *>( tool )->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
  }
}

void KadasCanvasContextMenu::measureHeightProfile()
{
  kApp->mainWindow()->actionMeasureHeightProfile()->trigger();
  QgsMapTool *tool = kApp->mainWindow()->mapCanvas()->mapTool();
  if ( mPickResult.geom && dynamic_cast<KadasMapToolCreateItem *>( tool ) )
  {
    static_cast<KadasMapToolCreateItem *>( tool )->addPartFromGeometry( *mPickResult.geom, mPickResult.crs );
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
}

void KadasCanvasContextMenu::print()
{
  QAction *printAction = kApp->mainWindow()->findChild<QAction *>( "mActionPrint" );
  if ( printAction && !printAction->isChecked() )
  {
    printAction->trigger();
  }
}
