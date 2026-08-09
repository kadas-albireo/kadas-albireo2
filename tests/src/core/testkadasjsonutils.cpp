/***************************************************************************
    testkadasjsonutils.cpp
    ----------------------
    copyright            : (C) 2026 by Damiano Lombardi
    email                : damiano at opengis dot ch
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

#include <QJsonDocument>

#include <kadas/core/kadasjsonutils.h>

class TestKadasJsonUtils : public QObject
{
    Q_OBJECT

  private slots:
    void extractsStringValueFromNestedArrayPath();
    void rejectsMalformedOrNonMatchingPaths();
};

void TestKadasJsonUtils::extractsStringValueFromNestedArrayPath()
{
  const QJsonDocument doc = QJsonDocument::fromJson( QByteArrayLiteral( "{\"features\":[{\"attributes\":{\"client_id\":\"abc123\"}}]}" ) );

  QCOMPARE( KadasJsonUtils::jsonPathToString( doc, QStringLiteral( "$.features[0].attributes.client_id" ) ), QStringLiteral( "abc123" ) );
}

void TestKadasJsonUtils::rejectsMalformedOrNonMatchingPaths()
{
  const QJsonDocument doc = QJsonDocument::fromJson( QByteArrayLiteral( "{\"features\":[{\"attributes\":{\"client_id\":\"abc123\",\"count\":7}}]}" ) );

  QCOMPARE( KadasJsonUtils::jsonPathToString( doc, QStringLiteral( "features[0].attributes.client_id" ) ), QString() );
  QCOMPARE( KadasJsonUtils::jsonPathToString( doc, QStringLiteral( "$.features[1].attributes.client_id" ) ), QString() );
  QCOMPARE( KadasJsonUtils::jsonPathToString( doc, QStringLiteral( "$.features[0].attributes.missing" ) ), QString() );
  QCOMPARE( KadasJsonUtils::jsonPathToString( doc, QStringLiteral( "$.features[0].attributes.count" ) ), QString() );
  QCOMPARE( KadasJsonUtils::jsonPathToString( doc, QStringLiteral( "$.features[0].attributes.client_id.extra" ) ), QString() );
}

QTEST_MAIN( TestKadasJsonUtils )
#include "testkadasjsonutils.moc"