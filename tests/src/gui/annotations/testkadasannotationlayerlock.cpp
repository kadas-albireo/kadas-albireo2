/***************************************************************************
    testkadasannotationlayerlock.cpp
    --------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <memory>

#include <QComboBox>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest/QTest>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsproject.h>

#include "kadas/core/kadaslayerutils.h"
#include "kadas/gui/annotationitems/kadasannotationcontrollerregistry.h"
#include "kadas/gui/annotationitems/kadasannotationlayerhelpers.h"
#include "kadas/gui/annotationitems/kadaslineannotationcontroller.h"
#include "kadas/gui/annotationitems/kadasmarkerannotationcontroller.h"
#include "kadas/gui/kadaslayerselectionwidget.h"
#include "kadas/gui/maptools/kadasmaptooleditannotationitem.h"

/**
 * Locked annotation layers are neither edited nor drawn into: the layer chooser
 * leaves them out, and the edit tool refuses them up front or lets go of them
 * as soon as they get locked.
 */
class TestKadasAnnotationLayerLock : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void chooserLeavesOutLockedLayers();
    void editingRefusesLockedLayer();
    void editingEndsWhenLayerGetsLocked();
    void drawingFallsBackFromLockedLayer();
    void drawingMovesOnWhenLayerGetsLocked();
    void drawingWaitsWhileEveryLayerIsLocked();

  private:
    //! Unsets the tool before deleting it, as the application does: only then does the tool tear down what it put on the canvas.
    struct ToolDeleter
    {
        void operator()( KadasMapToolEditAnnotationItem *tool ) const
        {
          tool->canvas()->unsetMapTool( tool );
          delete tool;
        }
    };
    using ToolPtr = std::unique_ptr<KadasMapToolEditAnnotationItem, ToolDeleter>;

    static QgsAnnotationLayer *addLayer( const QString &name );
    static QString addMarker( QgsAnnotationLayer *layer );
    static QgsAnnotationLayer *targetOf( const QSignalSpy &spy );

    std::unique_ptr<QgsMapCanvas> mCanvas;
};

void TestKadasAnnotationLayerLock::initTestCase()
{
  // Controllers persist the last used style in the settings: keep the developer's out of it.
  QStandardPaths::setTestModeEnabled( true );
  QgsApplication::init();
  QgsApplication::initQgis();
  qRegisterMetaType<QgsAnnotationLayer *>();
  // The edit tool looks up the controller of an existing item in the registry.
  KadasAnnotationControllerRegistry::instance()->addController( new KadasMarkerAnnotationController() );
}

void TestKadasAnnotationLayerLock::init()
{
  mCanvas = std::make_unique<QgsMapCanvas>();
  mCanvas->setDestinationCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  mCanvas->setExtent( QgsRectangle( -1000, -1000, 1000, 1000 ) );
}

void TestKadasAnnotationLayerLock::cleanup()
{
  mCanvas.reset();
  QgsProject::instance()->removeAllMapLayers();
}

QgsAnnotationLayer *TestKadasAnnotationLayerLock::addLayer( const QString &name )
{
  QgsAnnotationLayer *layer = KadasAnnotationLayerHelpers::createLayer( name, QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  QgsProject::instance()->addMapLayer( layer );
  return layer;
}

QString TestKadasAnnotationLayerLock::addMarker( QgsAnnotationLayer *layer )
{
  return layer->addItem( new QgsAnnotationMarkerItem( QgsPoint( 0, 0 ) ) );
}

QgsAnnotationLayer *TestKadasAnnotationLayerLock::targetOf( const QSignalSpy &spy )
{
  return spy.isEmpty() ? nullptr : qvariant_cast<QgsAnnotationLayer *>( spy.last().at( 0 ) );
}

void TestKadasAnnotationLayerLock::chooserLeavesOutLockedLayers()
{
  QgsAnnotationLayer *layerA = addLayer( QStringLiteral( "A" ) );
  QgsAnnotationLayer *layerB = addLayer( QStringLiteral( "B" ) );
  KadasLayerSelectionWidget chooser( mCanvas.get(), nullptr, []( QgsMapLayer *layer ) { return !KadasLayerUtils::isLocked( layer ); } );
  QComboBox *combo = chooser.findChild<QComboBox *>();
  QVERIFY( combo );
  QCOMPARE( combo->count(), 2 );

  chooser.setSelectedLayer( layerA );
  QCOMPARE( chooser.getSelectedLayer(), layerA );

  // Locking the selected layer takes it off the list and the selection along with it.
  KadasLayerUtils::setLocked( layerA, true );
  QCOMPARE( combo->count(), 1 );
  QCOMPARE( chooser.getSelectedLayer(), layerB );

  KadasLayerUtils::setLocked( layerA, false );
  QCOMPARE( combo->count(), 2 );
  QCOMPARE( chooser.getSelectedLayer(), layerB );
}

void TestKadasAnnotationLayerLock::editingRefusesLockedLayer()
{
  QgsAnnotationLayer *layer = addLayer( QStringLiteral( "A" ) );
  const QString itemId = addMarker( layer );
  KadasLayerUtils::setLocked( layer, true );

  ToolPtr tool( new KadasMapToolEditAnnotationItem( mCanvas.get(), layer, itemId ) );
  mCanvas->setMapTool( tool.get() );
  QVERIFY( mCanvas->mapTool() != tool.get() );
}

void TestKadasAnnotationLayerLock::editingEndsWhenLayerGetsLocked()
{
  QgsAnnotationLayer *layer = addLayer( QStringLiteral( "A" ) );
  const QString itemId = addMarker( layer );

  ToolPtr tool( new KadasMapToolEditAnnotationItem( mCanvas.get(), layer, itemId ) );
  mCanvas->setMapTool( tool.get() );
  QCOMPARE( mCanvas->mapTool(), tool.get() );

  QSignalSpy messages( tool.get(), &QgsMapTool::messageEmitted );
  KadasLayerUtils::setLocked( layer, true );
  QVERIFY( mCanvas->mapTool() != tool.get() );
  QCOMPARE( messages.count(), 1 );
  QCOMPARE( messages.at( 0 ).at( 0 ).toString(), KadasLayerUtils::lockedMessage( layer ) );
  QVERIFY( layer->item( itemId ) );
}

void TestKadasAnnotationLayerLock::drawingFallsBackFromLockedLayer()
{
  QgsAnnotationLayer *lockedLayer = addLayer( QStringLiteral( "A" ) );
  QgsAnnotationLayer *openLayer = addLayer( QStringLiteral( "B" ) );
  KadasLayerUtils::setLocked( lockedLayer, true );

  KadasLineAnnotationController controller;
  ToolPtr tool( new KadasMapToolEditAnnotationItem( mCanvas.get(), &controller, lockedLayer ) );
  QSignalSpy targets( tool.get(), &KadasMapToolEditAnnotationItem::targetLayerChanged );
  mCanvas->setMapTool( tool.get() );

  QCOMPARE( mCanvas->mapTool(), tool.get() );
  QCOMPARE( targetOf( targets ), openLayer );
  QVERIFY( lockedLayer->items().isEmpty() );

  tool->addPoint( QgsPointXY( 0, 0 ) );
  tool->addPoint( QgsPointXY( 100, 0 ) );
  QVERIFY( lockedLayer->items().isEmpty() );
  QCOMPARE( openLayer->items().size(), 1 );
}

void TestKadasAnnotationLayerLock::drawingMovesOnWhenLayerGetsLocked()
{
  QgsAnnotationLayer *layerA = addLayer( QStringLiteral( "A" ) );
  QgsAnnotationLayer *layerB = addLayer( QStringLiteral( "B" ) );

  KadasLineAnnotationController controller;
  ToolPtr tool( new KadasMapToolEditAnnotationItem( mCanvas.get(), &controller, layerA ) );
  mCanvas->setMapTool( tool.get() );
  QSignalSpy targets( tool.get(), &KadasMapToolEditAnnotationItem::targetLayerChanged );

  // A line half drawn into A...
  tool->addPoint( QgsPointXY( 0, 0 ) );
  tool->addPoint( QgsPointXY( 100, 0 ) );
  QCOMPARE( layerA->items().size(), 1 );

  // ...is not left behind on it once A is locked, and the next one goes into B.
  KadasLayerUtils::setLocked( layerA, true );
  QVERIFY( layerA->items().isEmpty() );
  QCOMPARE( mCanvas->mapTool(), tool.get() );
  QCOMPARE( targetOf( targets ), layerB );
  QCOMPARE( layerB->items().size(), 1 );
}

void TestKadasAnnotationLayerLock::drawingWaitsWhileEveryLayerIsLocked()
{
  QgsAnnotationLayer *lockedLayer = addLayer( QStringLiteral( "A" ) );
  KadasLayerUtils::setLocked( lockedLayer, true );

  KadasLineAnnotationController controller;
  ToolPtr tool( new KadasMapToolEditAnnotationItem( mCanvas.get(), &controller, lockedLayer ) );
  QSignalSpy targets( tool.get(), &KadasMapToolEditAnnotationItem::targetLayerChanged );
  mCanvas->setMapTool( tool.get() );
  QCOMPARE( mCanvas->mapTool(), tool.get() );

  tool->addPoint( QgsPointXY( 0, 0 ) );
  tool->addPoint( QgsPointXY( 100, 0 ) );
  QVERIFY( lockedLayer->items().isEmpty() );

  // A new layer, created from the chooser or anywhere else, is drawn into at once.
  QgsAnnotationLayer *newLayer = addLayer( QStringLiteral( "B" ) );
  QCOMPARE( targetOf( targets ), newLayer );
  tool->addPoint( QgsPointXY( 0, 0 ) );
  tool->addPoint( QgsPointXY( 100, 0 ) );
  QCOMPARE( newLayer->items().size(), 1 );
  QVERIFY( lockedLayer->items().isEmpty() );
}

QTEST_MAIN( TestKadasAnnotationLayerLock )
#include "testkadasannotationlayerlock.moc"
