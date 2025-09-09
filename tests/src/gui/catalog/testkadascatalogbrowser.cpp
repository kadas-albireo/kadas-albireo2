
#include <QtTest/QTest>

#include <qgis/qgstest.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/bullseye/kadasbullseyelayer.h>
#include <kadas/test/kadastest.h>

class TestKadasCatalogBrowser : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void cleanupTestCase();
};

void TestKadasCatalogBrowser::initTestCase()
{
  //int argc;
  //char **argv;
  // KadasApplication *app = new KadasApplication(argc, argv);

  KadasBullseyeLayer layer( "test" );

  QCoreApplication::setOrganizationName( QStringLiteral( "Kadas" ) );
  QCoreApplication::setOrganizationDomain( QStringLiteral( "kadas.org" ) );
  QCoreApplication::setApplicationName( QStringLiteral( "KADAS-TEST" ) );
}

void TestKadasCatalogBrowser::cleanupTestCase()
{
}

QGSTEST_MAIN( TestKadasCatalogBrowser )
#include "testkadascatalogbrowser.moc"
