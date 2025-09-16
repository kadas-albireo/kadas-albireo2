
#include <QtTest/QTest>

#include <qgis/qgslayertreemodel.h>
#include <qgis/qgslayertreelayer.h>
#include <qgis/qgslayertree.h>
#include <qgis/qgsmaplayer.h>

#include <kadas/test/kadastest.h>
#include <kadas/app/kadasmainwindow.h>

class TestKadasCatalogBrowser : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAddCatalogLayer();
    void testAddCatalogLayer_data();
};

void TestKadasCatalogBrowser::initTestCase()
{
  QCoreApplication::setOrganizationName( QStringLiteral( "Kadas" ) );
  QCoreApplication::setOrganizationDomain( QStringLiteral( "kadas.org" ) );
  QCoreApplication::setApplicationName( QStringLiteral( "KADAS-TEST" ) );
}

void TestKadasCatalogBrowser::cleanupTestCase()
{
  KadasApplication *app = KadasApplication::instance();
  KadasMainWindow *mainWindow = app->mainWindow();
  QgsLayerTree *layerTree = mainWindow->layerTreeView()->layerTreeModel()->rootGroup();
  layerTree->removeAllChildren();
}

void TestKadasCatalogBrowser::testAddCatalogLayer()
{
  QFETCH( QString, uri );

  KadasApplication *app = KadasApplication::instance();
  KadasMainWindow *mainWindow = app->mainWindow();
  QgsLayerTree *layerTree = mainWindow->layerTreeView()->layerTreeModel()->rootGroup();
  layerTree->removeAllChildren();

  QVERIFY(QMetaObject::invokeMethod(mainWindow,
                                    "addCatalogLayer",
                                    Qt::DirectConnection,
                                    Q_ARG(QgsMimeDataUtils::Uri, QgsMimeDataUtils::Uri(uri)),
                                    Q_ARG(QString, ""),
                                    Q_ARG(QVariantList, QVariantList())));

  QCOMPARE( layerTree->findLayers().size(), 1 );

  QgsLayerTreeLayer *firstLayerNode = dynamic_cast<QgsLayerTreeLayer *>( layerTree->children().at( 0 ) );
  QVERIFY( firstLayerNode );
  
  QgsMapLayer *layer = firstLayerNode->layer();
  QVERIFY( layer );
  QVERIFY( layer->isValid() );
}

void TestKadasCatalogBrowser::testAddCatalogLayer_data()
{
  QTest::addColumn<QString>( "uri" );

  QTest::newRow( "Apotheken" ) << QStringLiteral( "vector:arcgisfeatureserver:Apotheken:crs='EPSG\\:2056' url='https\\://geoinfo.op.intra2.admin.ch/arcgis/rest/services/O/ChPharmasuisseApotheken/MapServer'" );
  QTest::newRow( "Armeeapotheke" ) << QStringLiteral( "vector:arcgisfeatureserver:Armeeapotheke:crs='EPSG\\:2056' url='https\\://geoinfo.op.intra2.admin.ch/arcgis/rest/services/R4/ChVbsArmeeapotheke/MapServer'" );
  QTest::newRow( "Administrative Grenzen" ) << QStringLiteral( "vector:arcgisfeatureserver:Administrative Grenzen / Gemeindegrenzen:crs='EPSG\\:2056' url='https\\://geoinfo.op.intra2.admin.ch/arcgis/rest/services/D/ChSwisstopoSwissboundaries3dGemeindeFlaecheFill/MapServer'" );
  QTest::newRow( "Amtliches Strassenverzeichnis" ) << QStringLiteral( "vector:arcgisfeatureserver:Amtliches Strassenverzeichnis:crs='EPSG\\:2056' url='https\\://geoinfo.op.intra2.admin.ch/arcgis/rest/services/B/ChSwisstopoAmtlichesStrassenverzeichnis/MapServer'" );
}

KADASTEST_MAIN( TestKadasCatalogBrowser )
#include "testkadascatalogbrowser.moc"
