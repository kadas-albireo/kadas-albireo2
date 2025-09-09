
#include <QtTest/QTest>
#include <QSignalSpy>

#include <qgis/qgsfeedback.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgstest.h>

#include <kadas/gui/search/kadasalternategotolocatorfilter.h>

struct Result
{
    Result( QString displayString, const QgsPointXY &point, double scale = 0 )
      : displayString( displayString )
      , point( point )
      , scale( scale )
    {}

    QString displayString;
    QgsPointXY point;
    double scale = 0;
};

typedef QList<Result> Results;

Q_DECLARE_METATYPE( Results )


class TestKadasAlternateGotoLocatorFilter : public QObject
{
    Q_OBJECT

  private slots:

    void testGoto();
    void testGoto_data();

  private:
    QList<QgsLocatorResult> gatherResults( QgsLocatorFilter *filter, const QString &string, const QgsLocatorContext &context );
};

void TestKadasAlternateGotoLocatorFilter::testGoto()
{
  QFETCH( QString, string );
  QFETCH( Results, expected );

  QgsMapCanvas mapCanvas;
  mapCanvas.setDestinationCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ) );
  mapCanvas.mapSettings().setOutputDpi( 96 );
  mapCanvas.mapSettings().setOutputSize( QSize( 638, 478 ) );
  KadasAlternateGotoLocatorFilter filter( &mapCanvas );

  QList<QgsLocatorResult> results = gatherResults( &filter, string, QgsLocatorContext() );
  QCOMPARE( results.count(), expected.count() );

  for ( int i = 0; i < results.count(); i++ )
  {
    QCOMPARE( results.at( i ).displayString, expected.at( i ).displayString );
    QGSCOMPARENEARPOINT( results.at( i ).userData().toMap()[QStringLiteral( "point" )].value<QgsPointXY>(), expected.at( i ).point, 0.001 );
    if ( expected.at( i ).scale > 0 )
      QCOMPARE( results.at( 0 ).userData().toMap()[QStringLiteral( "scale" )].toDouble(), expected.at( i ).scale );
  }
}

void TestKadasAlternateGotoLocatorFilter::testGoto_data()
{
  QTest::addColumn<QString>( "string" );
  QTest::addColumn<Results>( "expected" );

  QTest::newRow( "simple" ) << QStringLiteral( "4 5" ) << Results( { { QObject::tr( "Go to 4°N 5°E" ), QgsPointXY( 5, 4 ) } } );

  QTest::newRow( "DMS-east-north" ) << QString::fromUtf8( "9°4′12.5″E, 47°2′2.2″N" ) << Results( { { QObject::tr( "Go to 47.03394444%1N 9.070138889%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( 9.0701388889, 47.03394444 ) } } );
  QTest::newRow( "DMS-east-south" ) << QString::fromUtf8( "38°13′56.7″E,14°20′58.4″S" ) << Results( { { QObject::tr( "Go to -14.34955556%1N 38.23241667%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( 38.23241667, -14.34955556 ) } } );
  QTest::newRow( "DMS-west-north" ) << QString::fromUtf8( "87°37′37.0″W,59°18′38.8″N" ) << Results( { { QObject::tr( "Go to 59.31077778%1N -87.62694444%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( -87.62694444, 59.31077778 ) } } );
  QTest::newRow( "DMS-west-south" ) << QString::fromUtf8( "68°17′27.7″W,37°5′24.9″S" ) << Results( { { QObject::tr( "Go to -37.09025%1N -68.29102778%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( -68.29102778, -37.09025 ) } } );

  QTest::newRow( "DM-east-north" ) << QString::fromUtf8( "16°36.680′E,50°20.727′N" ) << Results( { { QObject::tr( "Go to 50.34545%1N 16.61133333%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( 16.61133333, 50.34545 ) } } );
  QTest::newRow( "DM-east-south" ) << QString::fromUtf8( "76°33.164′E,17°23.555′S" ) << Results( { { QObject::tr( "Go to -17.39258333%1N 76.55273333%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( 76.55273333, -17.39258333 ) } } );
  QTest::newRow( "DM-west-north" ) << QString::fromUtf8( "120°40.429′W,60°48.124′N" ) << Results( { { QObject::tr( "Go to 60.80206667%1N -120.6738167%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( -120.6738167, 60.80206667 ) } } );
  QTest::newRow( "DM-west-south" ) << QString::fromUtf8( "72°51.679′W,38°45.245′S" ) << Results( { { QObject::tr( "Go to -38.75408333%1N -72.86131667%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( -72.86131667, -38.75408333 ) } } );

  QTest::newRow( "DD-east-north" ) << QString::fromUtf8( "7.64649°E,59.75639°N" ) << Results( { { QObject::tr( "Go to 59.75639%1N 7.64649%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( 7.64649, 59.75639 ) } } );
  QTest::newRow( "DD-east-south" ) << QString::fromUtf8( "24.52149°E,31.87756°S" ) << Results( { { QObject::tr( "Go to -31.87756%1N 24.52149%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( 24.52149, -31.87756 ) } } );
  QTest::newRow( "DD-west-north" ) << QString::fromUtf8( "100.98632°W,55.22902°N" ) << Results( { { QObject::tr( "Go to 55.22902%1N -100.98632%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( -100.98632, 55.22902 ) } } );
  QTest::newRow( "DD-west-south" ) << QString::fromUtf8( "65.83007°W,26.66710°S" ) << Results( { { QObject::tr( "Go to -26.6671%1N -65.83007%1E" ).arg( QString::fromUtf8( "°" ) ), QgsPointXY( -65.83007, -26.6671 ) } } );

  QTest::newRow( "UTM-33V" ) << QStringLiteral( "460102, 6422026 (zone 33V)" ) << Results( { { QObject::tr( "Go to 460102, 6422026 (zone 33V)" ), QgsPointXY( 14.3261663140, 57.9381762924 ) } } );
  QTest::newRow( "UTM-33K" ) << QStringLiteral( "754955, 7436525 (zone 33K)" ) << Results( { { QObject::tr( "Go to 754955, 7436525 (zone 33K)" ), QgsPointXY( 17.4902247957, -23.1605694189 ) } } );
  QTest::newRow( "UTM-12R" ) << QStringLiteral( "375484, 2792615 (zone 12R)" ) << Results( { { QObject::tr( "Go to 375484, 2792615 (zone 12R)" ), QgsPointXY( -112.2363331505, 25.2446866382 ) } } );

  QTest::newRow( "MGRS-Italia" ) << QStringLiteral( "32TPP 73642 62803" ) << Results( { { QObject::tr( "Go to 32TPP 73642 62803" ), QgsPointXY( 11.1620858580, 43.8978807883 ) } } );
  QTest::newRow( "MGRS-France" ) << QStringLiteral( "31TEN 91239 96251" ) << Results( { { QObject::tr( "Go to 31TEN 91239 96251" ), QgsPointXY( 4.2187310610, 47.8131408597 ) } } );
  QTest::newRow( "MGRS-Deutschland" ) << QStringLiteral( "32UNB 78237 14913" ) << Results( { { QObject::tr( "Go to 32UNB 78237 14913" ), QgsPointXY( 10.1074023522, 50.6807858612 ) } } );

  QTest::newRow( "osm.leaflet.OL" ) << QStringLiteral( "https://www.openstreetmap.org/#map=15/44.5546/6.4936" ) << Results( { { QObject::tr( "Go to 44.5546°N 6.4936°E at scale 1:22569" ), QgsPointXY( 6.4936, 44.5546 ), 22569.0 } } );
  QTest::newRow( "gmaps1" ) << QStringLiteral( "https://www.google.com/maps/@44.5546,6.4936,15.25z" ) << Results( { { QObject::tr( "Go to 44.5546°N 6.4936°E at scale 1:22569" ), QgsPointXY( 6.4936, 44.5546 ), 22569.0 } } );
  QTest::newRow( "gmaps2" ) << QStringLiteral( "https://www.google.com/maps/@7.931768,80.8731272,688963m/data=!3m1!1e3" ) << Results( { { QObject::tr( "Go to 7.931768°N 80.8731272°E at scale 1:6.48043e+07" ), QgsPointXY( 80.8731272, 7.931768 ) } } );
  QTest::newRow( "gmaps3" ) << QStringLiteral( "https://www.google.com/maps/@27.7132,85.3288,3a,75y,278.89h,90t/data=!3m8!1e1!3m6!1sAF1QipMrXuXozGc9x9bxx5uPl_3ys4H-rNVqMLr6EYLA!2e10!3e11!6shttps:%2F%2Flh5.googleusercontent.com%2Fp%2FAF1QipMrXuXozGc9x9bxx5uPl_3ys4H-rNVqMLr6EYLA%3Dw203-h100-k-no-pi2.869903-ya293.58762-ro-1.9255565-fo100!7i3840!8i1920" ) << Results( { { QObject::tr( "Go to 27.7132°N 85.3288°E at scale 1:282" ), QgsPointXY( 85.3288, 27.7132 ), 282.0 } } );
}

QList<QgsLocatorResult> TestKadasAlternateGotoLocatorFilter::gatherResults( QgsLocatorFilter *filter, const QString &string, const QgsLocatorContext &context )
{
  const QSignalSpy spy( filter, &QgsLocatorFilter::resultFetched );
  QgsFeedback f;
  filter->prepare( string, context );
  filter->fetchResults( string, context, &f );

  QList<QgsLocatorResult> results;
  for ( int i = 0; i < spy.count(); ++i )
  {
    const QVariant v = spy.at( i ).at( 0 );
    const QgsLocatorResult result = v.value<QgsLocatorResult>();
    results.append( result );
  }
  return results;
}

QTEST_MAIN( TestKadasAlternateGotoLocatorFilter )
#include "testkadasalternategotolocatorfilter.moc"
