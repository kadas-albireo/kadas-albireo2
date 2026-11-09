/***************************************************************************
    testkadaslayerutils.cpp
    -----------------------
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

#include <QSignalSpy>
#include <QtTest/QTest>

#include <qgis/qgsannotationlayer.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadaslayerutils.h>

class TestKadasLayerUtils : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
    void locksAnnotationLayer();
    void locksVectorLayer();
    void reportsLockChange();
    void readsLockFromBeforeTheProperty();
};

void TestKadasLayerUtils::initTestCase()
{
  QgsApplication::init();
  QgsApplication::initQgis();
}

void TestKadasLayerUtils::cleanupTestCase()
{
  QgsApplication::exitQgis();
}

void TestKadasLayerUtils::locksAnnotationLayer()
{
  QgsAnnotationLayer layer( QStringLiteral( "Annotation" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  QVERIFY( !KadasLayerUtils::isLocked( &layer ) );

  KadasLayerUtils::setLocked( &layer, true );
  QVERIFY( KadasLayerUtils::isLocked( &layer ) );
  QVERIFY( !layer.flags().testFlag( QgsMapLayer::Removable ) );

  KadasLayerUtils::setLocked( &layer, false );
  QVERIFY( !KadasLayerUtils::isLocked( &layer ) );
  QVERIFY( layer.flags().testFlag( QgsMapLayer::Removable ) );

  QVERIFY( !KadasLayerUtils::isLocked( nullptr ) );
}

void TestKadasLayerUtils::locksVectorLayer()
{
  QgsVectorLayer layer( QStringLiteral( "Point?crs=EPSG:4326" ), QStringLiteral( "Points" ), QStringLiteral( "memory" ) );
  QVERIFY( layer.isValid() );

  KadasLayerUtils::setLocked( &layer, true );
  QVERIFY( KadasLayerUtils::isLocked( &layer ) );
  QVERIFY( layer.readOnly() );
  QVERIFY( !layer.flags().testFlag( QgsMapLayer::Removable ) );

  KadasLayerUtils::setLocked( &layer, false );
  QVERIFY( !KadasLayerUtils::isLocked( &layer ) );
  QVERIFY( !layer.readOnly() );
  QVERIFY( layer.flags().testFlag( QgsMapLayer::Removable ) );
}

void TestKadasLayerUtils::reportsLockChange()
{
  // Tools and layer choosers follow the lock through this signal.
  QgsAnnotationLayer layer( QStringLiteral( "Annotation" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  QSignalSpy spy( &layer, &QgsMapLayer::customPropertyChanged );

  KadasLayerUtils::setLocked( &layer, true );
  QCOMPARE( spy.count(), 1 );
  QCOMPARE( spy.at( 0 ).at( 0 ).toString(), KadasLayerUtils::lockedPropertyKey() );
}

void TestKadasLayerUtils::readsLockFromBeforeTheProperty()
{
  // Locking used to only make a layer non-removable and, for a vector layer, read-only.
  QgsAnnotationLayer annotationLayer( QStringLiteral( "Annotation" ), QgsAnnotationLayer::LayerOptions( QgsCoordinateTransformContext() ) );
  annotationLayer.setFlags( annotationLayer.flags() & ~QgsMapLayer::Removable );
  QVERIFY( KadasLayerUtils::isLocked( &annotationLayer ) );

  QgsVectorLayer vectorLayer( QStringLiteral( "Point?crs=EPSG:4326" ), QStringLiteral( "Points" ), QStringLiteral( "memory" ) );
  vectorLayer.setFlags( vectorLayer.flags() & ~QgsMapLayer::Removable );
  QVERIFY( !KadasLayerUtils::isLocked( &vectorLayer ) );
  vectorLayer.setReadOnly( true );
  QVERIFY( KadasLayerUtils::isLocked( &vectorLayer ) );

  // Once there, the property has the final say.
  annotationLayer.setCustomProperty( KadasLayerUtils::lockedPropertyKey(), false );
  QVERIFY( !KadasLayerUtils::isLocked( &annotationLayer ) );
}

QTEST_MAIN( TestKadasLayerUtils )
#include "testkadaslayerutils.moc"
