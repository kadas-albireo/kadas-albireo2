
#include <QtTest/QTest>
#include <QSignalSpy>

#include <qgis/qgsfeedback.h>
#include <qgis/qgsmapcanvas.h>

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
  KadasAlternateGotoLocatorFilter filter( &mapCanvas );

  QList<QgsLocatorResult> results = gatherResults( &filter, string, QgsLocatorContext() );
  QCOMPARE( results.count(), expected.count() );

  for ( int i = 0; i < results.count(); i++ )
  {
    QCOMPARE( results.at( i ).displayString, expected.at( i ).displayString );
    QCOMPARE( results.at( i ).userData().toMap()[QStringLiteral( "point" )].value<QgsPointXY>(), expected.at( i ).point );
    if ( expected.at( i ).scale > 0 )
      QCOMPARE( results.at( 0 ).userData().toMap()[QStringLiteral( "scale" )].toDouble(), expected.at( i ).scale );
  }
}

void TestKadasAlternateGotoLocatorFilter::testGoto_data()
{
  QTest::addColumn<QString>( "string" );
  QTest::addColumn<Results>( "expected" );

  QTest::newRow( "simple" ) << QStringLiteral( "4 5" ) << Results( { { QObject::tr( "Go to 4°N 5°E" ), QgsPointXY( 5, 4 ) } } );

  QTest::newRow( "locale" ) << QStringLiteral( "1,234.56 789.012" ) << Results( { { QObject::tr( "Go to 1,234.56 789.012" ), QgsPointXY( 1234.56, 789.012 ) } } );

  QTest::newRow( "nort-west" ) << QStringLiteral( "12.345N, 67.890W" ) << Results( { { QObject::tr( "Go to 12.345°N -67.89°E" ), QgsPointXY( -67.890, 12.345 ) } } );
  QTest::newRow( "east-south" ) << QStringLiteral( "12.345 e, 67.890 s" ) << Results( { { QObject::tr( "Go to -67.89°N 12.345°E" ), QgsPointXY( 12.345, -67.890 ) } } );
  QTest::newRow( "degree-suffix" ) << QStringLiteral( "40deg 1' 0\" E 11deg  55' 0\" S" ) << Results( { { QObject::tr( "Go to -11.91666667°N 40.01666667°E" ), QgsPointXY( 40.0166666667, -11.9166666667 ) } } );
  QTest::newRow( "north-east------------" ) << QStringLiteral( "14°49′48″N 01°48′45″E" ) << Results( { { QObject::tr( "Go to 14.83°N 1.8125°E" ), QgsPointXY( 1.8125, 14.83 ) } } );
  QTest::newRow( "north-east-space------" ) << QStringLiteral( "14°49′48″ N 01°48′45″ E" ) << Results( { { QObject::tr( "Go to 14.83°N 1.8125°E" ), QgsPointXY( 1.8125, 14.83 ) } } );
  QTest::newRow( "north-east-comma------" ) << QStringLiteral( "14°49′48″N, 01°48′45″E" ) << Results( { { QObject::tr( "Go to 14.83°N 1.8125°E" ), QgsPointXY( 1.8125, 14.83 ) } } );
  QTest::newRow( "north-east-comma-space" ) << QStringLiteral( "14°49′48″ N, 01°48′45″ E" ) << Results( { { QObject::tr( "Go to 14.83°N 1.8125°E" ), QgsPointXY( 1.8125, 14.83 ) } } );
  QTest::newRow( "north-east-front------" ) << QStringLiteral( "N 14°49′48″ E 01°48′45″" ) << Results( { { QObject::tr( "Go to 14.83°N 1.8125°E" ), QgsPointXY( 1.8125, 14.83 ) } } );
  QTest::newRow( "north-east-front-comma" ) << QStringLiteral( "N 14°49′48″, E 01°48′45″" ) << Results( { { QObject::tr( "Go to 14.83°N 1.8125°E" ), QgsPointXY( 1.8125, 14.83 ) } } );
  QTest::newRow( "osm.leaflet.OL" ) << QStringLiteral( "https://www.openstreetmap.org/#map=15/44.5546/6.4936" ) << Results( { { QObject::tr( "Go to 44.5546°N 6.4936°E at scale 1:22569" ), QgsPointXY( 6.4936, 44.5546 ), 22569.0 } } );
  QTest::newRow( "gmaps1" ) << QStringLiteral( "https://www.google.com/maps/@44.5546,6.4936,15.25z" ) << Results( { { QObject::tr( "Go to 44.5546°N 6.4936°E at scale 1:22569" ), QgsPointXY( 6.4936, 44.5546 ), 22569.0 } } );
  QTest::newRow( "gmaps2" ) << QStringLiteral( "https://www.google.com/maps/@7.8750,81.0149,574195m/data=!3m1!1e3" ) << Results( { { QObject::tr( "Go to 7.875°N 81.0149°E at scale 1:6.49572e+07" ), QgsPointXY( 81.0149, 7.8750 ) } } );
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
