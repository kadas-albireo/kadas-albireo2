/***************************************************************************
    testkadasguidegridutils.cpp
    ---------------------------
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

#include <kadas/core/kadasguidegridutils.h>

class TestKadasGuideGridUtils : public QObject
{
    Q_OBJECT

  private slots:
    void gridLabel_data();
    void gridLabel();
    void avoidRepeatingLettersSkipsOnlyRepeatingLabels();
};

void TestKadasGuideGridUtils::gridLabel_data()
{
  QTest::addColumn<QString>( "start" );
  QTest::addColumn<int>( "offset" );
  QTest::addColumn<bool>( "avoidRepeatingLetters" );
  QTest::addColumn<QString>( "label" );

  QTest::newRow( "number" ) << "1" << 0 << false << "1";
  QTest::newRow( "number offset" ) << "1" << 9 << false << "10";
  QTest::newRow( "multi digit number" ) << "100" << 5 << false << "105";
  QTest::newRow( "number ignores repeating letters" ) << "1" << 10 << true << "11";

  QTest::newRow( "letter" ) << "A" << 0 << false << "A";
  QTest::newRow( "last single letter" ) << "A" << 25 << false << "Z";
  QTest::newRow( "two letters" ) << "A" << 26 << false << "AA";
  QTest::newRow( "two letters next" ) << "A" << 52 << false << "BA";
  QTest::newRow( "last two letters" ) << "A" << 701 << false << "ZZ";
  QTest::newRow( "three letters" ) << "A" << 702 << false << "AAA";
  QTest::newRow( "single letter start" ) << "Z" << 1 << false << "AA";
  QTest::newRow( "two letters start" ) << "BA" << 1 << false << "BB";
  QTest::newRow( "lowercase start" ) << "ab" << 0 << false << "AB";

  QTest::newRow( "skip AA" ) << "A" << 26 << true << "AB";
  QTest::newRow( "skip AA and BB" ) << "A" << 52 << true << "BC";
  QTest::newRow( "skip repeating start" ) << "AA" << 0 << true << "AB";
  QTest::newRow( "skip repeating start and next" ) << "ZZ" << 0 << true << "AAB";
  QTest::newRow( "skip after start" ) << "BA" << 1 << true << "BC";
}

void TestKadasGuideGridUtils::gridLabel()
{
  QFETCH( QString, start );
  QFETCH( int, offset );
  QFETCH( bool, avoidRepeatingLetters );
  QFETCH( QString, label );

  QCOMPARE( KadasGuideGridUtils::gridLabel( start, offset, avoidRepeatingLetters ), label );
}

void TestKadasGuideGridUtils::avoidRepeatingLettersSkipsOnlyRepeatingLabels()
{
  for ( const QString &start : { QStringLiteral( "A" ), QStringLiteral( "Z" ), QStringLiteral( "AA" ), QStringLiteral( "ZY" ) } )
  {
    QStringList expected;
    for ( int offset = 0; expected.size() < 1500; ++offset )
    {
      const QString label = KadasGuideGridUtils::gridLabel( start, offset );
      if ( label.size() == 1 || label.count( label.at( 0 ) ) != label.size() )
      {
        expected << label;
      }
    }
    for ( int offset = 0; offset < expected.size(); ++offset )
    {
      QCOMPARE( KadasGuideGridUtils::gridLabel( start, offset, true ), expected.at( offset ) );
    }
  }
}

QTEST_MAIN( TestKadasGuideGridUtils )
#include "testkadasguidegridutils.moc"
