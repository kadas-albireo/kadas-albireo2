/***************************************************************************
    testkadasannotationshadows.cpp
    ------------------------------
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

#include <QStandardPaths>
#include <QtTest/QTest>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscurvepolygon.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsrectangle.h>

#include <kadas/gui/annotationitems/kadasannotationcontrollerregistry.h>
#include <kadas/gui/annotationitems/kadasannotationitemcontext.h>
#include <kadas/gui/annotationitems/kadasannotationitemcontrollers.h>
#include <kadas/gui/annotationitems/kadasannotationlayerhelpers.h>
#include <kadas/gui/annotationitems/kadasannotationlayerregistry.h>
#include <kadas/gui/annotationitems/kadasannotationshadow.h>
#include <kadas/gui/annotationitems/kadascircleannotationcontroller.h>
#include <kadas/gui/annotationitems/kadascircleannotationitem.h>
#include <kadas/gui/annotationitems/kadascoordcrossannotationcontroller.h>
#include <kadas/gui/annotationitems/kadascoordcrossannotationitem.h>
#include <kadas/gui/annotationitems/kadaspinannotationcontroller.h>
#include <kadas/gui/annotationitems/kadaspinannotationitem.h>
#include <kadas/gui/annotationitems/kadasrectangleannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasrectangleannotationitem.h>


/**
 * Tests for the save-time \"shadow\" mechanism: each Kadas-specific
 * annotation type emits parallel stock-QGIS items at save time so that
 * vanilla QGIS can render the project, while the in-memory Kadas session
 * always sees only the master items.
 */
class TestKadasAnnotationShadows : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();

    // generateShadows() per type ----------------------------------------
    void rectangle_generateShadows_emitsPolygon();
    void circle_generateShadows_emitsPolygon();
    void pin_generateShadows_emitsMarker();
    void coordcross_generateShadows_emitsCrossAndLabel();

    // Layer-level prepare/strip lifecycle -------------------------------
    void prepareLayerForSave_addsShadowsAndStoresIds();
    void stripShadowsFromLayer_removesShadowsAndClearsIds();
    void prepareIsIdempotent();

    // Orphan reconstruction after a vanilla-QGIS round trip --------------
    void reconstructOrphanCrosses_rebuildsDroppedMaster();
    void reconstructOrphanCrosses_recoversMovedPosition();
    void reconstructOrphanCrosses_keepsMasterWhenPresent();

  private:
    static KadasAnnotationItemContext makeContext();
};


void TestKadasAnnotationShadows::initTestCase()
{
  QStandardPaths::setTestModeEnabled( true );
  QgsApplication::init();
  // The layer-level tests need the controller registry populated so that
  // KadasAnnotationLayerHelpers::prepareLayerForSave() can resolve a
  // controller for each master item type.
  KadasAnnotationItemControllers::registerBuiltins();
}

KadasAnnotationItemContext TestKadasAnnotationShadows::makeContext()
{
  const QgsCoordinateReferenceSystem crs( QStringLiteral( "EPSG:3857" ) );
  QgsMapSettings ms;
  ms.setDestinationCrs( crs );
  ms.setExtent( QgsRectangle( -1000, -1000, 1000, 1000 ) );
  ms.setOutputSize( QSize( 1000, 1000 ) );
  ms.setOutputDpi( 96 );
  static QgsAnnotationLayer sLayer( QStringLiteral( "test" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  sLayer.setCrs( crs );
  return KadasAnnotationItemContext( &sLayer, ms );
}


// ----------------------------- generateShadows -----------------------------

void TestKadasAnnotationShadows::rectangle_generateShadows_emitsPolygon()
{
  KadasRectangleAnnotationItem master;
  master.setBox( QgsPointXY( 0, 0 ), QSizeF( 100, 50 ), 0.0 );

  KadasRectangleAnnotationController controller;
  const auto shadows = controller.generateShadows( &master, makeContext() );
  QCOMPARE( shadows.size(), 1 );

  std::unique_ptr<QgsAnnotationItem> shadow( shadows.first() );
  auto *poly = dynamic_cast<QgsAnnotationPolygonItem *>( shadow.get() );
  QVERIFY( poly );
  QVERIFY( poly->geometry() );
  QCOMPARE( poly->geometry()->ringCount(), master.geometry()->ringCount() );
}

void TestKadasAnnotationShadows::circle_generateShadows_emitsPolygon()
{
  KadasCircleAnnotationItem master;
  master.setCenter( QgsPointXY( 0, 0 ) );
  master.setRingPoint( QgsPointXY( 100, 0 ) );

  KadasCircleAnnotationController controller;
  const auto shadows = controller.generateShadows( &master, makeContext() );
  QCOMPARE( shadows.size(), 1 );

  std::unique_ptr<QgsAnnotationItem> shadow( shadows.first() );
  auto *poly = dynamic_cast<QgsAnnotationPolygonItem *>( shadow.get() );
  QVERIFY( poly );
  QVERIFY( poly->geometry() );
}

void TestKadasAnnotationShadows::pin_generateShadows_emitsMarker()
{
  KadasPinAnnotationItem master;
  master.setGeometry( QgsPoint( 12.0, 34.0 ) );

  KadasPinAnnotationController controller;
  const auto shadows = controller.generateShadows( &master, makeContext() );
  QCOMPARE( shadows.size(), 1 );

  std::unique_ptr<QgsAnnotationItem> shadow( shadows.first() );
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( shadow.get() );
  QVERIFY( marker );
  QCOMPARE( marker->geometry().x(), 12.0 );
  QCOMPARE( marker->geometry().y(), 34.0 );
}

void TestKadasAnnotationShadows::coordcross_generateShadows_emitsCrossAndLabel()
{
  KadasCoordCrossAnnotationItem master;
  master.setGeometry( QgsPoint( 1000, 2000 ) );

  KadasCoordCrossAnnotationController controller;
  const auto shadows = controller.generateShadows( &master, makeContext() );
  // One cross-shape marker + one coordinate label.
  QCOMPARE( shadows.size(), 2 );

  std::unique_ptr<QgsAnnotationItem> a( shadows.at( 0 ) );
  std::unique_ptr<QgsAnnotationItem> b( shadows.at( 1 ) );

  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( a.get() );
  auto *label = dynamic_cast<QgsAnnotationPointTextItem *>( b.get() );
  QVERIFY( marker );
  QVERIFY( label );
  QCOMPARE( marker->geometry().x(), 1000.0 );
  QCOMPARE( marker->geometry().y(), 2000.0 );
}


// --------------------------- layer-level lifecycle --------------------------

void TestKadasAnnotationShadows::prepareLayerForSave_addsShadowsAndStoresIds()
{
  QgsAnnotationLayer::LayerOptions opts { QgsCoordinateTransformContext() };
  QgsAnnotationLayer layer( QStringLiteral( "test" ), opts );

  auto *master = new KadasRectangleAnnotationItem();
  master->setBox( QgsPointXY( 0, 0 ), QSizeF( 10, 10 ), 0.0 );
  const QString masterId = layer.addItem( master );

  QCOMPARE( layer.items().size(), 1 );

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );

  // Original master + 1 shadow.
  QCOMPARE( layer.items().size(), 2 );
  auto *m = dynamic_cast<KadasRectangleAnnotationItem *>( layer.item( masterId ) );
  QVERIFY( m );
  QCOMPARE( m->shadowIds().size(), 1 );
  // Each stored id must resolve to a non-Kadas stock item in the layer.
  for ( const QString &sid : m->shadowIds() )
  {
    QgsAnnotationItem *si = layer.item( sid );
    QVERIFY( si );
    QVERIFY( !dynamic_cast<KadasRectangleAnnotationItem *>( si ) );
  }
}

void TestKadasAnnotationShadows::stripShadowsFromLayer_removesShadowsAndClearsIds()
{
  QgsAnnotationLayer::LayerOptions opts { QgsCoordinateTransformContext() };
  QgsAnnotationLayer layer( QStringLiteral( "test" ), opts );

  auto *master = new KadasPinAnnotationItem();
  master->setGeometry( QgsPoint( 5, 5 ) );
  const QString masterId = layer.addItem( master );

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );
  QVERIFY( layer.items().size() > 1 );

  KadasAnnotationLayerHelpers::stripShadowsFromLayer( &layer );

  QCOMPARE( layer.items().size(), 1 );
  auto *m = dynamic_cast<KadasPinAnnotationItem *>( layer.item( masterId ) );
  QVERIFY( m );
  QVERIFY( m->shadowIds().isEmpty() );
}

void TestKadasAnnotationShadows::prepareIsIdempotent()
{
  // Calling prepare twice must not accumulate shadows: the second call
  // strips the first round of shadows before regenerating.
  QgsAnnotationLayer::LayerOptions opts { QgsCoordinateTransformContext() };
  QgsAnnotationLayer layer( QStringLiteral( "test" ), opts );

  auto *master = new KadasCircleAnnotationItem();
  master->setCenter( QgsPointXY( 0, 0 ) );
  master->setRingPoint( QgsPointXY( 50, 0 ) );
  layer.addItem( master );

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );
  const int after1 = layer.items().size();

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );
  const int after2 = layer.items().size();

  QCOMPARE( after1, after2 );
}


// ------------------------ orphan reconstruction ------------------------

void TestKadasAnnotationShadows::reconstructOrphanCrosses_rebuildsDroppedMaster()
{
  QgsAnnotationLayer::LayerOptions opts { QgsCoordinateTransformContext() };
  QgsAnnotationLayer layer( QStringLiteral( "test" ), opts );
  layer.setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );

  auto *master = new KadasCoordCrossAnnotationItem();
  master->setGeometry( QgsPoint( 1000, 2000 ) );
  const QString masterId = layer.addItem( master );

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );
  // master + cross marker + label shadow
  QCOMPARE( layer.items().size(), 3 );

  // Simulate a vanilla-QGIS round trip: the unknown master type is dropped,
  // the stock shadows and the layer custom properties survive.
  layer.removeItem( masterId );
  QCOMPARE( layer.items().size(), 2 );

  KadasAnnotationLayerHelpers::reconstructOrphanCrosses( &layer );

  // Shadows replaced by exactly one reconstructed coordinate cross.
  QCOMPARE( layer.items().size(), 1 );
  auto *cross = dynamic_cast<KadasCoordCrossAnnotationItem *>( layer.items().first() );
  QVERIFY( cross );
  QCOMPARE( cross->geometry().x(), 1000.0 );
  QCOMPARE( cross->geometry().y(), 2000.0 );

  // Orphan side-channel consumed.
  for ( const QString &key : layer.customPropertyKeys() )
    QVERIFY( !key.startsWith( QLatin1String( "kadas:orphan:" ) ) );
}

void TestKadasAnnotationShadows::reconstructOrphanCrosses_recoversMovedPosition()
{
  QgsAnnotationLayer::LayerOptions opts { QgsCoordinateTransformContext() };
  QgsAnnotationLayer layer( QStringLiteral( "test" ), opts );
  layer.setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );

  auto *master = new KadasCoordCrossAnnotationItem();
  master->setGeometry( QgsPoint( 1000, 2000 ) );
  const QString masterId = layer.addItem( master );

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );
  layer.removeItem( masterId );

  // The user dragged the cross in vanilla QGIS: move the stock marker shadow.
  for ( QgsAnnotationItem *item : layer.items() )
  {
    if ( auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item ) )
      marker->setGeometry( QgsPoint( 5000, 6000 ) );
  }

  KadasAnnotationLayerHelpers::reconstructOrphanCrosses( &layer );

  QCOMPARE( layer.items().size(), 1 );
  auto *cross = dynamic_cast<KadasCoordCrossAnnotationItem *>( layer.items().first() );
  QVERIFY( cross );
  // Reconstructed at the moved marker position, not the original.
  QCOMPARE( cross->geometry().x(), 5000.0 );
  QCOMPARE( cross->geometry().y(), 6000.0 );
}

void TestKadasAnnotationShadows::reconstructOrphanCrosses_keepsMasterWhenPresent()
{
  // Project opened straight in Kadas: the master deserializes fine, so
  // reconstruction must not add a duplicate. It only consumes the record.
  QgsAnnotationLayer::LayerOptions opts { QgsCoordinateTransformContext() };
  QgsAnnotationLayer layer( QStringLiteral( "test" ), opts );
  layer.setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );

  auto *master = new KadasCoordCrossAnnotationItem();
  master->setGeometry( QgsPoint( 1000, 2000 ) );
  const QString masterId = layer.addItem( master );

  KadasAnnotationLayerHelpers::prepareLayerForSave( &layer );
  const int withShadows = layer.items().size();

  KadasAnnotationLayerHelpers::reconstructOrphanCrosses( &layer );

  // No new master added; the original is untouched.
  QCOMPARE( layer.items().size(), withShadows );
  QVERIFY( dynamic_cast<KadasCoordCrossAnnotationItem *>( layer.item( masterId ) ) );
  for ( const QString &key : layer.customPropertyKeys() )
    QVERIFY( !key.startsWith( QLatin1String( "kadas:orphan:" ) ) );
}


QTEST_MAIN( TestKadasAnnotationShadows )
#include "testkadasannotationshadows.moc"
