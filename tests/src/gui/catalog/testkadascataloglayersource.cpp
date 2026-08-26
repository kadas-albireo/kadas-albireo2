/***************************************************************************
    testkadascataloglayersource.cpp
    -------------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QtTest/QTest>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsdatasourceuri.h>
#include <qgis/qgsmimedatautils.h>

#include <kadas/gui/kadascataloglayersource.h>

/**
 * Guards the source resolution shared by adding a catalog entry and previewing
 * it. Both go through KadasCatalogLayerSource, so a change here silently
 * changes which layer the catalog loads for every provider it supports.
 */
class TestKadasCatalogLayerSource : public QObject
{
    Q_OBJECT
  private slots:
    void resolvesProviderToLayerType_data();
    void resolvesProviderToLayerType();

    void narrowsWmsToSublayerName();
    void narrowsArcGisMapServerToSublayerId();
    void appendsSublayerIdToArcGisFeatureServerUrl();
    void keepsUriWithoutSublayer();

    void adjustsCrsToCanvas();
    void keepsUriWhenCanvasCrsUnsupported();

    void invalidWithoutUri();
    void unrecognizedProviderFallsBackToRaster();

  private:
    static QgsMimeDataUtils::Uri entry( const QString &providerKey, const QString &uri );
};

QgsMimeDataUtils::Uri TestKadasCatalogLayerSource::entry( const QString &providerKey, const QString &uri )
{
  QgsMimeDataUtils::Uri result;
  result.providerKey = providerKey;
  result.uri = uri;
  result.name = QStringLiteral( "Entry" );
  return result;
}

void TestKadasCatalogLayerSource::resolvesProviderToLayerType_data()
{
  QTest::addColumn<QString>( "providerKey" );
  QTest::addColumn<int>( "type" );

  QTest::newRow( "wms" ) << QStringLiteral( "wms" ) << static_cast<int>( KadasCatalogLayerSource::Type::Raster );
  QTest::newRow( "arcgismapserver" ) << QStringLiteral( "arcgismapserver" ) << static_cast<int>( KadasCatalogLayerSource::Type::Raster );
  QTest::newRow( "arcgisfeatureserver" ) << QStringLiteral( "arcgisfeatureserver" ) << static_cast<int>( KadasCatalogLayerSource::Type::Vector );
  QTest::newRow( "arcgisvectortileservice" ) << QStringLiteral( "arcgisvectortileservice" ) << static_cast<int>( KadasCatalogLayerSource::Type::VectorTile );
  // Anything else is loaded through its provider as a raster layer.
  QTest::newRow( "gdal" ) << QStringLiteral( "gdal" ) << static_cast<int>( KadasCatalogLayerSource::Type::Raster );
}

void TestKadasCatalogLayerSource::resolvesProviderToLayerType()
{
  QFETCH( QString, providerKey );
  QFETCH( int, type );

  const QgsMimeDataUtils::Uri uri = entry( providerKey, QStringLiteral( "url=https://example.com/service" ) );
  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( uri.providerKey, uri.uri, QVariant(), uri.name );

  QCOMPARE( static_cast<int>( source.type ), type );
  QCOMPARE( source.providerKey, providerKey );
  QCOMPARE( source.name, QStringLiteral( "Entry" ) );
  QVERIFY( source.isValid() );
}

void TestKadasCatalogLayerSource::narrowsWmsToSublayerName()
{
  // WMS sublayer ids are layer names, not numbers, so they have to survive as
  // strings.
  const QgsMimeDataUtils::Uri uri = entry( QStringLiteral( "wms" ), QStringLiteral( "crs=EPSG:2056&format=image/png&layers=all&url=https://example.com/wms" ) );

  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( uri.providerKey, uri.uri, QStringLiteral( "ch.swisstopo.pixelkarte" ), uri.name );

  QCOMPARE( source.uri, QStringLiteral( "crs=EPSG:2056&format=image/png&layers=ch.swisstopo.pixelkarte&url=https://example.com/wms" ) );
}

void TestKadasCatalogLayerSource::narrowsArcGisMapServerToSublayerId()
{
  const QgsMimeDataUtils::Uri uri = entry( QStringLiteral( "arcgismapserver" ), QStringLiteral( "crs='EPSG:2056' layer='0' url='https://example.com/MapServer'" ) );

  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( uri.providerKey, uri.uri, 7, uri.name );

  QgsDataSourceUri resolved( source.uri );
  QCOMPARE( resolved.param( QStringLiteral( "layer" ) ), QStringLiteral( "7" ) );
  QCOMPARE( resolved.param( QStringLiteral( "url" ) ), QStringLiteral( "https://example.com/MapServer" ) );
}

void TestKadasCatalogLayerSource::appendsSublayerIdToArcGisFeatureServerUrl()
{
  const QgsMimeDataUtils::Uri uri = entry( QStringLiteral( "arcgisfeatureserver" ), QStringLiteral( "crs='EPSG:2056' url='https://example.com/FeatureServer'" ) );

  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( uri.providerKey, uri.uri, 3, uri.name );

  QgsDataSourceUri resolved( source.uri );
  QCOMPARE( resolved.param( QStringLiteral( "url" ) ), QStringLiteral( "https://example.com/FeatureServer/3" ) );
}

void TestKadasCatalogLayerSource::keepsUriWithoutSublayer()
{
  const QString raw = QStringLiteral( "crs=EPSG:2056&format=image/png&layers=all&url=https://example.com/wms" );
  const QgsMimeDataUtils::Uri uri = entry( QStringLiteral( "wms" ), raw );

  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( uri.providerKey, uri.uri, QVariant(), uri.name );

  QCOMPARE( source.uri, raw );
}

void TestKadasCatalogLayerSource::adjustsCrsToCanvas()
{
  QgsMimeDataUtils::Uri uri = entry( QStringLiteral( "wms" ), QStringLiteral( "crs=EPSG:4326&layers=all&url=https://example.com/wms" ) );
  uri.supportedCrs = QStringList() << QStringLiteral( "EPSG:4326" ) << QStringLiteral( "EPSG:2056" );

  const QString adjusted = KadasCatalogLayerSource::adjustUri( uri, QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:2056" ) ) );

  QCOMPARE( adjusted, QStringLiteral( "crs=EPSG:2056&layers=all&url=https://example.com/wms" ) );
}

void TestKadasCatalogLayerSource::keepsUriWhenCanvasCrsUnsupported()
{
  QgsMimeDataUtils::Uri uri = entry( QStringLiteral( "wms" ), QStringLiteral( "crs=EPSG:4326&layers=all&url=https://example.com/wms" ) );
  uri.supportedCrs = QStringList() << QStringLiteral( "EPSG:4326" );

  const QString adjusted = KadasCatalogLayerSource::adjustUri( uri, QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:2056" ) ) );

  QCOMPARE( adjusted, uri.uri );
}

void TestKadasCatalogLayerSource::invalidWithoutUri()
{
  // A group row in the catalog carries no uri, and it is the missing uri that
  // makes the source invalid -- an unknown provider key on its own does not,
  // see unrecognizedProviderFallsBackToRaster().
  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( QString(), QString(), QVariant(), QString() );

  QVERIFY( !source.isValid() );
}

void TestKadasCatalogLayerSource::unrecognizedProviderFallsBackToRaster()
{
  // resolve() never reports Unknown: a provider it does not know is loaded
  // through that provider as a raster layer.
  const KadasCatalogLayerSource source = KadasCatalogLayerSource::resolve( QStringLiteral( "brand-new-provider" ), QStringLiteral( "url=https://example.com" ), QVariant(), QStringLiteral( "Entry" ) );

  QCOMPARE( source.type, KadasCatalogLayerSource::Type::Raster );
  QVERIFY( source.isValid() );
}

QTEST_MAIN( TestKadasCatalogLayerSource )
#include "testkadascataloglayersource.moc"
