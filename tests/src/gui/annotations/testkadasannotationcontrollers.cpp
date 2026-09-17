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

#include <QAction>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QMenu>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFragment>
#include <QtTest/QTest>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsannotationmarkeritem.h>
#include <qgis/qgsannotationpolygonitem.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgslinesymbollayer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgsrendercontext.h>

#include <kadas/gui/kadasattachmentutils.h>
#include <kadas/gui/kadasrichtextdialog.h>
#include <kadas/gui/kadasattributetypes.h>
#include <kadas/gui/kadasfeaturepicker.h>
#include <kadas/gui/annotationitems/kadasannotationitemcontext.h>
#include <kadas/gui/annotationitems/kadascircleannotationcontroller.h>
#include <kadas/gui/annotationitems/kadascircleannotationitem.h>
#include <kadas/gui/annotationitems/kadascoordcrossannotationcontroller.h>
#include <kadas/gui/annotationitems/kadascoordcrossannotationitem.h>
#include <kadas/gui/annotationitems/kadaslineannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasmarkerannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasmilxannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasmilxannotationitem.h>
#include <kadas/gui/annotationitems/kadasannotationrotation.h>
#include <kadas/gui/annotationitems/kadaspinannotationcontroller.h>
#include <kadas/gui/annotationitems/kadaspinannotationitem.h>
#include <kadas/gui/annotationitems/kadaspolygonannotationcontroller.h>
#include <kadas/gui/annotationitems/kadasannotationvertexedit.h>
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
    void marker_getEditContext_hitsAnchorOffsetSymbolBody();
    void pin_getEditContext_hitsBodyAndTip();
    void pin_tooltip_composesTitleDescriptionAndPosition();
    void pin_tooltip_escapesPlainTextDescription();
    void richText_bareUrlsBecomeLinksOnSave();
    void pin_tooltip_keepsRichTextVerbatim();
    void richText_inlineImagesBecomeCappedProjectAttachments();
    void richText_insertedImagesAreShrunkToTheirDisplaySize();
    void richText_portraitImagesAreCappedOnTheirLongestEdge();
    void richText_adjacentIdenticalImagesBothSurvive();
    void richText_bareUrlKeepsBracketsItOpened();
    void attachment_legacyIdentifierResolvesThroughResourceProvider();
    void richText_unformattedTextIsStoredAsPlainText();

    // KadasRectangleAnnotationController ---------------------------------
    void rectangle_nodes_returnFourCornersPlusRotation();
    void rectangle_getEditContext_rotatedQuadHitTest();
    void rectangle_edit_movesCorrectCorner();
    void rectangle_edit_cornerDraggedPastOppositeKeepsAnchor();

    // KadasCircleAnnotationController ------------------------------------
    void circle_nodes_returnsCenterAndRing();
    void circle_edit_centerAndRingRoundtrip();

    // KadasCoordCrossAnnotationController ----------------------------------
    void coordcross_startPart_snapsToKmGridInMetricLayerCrs();
    void coordcross_startPart_snapsViaMetricCrsOnDegreeLayer();

    // KadasPinAnnotationItem ---------------------------------------------
    void pin_defaultIconPath_resolvesInQrc();

    // KadasMilxAnnotationItem --------------------------------------------
    void milx_boundingBox_coversRenderedGlyph();
    void milx_rotationTransform_turnsAboutThePivot();
    void milx_rotationHandle_rotatesSinglePointSymbol();

    // KadasLineAnnotationController --------------------------------------
    void line_getEditContext_hitsOnSegmentNotInBoundingBox();
    void line_getEditContext_hitsVertex();
    void line_midpointHandle_insertsOneVertexThenMovesIt();
    void line_deleteNode_keepsAtLeastTwoVertices();
    void line_endDecoration_roundTripsPerEnd();
    void line_endDecoration_matchesLineAndFlipsTail();
    void line_symbolPreviewWhileDrawing_onlyWhenDecorated();

    // KadasPolygonAnnotationController -----------------------------------
    void polygon_getEditContext_hitsBodyNotBoundingBox();
    void polygon_nodes_offerMidpointHandlesOnlyOnFinishedShape();
    void polygon_midpointHandle_insertsOneVertexThenMovesIt();
    void polygon_midpointHandle_growsTwoVertexRingIntoPolygon();
    void polygon_deleteNode_reclosesRingAndKeepsAtLeastThree();

    // Multi-type hit isolation -------------------------------------------
    void hitTest_multipleTypesSelectsByGeometryNotBbox();

    // Selection ranking --------------------------------------------------
    void selection_lineEdgeHitIsPrecise();
    void selection_polygonBodyHitIsBody();
    void selection_rankerPrefersPrecisionOverZIndex();

    // Target layer eligibility -------------------------------------------
    void supportsLayer_acceptsPlainAnnotationLayersOnly();
    void supportsLayer_milxRequiresWgs84Layer();

  private:
    static KadasAnnotationItemContext makeContext();
};


void TestKadasAnnotationControllers::supportsLayer_acceptsPlainAnnotationLayersOnly()
{
  // The layer chooser of the create tool offers exactly what the controller
  // accepts, so an item type that is happy anywhere must still refuse the
  // parametric overlays, whose content comes from layer settings.
  KadasMarkerAnnotationController controller;

  QgsAnnotationLayer::LayerOptions options( ( QgsCoordinateTransformContext() ) );
  auto plain = std::make_unique<QgsAnnotationLayer>( QStringLiteral( "Annotation" ), options );
  plain->setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:2056" ) ) );
  QVERIFY( controller.supportsLayer( plain.get() ) );

  auto parametric = std::make_unique<QgsAnnotationLayer>( QStringLiteral( "Bullseye" ), options );
  parametric->setCustomProperty( QStringLiteral( "kadas/annotation-type" ), QStringLiteral( "bullseye" ) );
  QVERIFY( !controller.supportsLayer( parametric.get() ) );

  QVERIFY( !controller.supportsLayer( nullptr ) );
  // No preference means the new layer follows the project CRS.
  QVERIFY( !controller.preferredLayerCrs().isValid() );
}

void TestKadasAnnotationControllers::supportsLayer_milxRequiresWgs84Layer()
{
  // MSS geometry is stored in WGS84 and the item reads its layer CRS as being
  // that, so offering a layer in any other CRS would place symbols nowhere.
  KadasMilxAnnotationController controller;

  QgsAnnotationLayer::LayerOptions options( ( QgsCoordinateTransformContext() ) );
  auto wgs84 = std::make_unique<QgsAnnotationLayer>( QStringLiteral( "MSS" ), options );
  wgs84->setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ) );
  QVERIFY( controller.supportsLayer( wgs84.get() ) );

  auto projected = std::make_unique<QgsAnnotationLayer>( QStringLiteral( "Annotation" ), options );
  projected->setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:2056" ) ) );
  QVERIFY( !controller.supportsLayer( projected.get() ) );

  QCOMPARE( controller.preferredLayerCrs().authid(), QStringLiteral( "EPSG:4326" ) );
}

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

void TestKadasAnnotationControllers::marker_getEditContext_hitsAnchorOffsetSymbolBody()
{
  // Regression: pins use an SVG anchored at their bottom tip, so the
  // visible body extends well above the geographic anchor. Hit-testing
  // a fixed circular tolerance around the anchor (the legacy behavior)
  // would miss most of the visible icon and pins were unselectable.
  // The controller must hit-test against the rendered symbol footprint.
  KadasMarkerAnnotationController controller;
  const auto ctx = makeContext();

  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 100, 200 ), ctx );
  auto *marker = static_cast<QgsAnnotationMarkerItem *>( item.get() );

  // Build a tall, bottom-anchored simple marker that mimics a pin:
  //  - 20 mm at 96 DPI ≈ 75 px tall
  //  - anchored at the bottom so the body is rendered ABOVE the anchor.
  // makeContext() uses 2 m/px so the symbol body extends ≈ 150 m above
  // the anchor in map coordinates.
  auto *sl = new QgsSimpleMarkerSymbolLayer( Qgis::MarkerShape::Square );
  sl->setSize( 20.0 );
  sl->setVerticalAnchorPoint( Qgis::VerticalAnchorPoint::Bottom );
  sl->setColor( QColor( 255, 0, 0 ) );
  marker->setSymbol( new QgsMarkerSymbol( QgsSymbolLayerList() << sl ) );

  // Click 60 m ABOVE the anchor — well above the legacy ~10 m circular
  // tolerance, but inside the bottom-anchored symbol's body.
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 100, 260 ), ctx );
  QVERIFY2( ec.isValid(), "click on bottom-anchored marker body must hit" );

  // Right on the anchor still hits.
  ec = controller.getEditContext( item.get(), QgsPointXY( 100, 200 ), ctx );
  QVERIFY( ec.isValid() );

  // 100 m BELOW the anchor — outside the bottom-anchored symbol body.
  ec = controller.getEditContext( item.get(), QgsPointXY( 100, 100 ), ctx );
  QVERIFY2( !ec.isValid(), "click below bottom-anchored marker must miss" );
}


// ----- Pin (probe + behavioral test) ------------------------------------

void TestKadasAnnotationControllers::pin_getEditContext_hitsBodyAndTip()
{
  // Regression for the Kadas pin (bottom-anchored SVG marker): clicks on
  // the visible body must register as hits. The body extends UPWARD from
  // the geographic anchor (anchor = tip), so clicks above the anchor in
  // map y (i.e. larger map-y at this CRS) hit, clicks well below miss.
  //
  // QgsSvgMarkerSymbolLayer::bounds() over-shifts the symbol footprint by
  // a scaleFactor-dependent amount; the controller normalizes the anchor
  // and re-applies it itself. Without that workaround pins were
  // unselectable.
  KadasPinAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 0, 0 ), ctx );

  // What is under test is the anchor handling, not the shipped default size,
  // so fix the symbol at 24 mm and keep the distances below meaningful.
  auto *marker = static_cast<QgsAnnotationMarkerItem *>( item.get() );
  std::unique_ptr<QgsMarkerSymbol> sym( marker->symbol()->clone() );
  static_cast<QgsSvgMarkerSymbolLayer *>( sym->symbolLayer( 0 ) )->setSize( 24.0 );
  marker->setSymbol( sym.release() );

  // Pin renders ≈ 24 mm tall at 96 dpi = ≈ 91 px ≈ 182 m in this CRS
  // (mupp = 2). With Bottom anchor the body spans map-y [0, 182] above
  // the anchor, and map-x ≈ [-90, 90].

  // Hit: 100 m above the anchor — squarely in the body.
  KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 0, 100 ), ctx );
  QVERIFY2( ec.isValid(), "click on pin body must hit" );

  // Hit: 50 m above and offset 40 m to the side — still inside the body.
  ec = controller.getEditContext( item.get(), QgsPointXY( 40, 50 ), ctx );
  QVERIFY2( ec.isValid(), "click on offset pin body must hit" );

  // Hit: right at the geographic anchor (tip) — bottom edge of bounds.
  ec = controller.getEditContext( item.get(), QgsPointXY( 0, 0 ), ctx );
  QVERIFY2( ec.isValid(), "click on pin tip must hit" );

  // Miss: well below the anchor (south of the tip), outside the body.
  ec = controller.getEditContext( item.get(), QgsPointXY( 0, -100 ), ctx );
  QVERIFY2( !ec.isValid(), "click below pin tip must miss" );

  // Miss: well above the body (north of the head).
  ec = controller.getEditContext( item.get(), QgsPointXY( 0, 400 ), ctx );
  QVERIFY2( !ec.isValid(), "click far above pin head must miss" );

  // Miss: far to the side.
  ec = controller.getEditContext( item.get(), QgsPointXY( 300, 50 ), ctx );
  QVERIFY2( !ec.isValid(), "click far to the side of pin must miss" );
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


void TestKadasAnnotationControllers::rectangle_edit_cornerDraggedPastOppositeKeepsAnchor()
{
  KadasRectangleAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  auto *rect = static_cast<KadasRectangleAnnotationItem *>( item.get() );
  rect->setBox( QgsPointXY( 0, 0 ), QSizeF( 100, 100 ), 0.0 );

  // Grab the BR corner (50, -50) the way the map tool does; the anchor is the
  // opposite TL corner (-50, 50) and must stay put for the whole drag.
  const KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 50, -50 ), ctx );
  QCOMPARE( ec.vidx.vertex, 1 );

  // Still on the near side of the anchor.
  controller.edit( item.get(), ec, QgsPointXY( -20, 20 ), ctx );
  QCOMPARE( rect->center(), QgsPointXY( -35, 35 ) );
  QCOMPARE( rect->size(), QSizeF( 30, 30 ) );

  // Past the anchor in both axes: the box mirrors about the TL corner, which
  // makes the dragged corner swap places with the anchor in the item's own
  // corner ordering.
  controller.edit( item.get(), ec, QgsPointXY( -100, 100 ), ctx );
  QCOMPARE( rect->center(), QgsPointXY( -75, 75 ) );
  QCOMPARE( rect->size(), QSizeF( 50, 50 ) );

  // Regression: the next step must keep pivoting around (-50, 50) rather than
  // around whatever corner now carries index 3, which would drag the whole
  // rectangle along with the cursor.
  controller.edit( item.get(), ec, QgsPointXY( -150, 150 ), ctx );
  QCOMPARE( rect->center(), QgsPointXY( -100, 100 ) );
  QCOMPARE( rect->size(), QSizeF( 100, 100 ) );
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


// ----- CoordCross --------------------------------------------------------

void TestKadasAnnotationControllers::coordcross_startPart_snapsToKmGridInMetricLayerCrs()
{
  // Metric layer CRS: the position snaps to a round km directly in the
  // layer CRS (which is also the labelling CRS).
  const QgsCoordinateReferenceSystem crs( QStringLiteral( "EPSG:2056" ) );
  QgsMapSettings ms;
  ms.setDestinationCrs( crs );
  ms.setExtent( QgsRectangle( 2599000, 1199000, 2601000, 1201000 ) );
  ms.setOutputSize( QSize( 1000, 1000 ) );
  ms.setOutputDpi( 96 );
  QgsAnnotationLayer layer( QStringLiteral( "cross-metric" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  layer.setCrs( crs );
  const KadasAnnotationItemContext ctx( &layer, ms );

  QCOMPARE( KadasCoordCrossAnnotationItem::labelCrs( crs ), crs );

  KadasCoordCrossAnnotationController controller;
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 2600123.4, 1200456.7 ), ctx );

  const auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item.get() );
  QVERIFY( marker );
  QCOMPARE( marker->geometry().x(), 2600000.0 );
  QCOMPARE( marker->geometry().y(), 1200000.0 );
}

void TestKadasAnnotationControllers::coordcross_startPart_snapsViaMetricCrsOnDegreeLayer()
{
  // Degree-based layer CRS: rounding raw lat/lon to the nearest 1000
  // would collapse every position to (0, 0) — Null Island. The controller
  // must snap on the EPSG:3857 km grid instead and store the transformed
  // position back in the layer CRS.
  const QgsCoordinateReferenceSystem layerCrs( QStringLiteral( "EPSG:4326" ) );
  const QgsCoordinateReferenceSystem mapCrs( QStringLiteral( "EPSG:3857" ) );
  QgsMapSettings ms;
  ms.setDestinationCrs( mapCrs );
  ms.setExtent( QgsRectangle( 820000, 5930000, 840000, 5950000 ) );
  ms.setOutputSize( QSize( 1000, 1000 ) );
  ms.setOutputDpi( 96 );
  QgsAnnotationLayer layer( QStringLiteral( "cross-degrees" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  layer.setCrs( layerCrs );
  const KadasAnnotationItemContext ctx( &layer, ms );

  QCOMPARE( KadasCoordCrossAnnotationItem::labelCrs( layerCrs ), mapCrs );

  KadasCoordCrossAnnotationController controller;
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  // Click near Bern in EPSG:3857 map coords.
  controller.startPart( item.get(), QgsPointXY( 828437.0, 5933749.0 ), ctx );

  const auto *marker = dynamic_cast<QgsAnnotationMarkerItem *>( item.get() );
  QVERIFY( marker );
  // Stored in degrees: must NOT have been rounded to (0, 0).
  QVERIFY( std::abs( marker->geometry().x() - 7.44 ) < 0.1 );
  QVERIFY( std::abs( marker->geometry().y() - 46.9 ) < 0.1 );
  // Transformed back to the metric labelling CRS, the position sits on
  // the round-km grid.
  const QgsCoordinateTransform ct( layerCrs, mapCrs, QgsCoordinateTransformContext() );
  const QgsPointXY snapped = ct.transform( QgsPointXY( marker->geometry().x(), marker->geometry().y() ) );
  QVERIFY2( std::abs( snapped.x() - 828000.0 ) < 0.001, qPrintable( QString::number( snapped.x(), 'f', 4 ) ) );
  QVERIFY2( std::abs( snapped.y() - 5934000.0 ) < 0.001, qPrintable( QString::number( snapped.y(), 'f', 4 ) ) );
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

void TestKadasAnnotationControllers::line_endDecoration_roundTripsPerEnd()
{
  using Ctrl = KadasLineAnnotationController;

  QgsLineSymbol symbol( QgsSymbolLayerList() << new QgsSimpleLineSymbolLayer() );
  QVERIFY( !Ctrl::endDecoration( &symbol, Ctrl::LineEnd::Head ).shape.has_value() );
  QVERIFY( !Ctrl::endDecoration( &symbol, Ctrl::LineEnd::Tail ).shape.has_value() );

  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Head, { Qgis::MarkerShape::ArrowHeadFilled, 6.0 } );
  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Tail, { Qgis::MarkerShape::Circle, 2.5 } );
  QCOMPARE( symbol.symbolLayerCount(), 3 );

  Ctrl::EndDecoration head = Ctrl::endDecoration( &symbol, Ctrl::LineEnd::Head );
  QCOMPARE( head.shape, Qgis::MarkerShape::ArrowHeadFilled );
  QCOMPARE( head.size, 6.0 );
  Ctrl::EndDecoration tail = Ctrl::endDecoration( &symbol, Ctrl::LineEnd::Tail );
  QCOMPARE( tail.shape, Qgis::MarkerShape::Circle );
  QCOMPARE( tail.size, 2.5 );

  // Re-setting an end replaces its decoration rather than stacking a second one.
  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Head, { Qgis::MarkerShape::Square, 3.0 } );
  QCOMPARE( symbol.symbolLayerCount(), 3 );
  QCOMPARE( Ctrl::endDecoration( &symbol, Ctrl::LineEnd::Head ).shape, Qgis::MarkerShape::Square );
  // ... and the other end is untouched.
  QCOMPARE( Ctrl::endDecoration( &symbol, Ctrl::LineEnd::Tail ).shape, Qgis::MarkerShape::Circle );

  // A shapeless decoration clears the end, leaving the plain line behind.
  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Head, {} );
  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Tail, {} );
  QCOMPARE( symbol.symbolLayerCount(), 1 );
  QVERIFY( dynamic_cast<QgsSimpleLineSymbolLayer *>( symbol.symbolLayer( 0 ) ) );
}

void TestKadasAnnotationControllers::line_endDecoration_matchesLineAndFlipsTail()
{
  using Ctrl = KadasLineAnnotationController;

  auto *base = new QgsSimpleLineSymbolLayer();
  base->setColor( QColor( 0, 128, 255 ) );
  base->setWidth( 1.2 );
  QgsLineSymbol symbol( QgsSymbolLayerList() << base );

  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Head, { Qgis::MarkerShape::ArrowHeadFilled, 4.0 } );
  Ctrl::setEndDecoration( &symbol, Ctrl::LineEnd::Tail, { Qgis::MarkerShape::ArrowHeadFilled, 4.0 } );

  const auto markerAt = [&symbol]( Qgis::MarkerLinePlacement placement ) -> const QgsSimpleMarkerSymbolLayer * {
    for ( int i = 0; i < symbol.symbolLayerCount(); ++i )
    {
      auto *ml = dynamic_cast<QgsMarkerLineSymbolLayer *>( symbol.symbolLayer( i ) );
      if ( !ml || ml->placements() != Qgis::MarkerLinePlacements( placement ) )
        continue;
      auto *sub = dynamic_cast<QgsMarkerSymbol *>( ml->subSymbol() );
      return sub ? dynamic_cast<const QgsSimpleMarkerSymbolLayer *>( sub->symbolLayer( 0 ) ) : nullptr;
    }
    return nullptr;
  };

  const QgsSimpleMarkerSymbolLayer *head = markerAt( Qgis::MarkerLinePlacement::LastVertex );
  QVERIFY( head );
  QCOMPARE( head->color(), QColor( 0, 128, 255 ) );
  QCOMPARE( head->strokeColor(), QColor( 0, 128, 255 ) );
  QCOMPARE( head->strokeWidth(), 1.2 );
  // The marker line aims both ends forwards along the line, so only the tail is flipped.
  QCOMPARE( head->angle(), 0.0 );

  const QgsSimpleMarkerSymbolLayer *tail = markerAt( Qgis::MarkerLinePlacement::FirstVertex );
  QVERIFY( tail );
  QCOMPARE( tail->angle(), 180.0 );
}

void TestKadasAnnotationControllers::line_symbolPreviewWhileDrawing_onlyWhenDecorated()
{
  using Ctrl = KadasLineAnnotationController;
  Ctrl controller;

  auto *ls = new QgsLineString( QVector<QgsPoint> { QgsPoint( 0, 0 ), QgsPoint( 10, 10 ) } );
  auto item = std::make_unique<QgsAnnotationLineItem>( ls );
  item->setSymbol( new QgsLineSymbol( QgsSymbolLayerList() << new QgsSimpleLineSymbolLayer() ) );

  // A plain line looks the same either way, so it keeps the cheaper rubber band.
  QVERIFY( !controller.symbolPreviewWhileDrawing( item.get() ) );

  // A decorated end is invisible to a rubber band, so the real symbol is drawn.
  const auto decorate = [&item]( Ctrl::LineEnd end, const Ctrl::EndDecoration &decoration ) {
    QgsLineSymbol *clone = item->symbol()->clone();
    Ctrl::setEndDecoration( clone, end, decoration );
    item->setSymbol( clone );
  };
  decorate( Ctrl::LineEnd::Head, { Qgis::MarkerShape::ArrowHeadFilled, 4.0 } );
  QVERIFY( controller.symbolPreviewWhileDrawing( item.get() ) );

  decorate( Ctrl::LineEnd::Head, {} );
  decorate( Ctrl::LineEnd::Tail, { Qgis::MarkerShape::Circle, 3.0 } );
  QVERIFY( controller.symbolPreviewWhileDrawing( item.get() ) );

  decorate( Ctrl::LineEnd::Tail, {} );
  QVERIFY( !controller.symbolPreviewWhileDrawing( item.get() ) );
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


namespace
{
  // The vertices of a line or polygon ring, in item CRS.
  QVector<QgsPointXY> ringPoints( const QgsCurve *curve )
  {
    QVector<QgsPointXY> pts;
    if ( !curve )
      return pts;
    for ( int i = 0; i < curve->numPoints(); ++i )
    {
      const QgsPoint p = curve->vertexAt( QgsVertexId( 0, 0, i ) );
      pts.append( QgsPointXY( p.x(), p.y() ) );
    }
    return pts;
  }

  // The "Delete node" entry a controller put in \a menu, or nullptr.
  QAction *deleteNodeAction( const QMenu &menu )
  {
    const auto actions = menu.actions();
    for ( QAction *action : actions )
    {
      if ( action->text() == QLatin1String( "Delete node" ) )
        return action;
    }
    return nullptr;
  }
} //namespace

void TestKadasAnnotationControllers::line_midpointHandle_insertsOneVertexThenMovesIt()
{
  KadasLineAnnotationController controller;
  const auto ctx = makeContext();
  auto item = makeLine( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ) } );

  // 2 vertices + 1 rotation handle + 1 midpoint handle.
  QCOMPARE( controller.nodes( item.get(), ctx ).size(), 4 );

  // Grab the midpoint of the only segment.
  const KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 50, 0 ), ctx );
  QVERIFY( ec.isValid() );
  QCOMPARE( ec.vidx.part, KadasAnnotationVertexEdit::kPartInsert );
  QCOMPARE( ec.vidx.vertex, 0 );
  QVERIFY2( ec.appliesOnClick, "a midpoint handle must act on a plain click too" );

  // First drag step materialises the vertex between the two existing ones.
  controller.edit( item.get(), ec, QgsPointXY( 50, 50 ), ctx );
  QCOMPARE( ringPoints( item->geometry() ), QVector<QgsPointXY>( { QgsPointXY( 0, 0 ), QgsPointXY( 50, 50 ), QgsPointXY( 100, 0 ) } ) );

  // Every later step of the same drag moves that vertex rather than inserting
  // another one behind the cursor.
  controller.edit( item.get(), ec, QgsPointXY( 50, 80 ), ctx );
  QCOMPARE( ringPoints( item->geometry() ), QVector<QgsPointXY>( { QgsPointXY( 0, 0 ), QgsPointXY( 50, 80 ), QgsPointXY( 100, 0 ) } ) );
}

void TestKadasAnnotationControllers::line_deleteNode_keepsAtLeastTwoVertices()
{
  KadasLineAnnotationController controller;
  const auto ctx = makeContext();
  auto item = makeLine( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ) } );

  const KadasEditContext onVertex = controller.getEditContext( item.get(), QgsPointXY( 100, 0 ), ctx );
  QCOMPARE( onVertex.vidx.vertex, 1 );

  QMenu menu;
  controller.populateContextMenu( item.get(), &menu, onVertex, QgsPointXY( 100, 0 ), ctx );
  QAction *remove = deleteNodeAction( menu );
  QVERIFY( remove );
  QVERIFY( remove->isEnabled() );
  remove->trigger();
  QCOMPARE( ringPoints( item->geometry() ), QVector<QgsPointXY>( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 100 ) } ) );

  // Two vertices are the least a line can carry, so the entry now refuses.
  const KadasEditContext onLastTwo = controller.getEditContext( item.get(), QgsPointXY( 0, 0 ), ctx );
  QMenu tooFew;
  controller.populateContextMenu( item.get(), &tooFew, onLastTwo, QgsPointXY( 0, 0 ), ctx );
  QAction *blocked = deleteNodeAction( tooFew );
  QVERIFY( blocked );
  QVERIFY( !blocked->isEnabled() );
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


void TestKadasAnnotationControllers::polygon_nodes_offerMidpointHandlesOnlyOnFinishedShape()
{
  KadasPolygonAnnotationController controller;
  auto ctx = makeContext();
  auto item = makePolygon( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ), QgsPointXY( 0, 100 ), QgsPointXY( 0, 0 ) } );

  // 4 vertices + 1 rotation handle + 4 midpoint handles (the closing segment
  // gets one too).
  QCOMPARE( controller.nodes( item.get(), ctx ).size(), 9 );

  // While digitizing the rubber-band segments would sprout midpoint handles
  // that chase the cursor, so they are left out.
  ctx.setDigitizing( true );
  QCOMPARE( controller.nodes( item.get(), ctx ).size(), 5 );
  QVERIFY( !controller.getEditContext( item.get(), QgsPointXY( 50, 0 ), ctx ).vidx.isValid() );
}

void TestKadasAnnotationControllers::polygon_midpointHandle_insertsOneVertexThenMovesIt()
{
  KadasPolygonAnnotationController controller;
  const auto ctx = makeContext();
  auto item = makePolygon( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ), QgsPointXY( 0, 100 ), QgsPointXY( 0, 0 ) } );

  // Grab the midpoint of the closing segment, the one that runs from the last
  // vertex back to the first.
  const KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 0, 50 ), ctx );
  QVERIFY( ec.isValid() );
  QCOMPARE( ec.vidx.part, KadasAnnotationVertexEdit::kPartInsert );
  QCOMPARE( ec.vidx.vertex, 3 );

  controller.edit( item.get(), ec, QgsPointXY( -40, 50 ), ctx );
  QCOMPARE( ringPoints( item->geometry()->exteriorRing() ), QVector<QgsPointXY>( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ), QgsPointXY( 0, 100 ), QgsPointXY( -40, 50 ), QgsPointXY( 0, 0 ) } ) );

  // Later steps move the new vertex; the closing duplicate stays last.
  controller.edit( item.get(), ec, QgsPointXY( -80, 50 ), ctx );
  QCOMPARE( ringPoints( item->geometry()->exteriorRing() ), QVector<QgsPointXY>( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ), QgsPointXY( 0, 100 ), QgsPointXY( -80, 50 ), QgsPointXY( 0, 0 ) } ) );
}

void TestKadasAnnotationControllers::polygon_midpointHandle_growsTwoVertexRingIntoPolygon()
{
  // A polygon abandoned after two points renders as a line and encloses
  // nothing. Its one midpoint handle is what turns it back into a polygon,
  // since digitizing cannot be resumed.
  KadasPolygonAnnotationController controller;
  const auto ctx = makeContext();
  auto item = makePolygon( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 0, 0 ) } );

  // 2 vertices, no rotation handle (there is no shape to rotate yet) and a
  // single midpoint handle: the ring's two segments run along the same stretch.
  QCOMPARE( controller.nodes( item.get(), ctx ).size(), 3 );

  const KadasEditContext ec = controller.getEditContext( item.get(), QgsPointXY( 50, 0 ), ctx );
  QCOMPARE( ec.vidx.part, KadasAnnotationVertexEdit::kPartInsert );
  QCOMPARE( ec.vidx.vertex, 0 );

  controller.edit( item.get(), ec, QgsPointXY( 50, 60 ), ctx );
  QCOMPARE( ringPoints( item->geometry()->exteriorRing() ), QVector<QgsPointXY>( { QgsPointXY( 0, 0 ), QgsPointXY( 50, 60 ), QgsPointXY( 100, 0 ), QgsPointXY( 0, 0 ) } ) );
  QVERIFY( item->geometry()->area() > 0.0 );
}

void TestKadasAnnotationControllers::polygon_deleteNode_reclosesRingAndKeepsAtLeastThree()
{
  KadasPolygonAnnotationController controller;
  const auto ctx = makeContext();
  auto item = makePolygon( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ), QgsPointXY( 0, 100 ), QgsPointXY( 0, 0 ) } );

  // Deleting the first vertex leaves the closing duplicate standing on a vertex
  // that is gone, so the ring has to be re-closed on the new first one.
  const KadasEditContext onFirst = controller.getEditContext( item.get(), QgsPointXY( 0, 0 ), ctx );
  QCOMPARE( onFirst.vidx.vertex, 0 );
  QMenu menu;
  controller.populateContextMenu( item.get(), &menu, onFirst, QgsPointXY( 0, 0 ), ctx );
  QAction *remove = deleteNodeAction( menu );
  QVERIFY( remove );
  QVERIFY( remove->isEnabled() );
  remove->trigger();
  QCOMPARE( ringPoints( item->geometry()->exteriorRing() ), QVector<QgsPointXY>( { QgsPointXY( 100, 0 ), QgsPointXY( 100, 100 ), QgsPointXY( 0, 100 ), QgsPointXY( 100, 0 ) } ) );

  // Three vertices are the least a polygon can carry, so the entry now refuses.
  const KadasEditContext onTriangle = controller.getEditContext( item.get(), QgsPointXY( 100, 0 ), ctx );
  QMenu tooFew;
  controller.populateContextMenu( item.get(), &tooFew, onTriangle, QgsPointXY( 100, 0 ), ctx );
  QAction *blocked = deleteNodeAction( tooFew );
  QVERIFY( blocked );
  QVERIFY( !blocked->isEnabled() );
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


// ----- Selection ranking ------------------------------------------------

void TestKadasAnnotationControllers::selection_lineEdgeHitIsPrecise()
{
  // Regression: a click that falls on a line's stroke is a geometrically
  // precise hit. The KadasLineAnnotationController returns an edit
  // context with an invalid vidx (the subsequent drag uses whole-line
  // move semantics), so the default precision derivation from vidx
  // alone would mistakenly tag it as Body. The controller must
  // explicitly upgrade it to Precise so the canvas picker outranks a
  // higher-z polygon whose body merely contains the same click.
  KadasLineAnnotationController lineCtrl;
  const auto ctx = makeContext();
  auto line = makeLine( { QgsPointXY( 0, 0 ), QgsPointXY( 100, 0 ) } );

  // On a vertex: precise (covers the default vidx-derived path).
  KadasEditContext ecVertex = lineCtrl.getEditContext( line.get(), QgsPointXY( 0, 0 ), ctx );
  QVERIFY( ecVertex.isValid() );
  QVERIFY( ecVertex.vidx.isValid() );
  QCOMPARE( ecVertex.precision, KadasEditContext::HitPrecision::Precise );

  // On the segment between vertices: also precise (covers the explicit
  // upgrade in the edge-hit branch). Away from the midpoint, which belongs to
  // the insert handle.
  KadasEditContext ecEdge = lineCtrl.getEditContext( line.get(), QgsPointXY( 25, 0 ), ctx );
  QVERIFY( ecEdge.isValid() );
  QVERIFY( !ecEdge.vidx.isValid() ); // whole-line drag, no vertex
  QCOMPARE( ecEdge.precision, KadasEditContext::HitPrecision::Precise );
}

void TestKadasAnnotationControllers::selection_polygonBodyHitIsBody()
{
  // The polygon containment hit is geometrically loose: the click is
  // anywhere inside the filled body. The edit context returned by
  // KadasPolygonAnnotationController has an invalid vidx so the
  // default-derived precision must be Body.
  KadasPolygonAnnotationController polyCtrl;
  const auto ctx = makeContext();
  auto poly = makePolygon( {
    QgsPointXY( 0, 0 ),
    QgsPointXY( 100, 0 ),
    QgsPointXY( 100, 100 ),
    QgsPointXY( 0, 100 ),
    QgsPointXY( 0, 0 ),
  } );

  KadasEditContext ec = polyCtrl.getEditContext( poly.get(), QgsPointXY( 50, 50 ), ctx );
  QVERIFY( ec.isValid() );
  QVERIFY( !ec.vidx.isValid() );
  QCOMPARE( ec.precision, KadasEditContext::HitPrecision::Body );
}

void TestKadasAnnotationControllers::selection_rankerPrefersPrecisionOverZIndex()
{
  // End-to-end regression for KadasFeaturePicker::rankAnnotationCandidates:
  // a low-z line whose edge is hit by the click must outrank a high-z
  // polygon whose body merely contains the same click. Without the
  // precision tier, the high-z polygon would have won.
  KadasFeaturePicker::AnnotationPickCandidate lineCand;
  lineCand.itemId = QStringLiteral( "line" );
  lineCand.precision = KadasEditContext::HitPrecision::Precise;
  lineCand.zIndex = 1;
  lineCand.bboxArea = 1000000.0;

  KadasFeaturePicker::AnnotationPickCandidate polyCand;
  polyCand.itemId = QStringLiteral( "poly" );
  polyCand.precision = KadasEditContext::HitPrecision::Body;
  polyCand.zIndex = 99;      // way higher
  polyCand.bboxArea = 100.0; // way smaller

  // Order must not matter.
  {
    const QList<KadasFeaturePicker::AnnotationPickCandidate> list { lineCand, polyCand };
    const int best = KadasFeaturePicker::rankAnnotationCandidates( list );
    QCOMPARE( best, 0 );
    QCOMPARE( list.at( best ).itemId, QStringLiteral( "line" ) );
  }
  {
    const QList<KadasFeaturePicker::AnnotationPickCandidate> list { polyCand, lineCand };
    const int best = KadasFeaturePicker::rankAnnotationCandidates( list );
    QCOMPARE( best, 1 );
    QCOMPARE( list.at( best ).itemId, QStringLiteral( "line" ) );
  }

  // Within the same precision tier the existing z-then-area tiebreakers
  // must still apply.
  KadasFeaturePicker::AnnotationPickCandidate a;
  a.itemId = QStringLiteral( "a" );
  a.precision = KadasEditContext::HitPrecision::Body;
  a.zIndex = 1;
  a.bboxArea = 10.0;
  KadasFeaturePicker::AnnotationPickCandidate b;
  b.itemId = QStringLiteral( "b" );
  b.precision = KadasEditContext::HitPrecision::Body;
  b.zIndex = 2; // higher z wins over a
  b.bboxArea = 100.0;
  KadasFeaturePicker::AnnotationPickCandidate c;
  c.itemId = QStringLiteral( "c" );
  c.precision = KadasEditContext::HitPrecision::Body;
  c.zIndex = 2;      // tie with b on z
  c.bboxArea = 50.0; // smaller area wins
  const QList<KadasFeaturePicker::AnnotationPickCandidate> tie { a, b, c };
  const int bestIdx = KadasFeaturePicker::rankAnnotationCandidates( tie );
  QCOMPARE( tie.at( bestIdx ).itemId, QStringLiteral( "c" ) );

  // Empty list returns -1.
  QCOMPARE( KadasFeaturePicker::rankAnnotationCandidates( {} ), -1 );
}


void TestKadasAnnotationControllers::pin_tooltip_composesTitleDescriptionAndPosition()
{
  // The pin tooltip is composed on every hover rather than stored, so it can
  // report where the pin is and how high the terrain is under it.
  KadasPinAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 1000, 2000 ), ctx );
  auto *pin = static_cast<KadasPinAnnotationItem *>( item.get() );
  pin->setName( QStringLiteral( "Base camp" ) );
  pin->setRemarks( QStringLiteral( "Reachable on foot" ) );

  const QString tooltip = controller.tooltip( item.get(), ctx.itemCrs() );
  // The position/altitude band leads, the pin's own text follows.
  QVERIFY( tooltip.startsWith( QStringLiteral( "<table" ) ) );
  QVERIFY( tooltip.contains( QStringLiteral( "<b>Base camp</b>" ) ) );
  QVERIFY( tooltip.contains( QStringLiteral( "Reachable on foot" ) ) );
  QVERIFY( tooltip.contains( QStringLiteral( ">Position</font>" ) ) );
  // No heightmap is configured in a bare test project, and the tooltip says so
  // instead of quietly reporting 0 m.
  QVERIFY( tooltip.contains( QStringLiteral( ">Altitude</font>" ) ) );
  QVERIFY( tooltip.contains( QStringLiteral( "undefined (no heightmap defined)" ) ) );

  // An item that is not a pin has no live tooltip, so the caller falls back to
  // whatever the layer stored.
  KadasMarkerAnnotationController markerController;
  std::unique_ptr<QgsAnnotationItem> marker( markerController.createItem() );
  QVERIFY( markerController.tooltip( marker.get(), ctx.itemCrs() ).isEmpty() );
}

void TestKadasAnnotationControllers::pin_tooltip_escapesPlainTextDescription()
{
  // A description that never went through the editor is plain text, so the
  // tooltip has to escape it rather than let it act as markup.
  KadasPinAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 0, 0 ), ctx );
  auto *pin = static_cast<KadasPinAnnotationItem *>( item.get() );
  const QString remarks = QStringLiteral( "5 < 10 & rising\nsecond line" );
  QVERIFY2( !Qt::mightBeRichText( remarks ), "fixture must exercise the plain-text branch" );
  pin->setRemarks( remarks );

  const QString tooltip = controller.tooltip( item.get(), ctx.itemCrs() );
  QVERIFY( tooltip.contains( QStringLiteral( "5 &lt; 10 &amp; rising" ) ) );
  QVERIFY( tooltip.contains( QStringLiteral( "<br>second line" ) ) );
}

void TestKadasAnnotationControllers::richText_bareUrlsBecomeLinksOnSave()
{
  // Bare URLs are linked into what the editor stores, not into what a viewer
  // renders, so a typed address is a real link everywhere the markup is read.
  QTextDocument document;
  document.setHtml( QStringLiteral(
    "<p>see https://example.org/a?b=1, and www.example.com.</p>"
    "<p>plus <a href=\"https://example.org/keep\">a label</a></p>"
  ) );
  KadasRichTextDialog::linkifyBareUrls( &document );

  QStringList anchors;
  for ( QTextBlock block = document.begin(); block.isValid(); block = block.next() )
  {
    for ( QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it )
    {
      const QTextFragment fragment = it.fragment();
      if ( fragment.isValid() && fragment.charFormat().isAnchor() )
        anchors << QStringLiteral( "%1 -> %2" ).arg( fragment.text(), fragment.charFormat().anchorHref() );
    }
  }

  const QStringList expected {
    // Trailing sentence punctuation stays outside the link...
    QStringLiteral( "https://example.org/a?b=1 -> https://example.org/a?b=1" ),
    // ...and a bare host gains a scheme, so QDesktopServices can open it.
    QStringLiteral( "www.example.com -> http://www.example.com" ),
    // What the author linked by hand keeps both its label and its target.
    QStringLiteral( "a label -> https://example.org/keep" ),
  };
  QCOMPARE( anchors, expected );

  // Running it again finds nothing new to do.
  const QString before = document.toHtml();
  KadasRichTextDialog::linkifyBareUrls( &document );
  QCOMPARE( document.toHtml(), before );
}

void TestKadasAnnotationControllers::pin_tooltip_keepsRichTextVerbatim()
{
  // A rich-text description already carries its own anchors and images, so the
  // tooltip must pass it through rather than escape it and re-link it.
  KadasPinAnnotationController controller;
  const auto ctx = makeContext();
  std::unique_ptr<QgsAnnotationItem> item( controller.createItem() );
  controller.startPart( item.get(), QgsPointXY( 0, 0 ), ctx );
  auto *pin = static_cast<KadasPinAnnotationItem *>( item.get() );
  pin->setRemarks( QStringLiteral(
    "<p>See <a href=\"https://example.org/plan\">the plan</a></p>"
    "<p><img src=\"attachment:///photo.png\" width=\"280\" height=\"140\"/></p>"
  ) );

  const QString tooltip = controller.tooltip( item.get(), ctx.itemCrs() );
  QVERIFY( tooltip.contains( QStringLiteral( "<a href=\"https://example.org/plan\">the plan</a>" ) ) );
  QVERIFY( tooltip.contains( QStringLiteral( "<img src=\"attachment:///photo.png\"" ) ) );
  QVERIFY2( !tooltip.contains( QStringLiteral( "&lt;" ) ), qPrintable( tooltip ) );
}

void TestKadasAnnotationControllers::richText_inlineImagesBecomeCappedProjectAttachments()
{
  // QgsRichTextEditor embeds an inserted image inline at its original
  // resolution. Storing that verbatim would put megabytes of base64 into a
  // single XML attribute, so it is moved into the project archive on the way in.
  QImage source( 2400, 1200, QImage::Format_ARGB32 );
  source.fill( Qt::darkCyan );
  QByteArray png;
  QBuffer buffer( &png );
  QVERIFY( buffer.open( QIODevice::WriteOnly ) );
  QVERIFY( source.save( &buffer, "PNG" ) );
  buffer.close();

  const QString html = QStringLiteral( "<p>before</p><p><img src=\"data:image/17.PNG;base64,%1\" /></p><p>after</p>" ).arg( QString::fromLatin1( png.toBase64() ) );
  const QString stored = KadasAttachmentUtils::materializeInlineImages( html );

  QVERIFY2( !stored.contains( QStringLiteral( "data:image" ) ), "inline image must not survive" );
  QVERIFY( stored.contains( QStringLiteral( "before" ) ) && stored.contains( QStringLiteral( "after" ) ) );

  static const QRegularExpression srcRe( QStringLiteral( "src=\"(attachment:///[^\"]+)\"" ) );
  const QRegularExpressionMatch match = srcRe.match( stored );
  QVERIFY2( match.hasMatch(), qPrintable( stored ) );

  // The stored file is capped on its longest edge, keeping enough resolution to
  // be worth opening at full size...
  const QString file = KadasAttachmentUtils::resolve( match.captured( 1 ) );
  QVERIFY( !file.isEmpty() );
  QCOMPARE( QImage( file ).size(), QSize( 1920, 960 ) );

  // ...while the markup asks for a size that fits the tooltip.
  static const QRegularExpression widthRe( QStringLiteral( "width=\"(\\d+)\"" ) );
  QCOMPARE( widthRe.match( stored ).captured( 1 ).toInt(), 280 );

  // Markup with nothing inline is handed back untouched.
  const QString plain = QStringLiteral( "<p>no images here</p>" );
  QCOMPARE( KadasAttachmentUtils::materializeInlineImages( plain ), plain );
}

void TestKadasAnnotationControllers::richText_insertedImagesAreShrunkToTheirDisplaySize()
{
  // QgsRichTextEditor drops an image in at its full pixel size, so a photo
  // arrives in the editor many times wider than the field it will be shown in.
  QImage source( 2400, 1200, QImage::Format_ARGB32 );
  source.fill( Qt::darkCyan );
  QByteArray png;
  QBuffer buffer( &png );
  QVERIFY( buffer.open( QIODevice::WriteOnly ) );
  QVERIFY( source.save( &buffer, "PNG" ) );
  buffer.close();
  const QString url = QStringLiteral( "data:image/17.PNG;base64,%1" ).arg( QString::fromLatin1( png.toBase64() ) );

  QTextDocument document;
  QTextCursor cursor( &document );
  QTextImageFormat format;
  format.setName( url );
  format.setWidth( source.width() );
  format.setHeight( source.height() );
  cursor.insertImage( format );

  QVERIFY( KadasAttachmentUtils::clampImageDisplaySize( &document ) );

  const QTextImageFormat clamped = document.begin().begin().fragment().charFormat().toImageFormat();
  QCOMPARE( clamped.width(), 280.0 );
  QCOMPARE( clamped.height(), 140.0 );
  // The image itself is untouched: only how large it is drawn changed.
  QCOMPARE( clamped.name(), url );

  // Already small enough, so nothing to do and no spurious undo entry.
  QVERIFY( !KadasAttachmentUtils::clampImageDisplaySize( &document ) );
}

void TestKadasAnnotationControllers::richText_portraitImagesAreCappedOnTheirLongestEdge()
{
  // Capping the width alone leaves a portrait photo taller than the tooltip it
  // is shown in, which does not grow to take it.
  QImage source( 1200, 2400, QImage::Format_ARGB32 );
  source.fill( Qt::darkMagenta );
  QByteArray png;
  QBuffer buffer( &png );
  QVERIFY( buffer.open( QIODevice::WriteOnly ) );
  QVERIFY( source.save( &buffer, "PNG" ) );
  buffer.close();
  const QString url = QStringLiteral( "data:image/17.PNG;base64,%1" ).arg( QString::fromLatin1( png.toBase64() ) );

  QTextDocument document;
  QTextCursor cursor( &document );
  QTextImageFormat format;
  format.setName( url );
  format.setWidth( source.width() );
  format.setHeight( source.height() );
  cursor.insertImage( format );

  QVERIFY( KadasAttachmentUtils::clampImageDisplaySize( &document ) );
  const QTextImageFormat clamped = document.begin().begin().fragment().charFormat().toImageFormat();
  QCOMPARE( clamped.height(), 280.0 );
  QCOMPARE( clamped.width(), 140.0 );

  // The same bound applies to what gets stored.
  const QString stored = KadasAttachmentUtils::materializeInlineImages( QStringLiteral( "<p><img src=\"%1\" /></p>" ).arg( url ) );
  static const QRegularExpression heightRe( QStringLiteral( "height=\"(\\d+)\"" ) );
  QCOMPARE( heightRe.match( stored ).captured( 1 ).toInt(), 280 );
}

void TestKadasAnnotationControllers::richText_adjacentIdenticalImagesBothSurvive()
{
  // Two identical images side by side share one character format, so Qt reports
  // them as a single text fragment two characters long. Rewriting that fragment
  // as one image would quietly drop the second.
  QImage source( 400, 400, QImage::Format_ARGB32 );
  source.fill( Qt::darkGreen );
  QByteArray png;
  QBuffer buffer( &png );
  QVERIFY( buffer.open( QIODevice::WriteOnly ) );
  QVERIFY( source.save( &buffer, "PNG" ) );
  buffer.close();
  const QString url = QStringLiteral( "data:image/17.PNG;base64,%1" ).arg( QString::fromLatin1( png.toBase64() ) );

  // Inserted the way the editor inserts them - parsing markup instead would
  // give two fragments, and miss this entirely.
  QTextDocument document;
  QTextCursor cursor( &document );
  QTextImageFormat format;
  format.setName( url );
  format.setWidth( source.width() );
  format.setHeight( source.height() );
  cursor.insertImage( format );
  cursor.insertImage( format );
  QCOMPARE( document.begin().begin().fragment().length(), 2 );

  QVERIFY( KadasAttachmentUtils::clampImageDisplaySize( &document ) );

  int images = 0;
  for ( QTextBlock block = document.begin(); block.isValid(); block = block.next() )
  {
    for ( QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it )
    {
      const QTextFragment fragment = it.fragment();
      if ( fragment.isValid() && fragment.charFormat().isImageFormat() )
        images += fragment.length();
    }
  }
  QCOMPARE( images, 2 );

  const QString stored = KadasAttachmentUtils::materializeInlineImages( document.toHtml() );
  QCOMPARE( stored.count( QStringLiteral( "<img" ), Qt::CaseInsensitive ), 2 );
  QVERIFY2( !stored.contains( QStringLiteral( "data:image" ) ), qPrintable( stored ) );
}

void TestKadasAnnotationControllers::richText_bareUrlKeepsBracketsItOpened()
{
  QTextDocument document;
  document.setHtml( QStringLiteral( "<p>see https://en.wikipedia.org/wiki/Foo_(bar) and (www.example.com) too</p>" ) );
  KadasRichTextDialog::linkifyBareUrls( &document );

  QStringList anchors;
  for ( QTextBlock block = document.begin(); block.isValid(); block = block.next() )
  {
    for ( QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it )
    {
      const QTextFragment fragment = it.fragment();
      if ( fragment.isValid() && fragment.charFormat().isAnchor() )
        anchors << fragment.charFormat().anchorHref();
    }
  }

  const QStringList expected {
    // A bracket the address opened itself belongs to it...
    QStringLiteral( "https://en.wikipedia.org/wiki/Foo_(bar)" ),
    // ...while one the sentence opened does not.
    QStringLiteral( "http://www.example.com" ),
  };
  QCOMPARE( anchors, expected );
}

void TestKadasAnnotationControllers::attachment_legacyIdentifierResolvesThroughResourceProvider()
{
  // Kadas 2.x wrote a bare "attachment:name"; only "attachment:///name" is the
  // spelling QgsProject resolves, and both reach the provider as image URLs.
  QImage source( 32, 32, QImage::Format_ARGB32 );
  source.fill( Qt::red );
  const QString identifier = KadasAttachmentUtils::attachImage( source, QStringLiteral( "png" ) );
  QVERIFY( identifier.startsWith( QStringLiteral( "attachment:///" ) ) );
  const QString legacy = QStringLiteral( "attachment:" ) + identifier.mid( QStringLiteral( "attachment:///" ).size() );

  QCOMPARE( KadasAttachmentUtils::canonicalIdentifier( legacy ), identifier );

  QTextDocument document;
  KadasAttachmentUtils::installResourceProvider( &document );
  for ( const QString &spelling : { identifier, legacy } )
  {
    const QVariant resource = document.resource( QTextDocument::ImageResource, QUrl( spelling ) );
    QVERIFY2( !qvariant_cast<QImage>( resource ).isNull(), qPrintable( spelling ) );
  }

  // The "?w=&h=" display size Kadas 2.x stored still scales, in either spelling.
  const QImage scaled = qvariant_cast<QImage>( document.resource( QTextDocument::ImageResource, QUrl( legacy + QStringLiteral( "?w=8&h=8" ) ) ) );
  QCOMPARE( scaled.size(), QSize( 8, 8 ) );
}

void TestKadasAnnotationControllers::richText_unformattedTextIsStoredAsPlainText()
{
  // Qt writes the document's default styling onto every paragraph, so a line
  // nobody formatted would otherwise be stored as ten times its own length in
  // markup - in every project, forever, once resaved.
  {
    KadasRichTextDialog dialog( QStringLiteral( "t" ), QStringLiteral( "typed plainly" ) );
    dialog.accept();
    QCOMPARE( dialog.html(), QStringLiteral( "typed plainly" ) );
  }

  // Formatting is not something to collapse away, though.
  {
    KadasRichTextDialog dialog( QStringLiteral( "t" ), QStringLiteral( "<p>Hello <b>there</b></p>" ) );
    dialog.accept();
    QVERIFY2( dialog.html().contains( QStringLiteral( "font-weight" ) ), qPrintable( dialog.html() ) );
  }

  // Nor is a link.
  {
    KadasRichTextDialog dialog( QStringLiteral( "t" ), QStringLiteral( "<p>see <a href=\"https://example.org\">x</a></p>" ) );
    dialog.accept();
    QVERIFY2( dialog.html().contains( QStringLiteral( "href=\"https://example.org\"" ) ), qPrintable( dialog.html() ) );
  }

  // An emptied field stores nothing at all, rather than a skeleton of markup.
  {
    KadasRichTextDialog dialog( QStringLiteral( "t" ), QString() );
    dialog.accept();
    QVERIFY( dialog.html().isEmpty() );
  }
}


// ----- MilX ---------------------------------------------------------------

void TestKadasAnnotationControllers::milx_boundingBox_coversRenderedGlyph()
{
  // The rendered MSS glyph is far bigger than the control point it hangs on, and
  // the scale-dependent bounding box is all QgsAnnotationLayer::itemsInBounds()
  // has to offer a click: too tight a box and clicking the symbol picks nothing,
  // so neither the editor nor the context menu can ever open on it.
  const QgsCoordinateReferenceSystem mapCrs( QStringLiteral( "EPSG:3857" ) );
  const QgsCoordinateReferenceSystem itemCrs( QStringLiteral( "EPSG:4326" ) );
  QgsMapSettings ms;
  ms.setDestinationCrs( mapCrs );
  ms.setExtent( QgsRectangle( 820000, 5930000, 840000, 5950000 ) );
  ms.setOutputSize( QSize( 1000, 1000 ) );
  ms.setOutputDpi( 96 );

  const QgsCoordinateTransform toMap( itemCrs, mapCrs, QgsCoordinateTransformContext() );
  const QgsPointXY anchor( 7.44, 46.95 );

  KadasMilxAnnotationItem item;
  item.setMssString( QStringLiteral( "<mss-symbol/>" ) );
  item.setPoints( { anchor } );

  const QPointF anchorScreen = ms.mapToPixel().transform( toMap.transform( anchor ) ).toQPointF();
  auto itemPosAtScreenOffset = [&]( int dx, int dy ) {
    const QgsPointXY mapPos = ms.mapToPixel().toMapCoordinates( QPoint( anchorScreen.x() + dx, anchorScreen.y() + dy ) );
    return toMap.transform( mapPos, Qgis::TransformDirection::Reverse );
  };

  QgsRenderContext context = QgsRenderContext::fromMapSettings( ms );

  // 40 px above the anchor is on the glyph of a single point symbol, yet way
  // outside the (zero-size) hull of its control points.
  const QgsPointXY onGlyph = itemPosAtScreenOffset( 0, -40 );
  QVERIFY( !item.boundingBox().contains( onGlyph ) );
  QVERIFY( item.boundingBox( context ).contains( onGlyph ) );

  // Dragging the symbol away from its anchor leaves a leader line behind: the
  // box has to follow the glyph, not stay on the anchor.
  item.setUserOffset( QPoint( 0, -400 ) );
  QVERIFY( item.boundingBox( context ).contains( itemPosAtScreenOffset( 0, -400 ) ) );
}

void TestKadasAnnotationControllers::milx_rotationTransform_turnsAboutThePivot()
{
  KadasMilxAnnotationItem item;
  item.setMssString( QStringLiteral( "<mss-symbol/>" ) );
  item.setPoints( { QgsPointXY( 7.44, 46.95 ) } );
  item.setRotation( 90.0 );

  // Screen space is y-down, so a clockwise-from-north bearing of 90 degrees
  // takes a point above the pivot to a point right of it.
  const QPoint pivot( 100, 100 );
  const QPoint above( 100, 60 );
  QCOMPARE( item.rotationTransform( pivot ).map( above ), QPoint( 140, 100 ) );

  // An unrotated symbol must not pay for a transform at all.
  item.setRotation( 0.0 );
  QVERIFY( item.rotationTransform( pivot ).isIdentity() );
}

void TestKadasAnnotationControllers::milx_rotationHandle_rotatesSinglePointSymbol()
{
  // Same rotation UX as every other annotation: a knob north of the symbol,
  // grabbed through getEditContext() and dragged through edit().
  const QgsCoordinateReferenceSystem mapCrs( QStringLiteral( "EPSG:3857" ) );
  QgsMapSettings ms;
  ms.setDestinationCrs( mapCrs );
  ms.setExtent( QgsRectangle( 820000, 5930000, 840000, 5950000 ) );
  ms.setOutputSize( QSize( 1000, 1000 ) );
  ms.setOutputDpi( 96 );
  QgsAnnotationLayer layer( QStringLiteral( "mss" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  layer.setCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:4326" ) ) );
  const KadasAnnotationItemContext ctx( &layer, ms );

  KadasMilxAnnotationItem item;
  item.setMssString( QStringLiteral( "<mss-symbol/>" ) );
  const QgsPointXY anchor( 7.44, 46.95 );
  item.setPoints( { anchor } );

  // The symbol hangs off a screen pixel, so the pivot the controller rotates
  // about is that pixel mapped back - not the unsnapped anchor.
  const QgsPointXY pivot = ms.mapToPixel().toMapCoordinates( item.pivot( ms ) );

  // The knob clears the glyph: half the symbol size plus the knob radius and a
  // gap, never closer than the shared rest offset.
  const double mupp = ms.mapUnitsPerPixel();
  const double offPx = std::max( KadasAnnotationRotation::sHandleOffsetPixels, 0.5 * KadasMilxSymbolSettings::DefaultSymbolSize + KadasAnnotationRotation::sHandleRadiusPixels + 6.0 );
  const QgsPointXY handle( pivot.x(), pivot.y() + offPx * mupp );

  KadasMilxAnnotationController controller;
  const KadasEditContext editContext = controller.getEditContext( &item, handle, ctx );
  QVERIFY( editContext.vidx.isValid() );
  QCOMPARE( editContext.attributes.size(), 1 );

  // Drag the knob due east: a quarter turn clockwise from north.
  controller.edit( &item, editContext, QgsPointXY( pivot.x() + offPx * mupp, pivot.y() ), ctx );
  QCOMPARE( item.rotation(), 90.0 );
  // The anchor itself must not move - only the graphic turns.
  QCOMPARE( item.points().size(), 1 );
  QCOMPARE( item.points().front().x(), anchor.x() );

  // The numeric angle field reports and applies the same rotation.
  const KadasAttribValues reported = controller.editAttribsFromPosition( &item, editContext, QgsPointXY( pivot.x(), pivot.y() - offPx * mupp ), ctx );
  QCOMPARE( reported.size(), 1 );
  QCOMPARE( reported.first(), 180.0 );
  controller.edit( &item, editContext, reported, ctx );
  QCOMPARE( item.rotation(), 180.0 );
}

QTEST_MAIN( TestKadasAnnotationControllers )
#include "testkadasannotationcontrollers.moc"
