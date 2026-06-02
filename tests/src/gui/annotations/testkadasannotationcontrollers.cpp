/***************************************************************************
    testkadasannotationcontrollers.cpp
    ----------------------------------
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

#include <QFile>
#include <QStandardPaths>
#include <QtTest/QTest>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsrectangle.h>

#include <kadas/gui/kadasattributetypes.h>
#include <kadas/gui/annotationitems/kadasannotationitemcontext.h>
#include <kadas/gui/annotationitems/kadascircleannotationcontroller.h>
#include <kadas/gui/annotationitems/kadascircleannotationitem.h>
#include <kadas/gui/annotationitems/kadaslineannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasmarkerannotationcontroller.h>
#include <kadas/gui/annotationitems/kadaspinannotationitem.h>
#include <kadas/gui/annotationitems/kadaspolygonannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasrectangleannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasrectangleannotationitem.h>


/**
 * Controller-level unit tests for the unified annotation pipeline.
 *
 * These do not exercise the map tool — they hit the controllers directly
 * with a synthetic KadasAnnotationItemContext so the tests are fast and
 * have no dependency on a live QgsMapCanvas or layer.
 */
class TestKadasAnnotationControllers : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();

    // KadasMarkerAnnotationController ------------------------------------
    void marker_createItem_hasVisibleSymbol();
    void marker_applyPersistedStyle_preservesShape();
    void marker_applyPersistedStyle_rejectsTransparentFill();
    void marker_getEditContext_hitsWithinTolerance();

    // KadasRectangleAnnotationController ---------------------------------
    void rectangle_nodes_returnFourCornersPlusRotation();
    void rectangle_getEditContext_rotatedQuadHitTest();
    void rectangle_edit_movesCorrectCorner();

    // KadasCircleAnnotationController ------------------------------------
    void circle_nodes_returnsCenterAndRing();
    void circle_edit_centerAndRingRoundtrip();

    // KadasPinAnnotationItem ---------------------------------------------
    void pin_defaultIconPath_resolvesInQrc();

    // KadasLineAnnotationController --------------------------------------
    void line_getEditContext_hitsOnSegmentNotInBoundingBox();
    void line_getEditContext_hitsVertex();

    // KadasPolygonAnnotationController -----------------------------------
    void polygon_getEditContext_hitsBodyNotBoundingBox();

    // Multi-type hit isolation -------------------------------------------
    void hitTest_multipleTypesSelectsByGeometryNotBbox();

  private:
    static KadasAnnotationItemContext makeContext();
};


void TestKadasAnnotationControllers::initTestCase()
{
  // Isolate QSettings used by KadasMarkerAnnotationController persisted
  // entries so we never clobber the developer's settings.
  QStandardPaths::setTestModeEnabled( true );
  QgsApplication::init();
}

KadasAnnotationItemContext TestKadasAnnotationControllers::makeContext()
{
  // Use a CRS where map units are meters and 1 unit ≈ 1 unit so the test
  // distances stay readable (EPSG:3857). Item CRS == map CRS so toMapPos /
  // toItemPos are identities.
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


// ----- Marker -----------------------------------------------------------

void TestKadasAnnotationControllers::marker_createItem_hasVisibleSymbol()
{
  KadasMarkerAnnotationController controller;
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  QVERIFY( item );
  auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item.get() );
  QVERIFY( marker );
  QVERIFY( marker->symbol() );
  // Regression: an empty QgsMarkerSymbol() has zero layers and renders
  // nothing, leaving the user with only the vertex handle visible.
  QVERIFY( marker->symbol()->symbolLayerCount() > 0 );
  auto *sl = dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( marker->symbol()->symbolLayer( 0 ) );
  QVERIFY( sl );
  QVERIFY( sl->size() > 0 );
  QVERIFY( sl->color().alpha() > 0 );
}

void TestKadasAnnotationControllers::marker_applyPersistedStyle_preservesShape()
{
  KadasMarkerAnnotationController controller;
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *marker = static_cast<QgsAnnotationMarkerItem *>( item.get() );

  // Pretend the toolbar handed us a Triangle.
  auto *sl = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Triangle );
  sl->setSize( 4 );
  sl->setColor( QColor( 0, 200, 0 ) );
  marker->setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << sl ) );

  // Persist, then re-apply: shape must survive (toolbar wins, not settings).
  controller.persistStyle( marker );
  controller.applyPersistedStyle( marker );

  auto *sl2 = dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( marker->symbol()->symbolLayer( 0 ) );
  QVERIFY( sl2 );
  QCOMPARE( sl2->shape(), Qgis::MarkerShape::Triangle );
}

void TestKadasAnnotationControllers::marker_applyPersistedStyle_rejectsTransparentFill()
{
  KadasMarkerAnnotationController controller;

  // Persist a fully transparent fill, simulating the bug where the user
  // accidentally picked alpha=0 in the inline color editor.
  KadasMarkerAnnotationController::settingsFillColor->setValue( QColor( 255, 0, 0, 0 ) );
  KadasMarkerAnnotationController::settingsStrokeColor->setValue( QColor( 0, 0, 0 ) );
  KadasMarkerAnnotationController::settingsSize->setValue( 4 );
  KadasMarkerAnnotationController::settingsStrokeWidth->setValue( 0.4 );
  KadasMarkerAnnotationController::settingsStrokeStyle->setValue( static_cast<int>( Qt::SolidLine ) );

  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *marker = static_cast<QgsAnnotationMarkerItem *>( item.get() );
  controller.applyPersistedStyle( marker );

  auto *sl = dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( marker->symbol()->symbolLayer( 0 ) );
  QVERIFY( sl );
  QVERIFY2( sl->color().alpha() > 0, "transparent persisted fill must be rejected" );
}

void TestKadasAnnotationControllers::marker_getEditContext_hitsWithinTolerance()
{
  KadasMarkerAnnotationController controller;
  const auto ctx = makeContext();

  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 100, 200 ), ctx );

  // Right on the marker point: hit.
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 100, 200 ), ctx );
  QVERIFY( ec.isValid() );

  // Far away: miss.
  ec = controller.getEditContext( item.get(), QgsPointXY( 500, 500 ), ctx );
  QVERIFY( !ec.isValid() );
}


// ----- Rectangle --------------------------------------------------------

void TestKadasAnnotationControllers::rectangle_nodes_returnFourCornersPlusRotation()
{
  KadasRectangleAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *rect = static_cast<KadasRectangleAnnotationItem *>( item.get() );
  rect->setBox( QgsPointXY( 0, 0 ), QSizeF( 100, 50 ), 0.0 );

  const auto nodes = controller.nodes( item.get(), ctx );
  QCOMPARE( nodes.size(), 5 ); // 4 corners + 1 rotation handle
}

void TestKadasAnnotationControllers::rectangle_getEditContext_rotatedQuadHitTest()
{
  KadasRectangleAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *rect = static_cast<KadasRectangleAnnotationItem *>( item.get() );

  // 200x100 rectangle centered at origin, rotated 45°.
  rect->setBox( QgsPointXY( 0, 0 ), QSizeF( 200, 100 ), 45.0 );

  // Origin is inside the rotated quad.
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 0, 0 ), ctx );
  QVERIFY2( ec.isValid(), "click at center of rotated rectangle should hit body" );

  // Top-right of the AABB but outside the rotated quad: must miss.
  // AABB extends roughly to ~106 along each axis; (95, 95) lies within
  // the AABB but outside the rotated body.
  ec = controller.getEditContext( item.get(), QgsPointXY( 95, 95 ), ctx );
  QVERIFY2( !ec.isValid(), "click in AABB corner outside rotated body must miss" );
}

void TestKadasAnnotationControllers::rectangle_edit_movesCorrectCorner()
{
  KadasRectangleAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *rect = static_cast<KadasRectangleAnnotationItem *>( item.get() );
  rect->setBox( QgsPointXY( 0, 0 ), QSizeF( 100, 100 ), 0.0 );

  // Pick the BR corner (vertex 1) and drag it to (200, -50).
  const auto cornersBefore = rect->corners();
  KadasEditContext ec( QgsVertexId( 0, 0, 1 ), cornersBefore[1] );
  controller.edit( item.get(), ec, QgsPointXY( 200, -50 ), ctx );

  // After the drag the rectangle's center / size must reflect the new BR.
  // BR was (50, -50) -> now (200, -50). TL stays at (-50, 50). New box:
  // center = (75, 0), size = (250, 100).
  QCOMPARE( rect->size().width(), 250.0 );
  QCOMPARE( rect->size().height(), 100.0 );
  QCOMPARE( rect->center().x(), 75.0 );
  QCOMPARE( rect->center().y(), 0.0 );
}


// ----- Circle -----------------------------------------------------------

void TestKadasAnnotationControllers::circle_nodes_returnsCenterAndRing()
{
  KadasCircleAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *circle = static_cast<KadasCircleAnnotationItem *>( item.get() );
  circle->setCenter( QgsPointXY( 10, 20 ) );
  circle->setRingPoint( QgsPointXY( 30, 20 ) );

  const auto nodes = controller.nodes( item.get(), ctx );
  QCOMPARE( nodes.size(), 2 ); // center + ring point
  QCOMPARE( nodes[0].pos.x(), 10.0 );
  QCOMPARE( nodes[1].pos.x(), 30.0 );
}

void TestKadasAnnotationControllers::circle_edit_centerAndRingRoundtrip()
{
  KadasCircleAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *circle = static_cast<KadasCircleAnnotationItem *>( item.get() );
  circle->setCenter( QgsPointXY( 0, 0 ) );
  circle->setRingPoint( QgsPointXY( 50, 0 ) );

  // Drag the ring vertex (vid 1) outward to (100, 0): radius doubles.
  KadasEditContext ec( QgsVertexId( 0, 0, 1 ), QgsPointXY( 50, 0 ) );
  controller.edit( item.get(), ec, QgsPointXY( 100, 0 ), ctx );
  QCOMPARE( circle->ringPoint().x(), 100.0 );
  QCOMPARE( circle->center().x(), 0.0 );
}


// ----- Pin --------------------------------------------------------------

void TestKadasAnnotationControllers::pin_defaultIconPath_resolvesInQrc()
{
  // Regression: the pin icon must resolve through Qt's resource system,
  // otherwise QgsSvgMarkerSymbolLayer falls back to a "?" placeholder.
  // The qrc itself is compiled into the kadas app target (not kadas_gui),
  // so this test verifies the contract by checking that:
  //   1. defaultIconPath() returns a Qt resource path (":/...")
  //   2. the corresponding on-disk SVG exists in kadas/resources/icons/.
  const QString path = KadasPinAnnotationItem::defaultIconPath();
  QVERIFY2( !path.isEmpty(), "defaultIconPath must not be empty" );
  QVERIFY2( path.startsWith( QLatin1String( ":/kadas/icons/" ) ), qPrintable( QStringLiteral( "expected ':/kadas/icons/...' got %1" ).arg( path ) ) );
  const QString relative = path.mid( QStringLiteral( ":/kadas/" ).size() );
  const QString diskPath = QStringLiteral( "%1/kadas/resources/%2.svg" ).arg( CMAKE_SOURCE_DIR, relative );
  QVERIFY2( QFile::exists( diskPath ), qPrintable( QStringLiteral( "pin SVG file missing: %1" ).arg( diskPath ) ) );
}


// ----- Line -------------------------------------------------------------

namespace
{
  // Build a QgsAnnotationLineItem with the given polyline (in item CRS,
  // which equals map CRS in makeContext()).
  std::unique_ptr<QgsAnnotationLineItem> makeLine( const QVector<QgsPointXY> &pts )
  {
    auto *ls = new QgsLineString();
    for ( const QgsPointXY &p : pts )
      ls->addVertex( QgsPoint( p.x(), p.y() ) );
    return std::make_unique<QgsAnnotationLineItem>( ls );
  }

  std::unique_ptr<QgsAnnotationPolygonItem> makePolygon( const QVector<QgsPointXY> &ringPts )
  {
    auto *ring = new QgsLineString();
    for ( const QgsPointXY &p : ringPts )
      ring->addVertex( QgsPoint( p.x(), p.y() ) );
    auto *poly = new QgsPolygon();
    poly->setExteriorRing( ring );
    return std::make_unique<QgsAnnotationPolygonItem>( poly );
  }
} //namespace

void TestKadasAnnotationControllers::line_getEditContext_hitsOnSegmentNotInBoundingBox()
{
  // Regression: a long diagonal line's bounding box covers vast empty
  // space. Selection must use distance-to-segment, not bbox containment,
  // otherwise any click in the bbox falsely picks the line.
  KadasLineAnnotationController controller;
  const auto ctx = makeContext();

  // Diagonal from (0, 0) to (1000, 1000).
  auto item = makeLine( { QgsPointXY( 0, 0 ), QgsPointXY( 1000, 1000 ) } );

  // Click ON the segment, near its midpoint: hit.
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 500, 500 ), ctx );
  QVERIFY2( ec.isValid(), "click on segment must hit" );

  // Click in the bbox but far from the diagonal (top-left corner of
  // bbox, no segment passes nearby): must miss.
  ec = controller.getEditContext( item.get(), QgsPointXY( 50, 950 ), ctx );
  QVERIFY2( !ec.isValid(), "click in bbox far from segment must miss" );

  // Sanity: click well outside bbox: miss.
  ec = controller.getEditContext( item.get(), QgsPointXY( -500, -500 ), ctx );
  QVERIFY( !ec.isValid() );
}

void TestKadasAnnotationControllers::line_getEditContext_hitsVertex()
{
  KadasLineAnnotationController controller;
  const auto ctx = makeContext();
  auto item = makeLine( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ) } );

  // On a vertex: hit (as a vertex edit, not body move).
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 100, 0 ), ctx );
  QVERIFY( ec.isValid() );
  QVERIFY( ec.vidx.isValid() );
}


// ----- Polygon ----------------------------------------------------------

void TestKadasAnnotationControllers::polygon_getEditContext_hitsBodyNotBoundingBox()
{
  // Regression: a U-shaped polygon's bounding box includes the empty
  // area between its arms. The controller must use real geometry
  // containment, not bbox containment.
  KadasPolygonAnnotationController controller;
  const auto ctx = makeContext();

  // U shape (open at the top): outer ring traces a thick "U".
  //   *--*    *--*
  //   |  |    |  |
  //   |  *----*  |
  //   |          |
  //   *----------*
  auto item = makePolygon( {
    QgsPointXY( 0, 0 ),
    QgsPointXY( 100, 0 ),
    QgsPointXY( 100, 80 ),
    QgsPointXY( 70, 80 ),
    QgsPointXY( 70, 30 ),
    QgsPointXY( 30, 30 ),
    QgsPointXY( 30, 80 ),
    QgsPointXY( 0, 80 ),
    QgsPointXY( 0, 0 ),
  } );

  // Click in the gap between arms (50, 60): inside bbox, NOT inside U.
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 50, 60 ), ctx );
  QVERIFY2( !ec.isValid(), "click in U's empty gap must miss" );

  // Click in solid body of left arm: hit.
  ec = controller.getEditContext( item.get(), QgsPointXY( 15, 50 ), ctx );
  QVERIFY2( ec.isValid(), "click inside left arm of U must hit body" );

  // Click in solid base of U: hit.
  ec = controller.getEditContext( item.get(), QgsPointXY( 50, 15 ), ctx );
  QVERIFY( ec.isValid() );
}


// ----- Multi-type isolation --------------------------------------------

void TestKadasAnnotationControllers::hitTest_multipleTypesSelectsByGeometryNotBbox()
{
  // Stage: a long diagonal line whose bbox covers a marker placed in
  // the empty corner. The picker (via getEditContext) must only hit the
  // line when the click is actually near a segment, so a click on the
  // marker's point selects the marker — not the line.
  KadasLineAnnotationController lineCtrl;
  KadasMarkerAnnotationController markerCtrl;
  KadasPolygonAnnotationController polyCtrl;
  const auto ctx = makeContext();

  auto line = makeLine( { QgsPointXY( 0, 0 ), QgsPointXY( 800, 800 ) } );

  std::unique_ptr<QgsAnnotationItem> markerItem( markerCtrl.createItem() );
  markerCtrl.startPart( markerItem.get(), QgsPointXY( 50, 700 ), ctx );

  // U-shaped polygon with a gap centered around (500, 60).
  auto poly = makePolygon( {
    QgsPointXY( 400, 0 ),
    QgsPointXY( 600, 0 ),
    QgsPointXY( 600, 100 ),
    QgsPointXY( 540, 100 ),
    QgsPointXY( 540, 30 ),
    QgsPointXY( 460, 30 ),
    QgsPointXY( 460, 100 ),
    QgsPointXY( 400, 100 ),
    QgsPointXY( 400, 0 ),
  } );

  // 1) Click on the marker (in the line's bbox, far from the diagonal):
  //    only marker hits.
  const QgsPointXY pMarker( 50, 700 );
  QVERIFY2( markerCtrl.getEditContext( markerItem.get(), pMarker, ctx ).isValid(), "marker should hit at its point" );
  QVERIFY2( !lineCtrl.getEditContext( line.get(), pMarker, ctx ).isValid(), "line must NOT hit at marker (was the regression)" );

  // 2) Click in the U's empty gap (also inside line bbox, far from line):
  //    nothing should hit.
  const QgsPointXY pGap( 500, 60 );
  QVERIFY2( !polyCtrl.getEditContext( poly.get(), pGap, ctx ).isValid(), "polygon must NOT hit in U gap" );
  QVERIFY2( !lineCtrl.getEditContext( line.get(), pGap, ctx ).isValid(), "line must NOT hit in U gap" );

  // 3) Click ON the diagonal: only the line hits.
  const QgsPointXY pLine( 400, 400 );
  QVERIFY2( lineCtrl.getEditContext( line.get(), pLine, ctx ).isValid(), "line should hit on segment" );
  QVERIFY2( !markerCtrl.getEditContext( markerItem.get(), pLine, ctx ).isValid(), "marker should not hit far from its point" );
  QVERIFY2( !polyCtrl.getEditContext( poly.get(), pLine, ctx ).isValid(), "polygon should not hit far from its body" );

  // 4) Click in solid polygon arm: only polygon hits.
  const QgsPointXY pPoly( 420, 50 );
  QVERIFY2( polyCtrl.getEditContext( poly.get(), pPoly, ctx ).isValid(), "polygon should hit in solid body" );
  QVERIFY2( !lineCtrl.getEditContext( line.get(), pPoly, ctx ).isValid(), "line must NOT hit in polygon body (off-diagonal)" );
}


QTEST_MAIN( TestKadasAnnotationControllers )
#include "testkadasannotationcontrollers.moc"
