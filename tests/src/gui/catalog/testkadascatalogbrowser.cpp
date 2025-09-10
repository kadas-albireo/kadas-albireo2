
#include <QtTest/QTest>

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
  QCoreApplication::setOrganizationName( QStringLiteral( "Kadas" ) );
  QCoreApplication::setOrganizationDomain( QStringLiteral( "kadas.org" ) );
  QCoreApplication::setApplicationName( QStringLiteral( "KADAS-TEST" ) );
}

void TestKadasCatalogBrowser::cleanupTestCase()
{
}

KADASTEST_MAIN( TestKadasCatalogBrowser )
#include "testkadascatalogbrowser.moc"
