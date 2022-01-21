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
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadascoordinateformat.h>
#include <kadas/gui/kadasclipboard.h>
#include <kadas/gui/kadasitemcontextmenuactions.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadasmapcanvasitemmanager.h>
#include <kadas/gui/mapitems/kadasselectionrectitem.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>
#include <kadas/gui/maptools/kadasmaptooledititem.h>
#include <kadas/gui/maptools/kadasmaptoolhillshade.h>
#include <kadas/gui/maptools/kadasmaptoolslope.h>
#include <kadas/app/kadasapplication.h>
#include <kadas/app/kadascanvascontextmenu.h>
#include <kadas/app/kadasmainwindow.h>
#include <kadas/app/kadasmapidentifydialog.h>
#include <kadas/app/kadasredliningintegration.h>

KadasCanvasContextMenu::KadasCanvasContextMenu( QgsMapCanvas *canvas, const QgsPointXY &mapPos )
  : mMapPos( mapPos ), mCanvas( canvas )
{
  mPickResult = KadasFeaturePicker::pick( mCanvas, mapPos );
  KadasMapItem *pickedItem = mPickResult.itemId != KadasItemLayer::ITEM_ID_NULL ? static_cast<KadasItemLayer *>( mPickResult.layer )->items()[mPickResult.itemId] : nullptr;
  QgsWkbTypes::GeometryType geomType = QgsWkbTypes::UnknownGeometry;
  if ( mPickResult.geom )
  {
    geomType = QgsWkbTypes::geometryType( mPickResult.geom->wkbType() );
  }

  addAction( QgsApplication::getThemeIcon( "/mActionIdentify.svg" ), tr( "Identify" ), this, SLOT( identify() ) );
  addSeparator();

  if ( pickedItem )
  {
    addAction( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ), tr( "Edit" ), this, &KadasCanvasContextMenu::editItem );
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
  }
  addSeparator();
  if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::LineGeometry || geomType == QgsWkbTypes::PolygonGeometry )
  {
    QMenu *measureMenu = new QMenu();
    addAction( tr( "Measure" ) )->setMenu( measureMenu );

    if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::LineGeometry )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_line" ), tr( "Distance / Azimuth" ), this, &KadasCanvasContextMenu::measureLine );
    }
    if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::PolygonGeometry )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_area" ), tr( "Area" ), this, &KadasCanvasContextMenu::measurePolygon );
    }
    if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::PolygonGeometry )
    {
      measureMenu->addAction( QIcon( ":/kadas/icons/measure_circle" ), tr( "Circle" ), this, SLOT( measureCircle() ) );
    }
    if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::LineGeometry )
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
    if ( mPickResult.isEmpty() || geomType == QgsWkbTypes::LineGeometry )
    {
      analysisMenu->addAction( QIcon( ":/kadas/icons/measure_height_profile" ), tr( "Line of sight" ), this, &KadasCanvasContextMenu::measureHeightProfile );
    }
  }

  if ( mPickResult.isEmpty() )
  {
    addAction( QIcon( ":/kadas/icons/copy_coordinates" ), tr( "Copy coordinates" ), this, &KadasCanvasContextMenu::copyCoordinates );
    addAction( QIcon( ":/kadas/icons/copy_map" ), tr( "Copy map" ), this, &KadasCanvasContextMenu::copyMap );
    addAction( QgsApplication::getThemeIcon( "/mActionFilePrint.svg" ), tr( "Print" ), this, &KadasCanvasContextMenu::print );
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

void KadasCanvasContextMenu::drawCoordinateCross()
{
  kApp->mainWindow()->redliningIntegration()->actionNewCoordinateCross()->trigger();
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
