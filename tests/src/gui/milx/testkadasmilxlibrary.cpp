/***************************************************************************
    testkadasmilxlibrary.cpp
    ------------------------
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

#include <QDomDocument>
#include <QtTest/QTest>

#include <kadas/gui/milx/kadasmilxlibrary.h>

/**
 * Tests for reading MSS gallery files.
 *
 * The names of a gallery and of its sections are what turns the symbol library
 * into a tree: a group whose name comes out empty collapses into its parent, so
 * the whole library degenerates into one flat list of symbols.
 */
class TestKadasMilxLibrary : public QObject
{
    Q_OBJECT

  private slots:
    void localizedName_readsTranslDescGalleries();
    void localizedName_readsLegacyNameElements();
    void localizedName_fallsBackToEnglish();
    void localizedName_treatsBlankTranslationAsMissing();
    void localizedName_hasNoNameToRead();

  private:
    static QDomElement parse( const QString &xml, QDomDocument &doc );
};

QDomElement TestKadasMilxLibrary::parse( const QString &xml, QDomDocument &doc )
{
  doc.setContent( xml );
  return doc.documentElement();
}

void TestKadasMilxLibrary::localizedName_readsTranslDescGalleries()
{
  // The schema MSS uses from the 2026 library on.
  QDomDocument doc;
  const QDomElement el = parse(
    QStringLiteral( R"(<Section>
      <TranslDesc LangId="EN" Desc="Existing Situation"/>
      <TranslDesc LangId="DE" Desc="Bestehende Situation"/>
      <TranslDesc LangId="PT_BR" Desc="Situação Existente"/>
    </Section>)" ),
    doc
  );

  QCOMPARE( KadasMilxLibrary::localizedName( el, QStringLiteral( "DE" ) ), QStringLiteral( "Bestehende Situation" ) );
  QCOMPARE( KadasMilxLibrary::localizedName( el, QStringLiteral( "EN" ) ), QStringLiteral( "Existing Situation" ) );
  // A regional variant answers for the plain language code.
  QCOMPARE( KadasMilxLibrary::localizedName( el, QStringLiteral( "PT" ) ), QStringLiteral( "Situação Existente" ) );
}

void TestKadasMilxLibrary::localizedName_readsLegacyNameElements()
{
  // Galleries shipped before the 2026 MSS library.
  QDomDocument doc;
  const QDomElement el = parse( QStringLiteral( "<Section><Name_EN>Existing Situation</Name_EN><Name_DE>Bestehende Situation</Name_DE></Section>" ), doc );

  QCOMPARE( KadasMilxLibrary::localizedName( el, QStringLiteral( "DE" ) ), QStringLiteral( "Bestehende Situation" ) );
  QCOMPARE( KadasMilxLibrary::localizedName( el, QStringLiteral( "EN" ) ), QStringLiteral( "Existing Situation" ) );
}

void TestKadasMilxLibrary::localizedName_fallsBackToEnglish()
{
  QDomDocument translDesc;
  QCOMPARE( KadasMilxLibrary::localizedName( parse( QStringLiteral( R"(<Section><TranslDesc LangId="EN" Desc="Existing Situation"/></Section>)" ), translDesc ), QStringLiteral( "IT" ) ), QStringLiteral( "Existing Situation" ) );

  QDomDocument legacy;
  QCOMPARE( KadasMilxLibrary::localizedName( parse( QStringLiteral( "<Section><Name_EN>Existing Situation</Name_EN></Section>" ), legacy ), QStringLiteral( "IT" ) ), QStringLiteral( "Existing Situation" ) );
}

void TestKadasMilxLibrary::localizedName_treatsBlankTranslationAsMissing()
{
  // The shipped galleries carry a few groups whose localized name is present
  // but empty - one subsection of Units.xml has had an empty <Name_DE/> since
  // 2.3. Honouring the blank verbatim would label that group with nothing.
  QDomDocument legacy;
  QCOMPARE( KadasMilxLibrary::localizedName( parse( QStringLiteral( "<SubSection><Name_DE></Name_DE><Name_EN>Flugzeug</Name_EN></SubSection>" ), legacy ), QStringLiteral( "DE" ) ), QStringLiteral( "Flugzeug" ) );

  QDomDocument translDesc;
  QCOMPARE( KadasMilxLibrary::localizedName( parse( QStringLiteral( R"(<SubSection><TranslDesc LangId="DE" Desc=""/><TranslDesc LangId="EN" Desc="Flugzeug"/></SubSection>)" ), translDesc ), QStringLiteral( "DE" ) ), QStringLiteral( "Flugzeug" ) );
}

void TestKadasMilxLibrary::localizedName_hasNoNameToRead()
{
  QDomDocument doc;
  QVERIFY( KadasMilxLibrary::localizedName( parse( QStringLiteral( "<Section><Member MssStringXML=\"x\"/></Section>" ), doc ), QStringLiteral( "DE" ) ).isEmpty() );
}

QTEST_MAIN( TestKadasMilxLibrary )
#include "testkadasmilxlibrary.moc"
