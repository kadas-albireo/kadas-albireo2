/***************************************************************************
    testkadasmapitemtooltip.cpp
    ---------------------------
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

#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QTextDocument>
#include <QUrl>
#include <QtTest/QTest>

#include <qgis/qgsapplication.h>
#include <qgis/qgsmapcanvas.h>

#include <kadas/gui/kadasmapitemtooltip.h>

/**
 * Collects what the tooltip hands to QDesktopServices, so a test can tell an
 * opened link from one that went nowhere without a browser appearing.
 */
class UrlCatcher : public QObject
{
    Q_OBJECT
  public:
    QStringList urls;
  public slots:
    void handle( const QUrl &url ) { urls << url.toString(); }
};

class TestKadasMapItemTooltip : public QObject
{
    Q_OBJECT
  private slots:
    void initTestCase();
    void linkOpensOnClickThatBarelyMoved();
    void linkStaysShutWhenTheClickWasADrag();
    void linkStaysShutWhileAMapToolOwnsThePointer();
    void plainClickLeavesNoTextSelectionBehind();

  private:
    //! A point inside the anchor of the tooltip's one link, in viewport coordinates.
    static QPoint linkPos( const KadasMapItemTooltip &tooltip );
};

void TestKadasMapItemTooltip::initTestCase()
{
  QStandardPaths::setTestModeEnabled( true );
  QgsApplication::init();
}

QPoint TestKadasMapItemTooltip::linkPos( const KadasMapItemTooltip &tooltip )
{
  for ( int y = 0; y < tooltip.viewport()->height(); y += 2 )
  {
    for ( int x = 0; x < tooltip.viewport()->width(); x += 2 )
    {
      if ( !tooltip.document()->documentLayout()->anchorAt( QPointF( x, y ) ).isEmpty() )
        return QPoint( x, y );
    }
  }
  return QPoint();
}

void TestKadasMapItemTooltip::linkOpensOnClickThatBarelyMoved()
{
  // Regression: the release used to count as a click only when no mouse move at
  // all had arrived since the press. A trackpad and a touch screen both put a
  // pixel or two of travel into an ordinary click, so a link in a pin's
  // description read as a link — blue and underlined — and did nothing.
  QgsMapCanvas canvas;
  canvas.resize( 800, 600 );
  canvas.show();
  KadasMapItemTooltip tooltip( &canvas );
  tooltip.setHtml( QStringLiteral( "<p>see <a href=\"https://example.com/x\">https://example.com/x</a> for more</p>" ) );
  tooltip.move( 10, 10 );
  tooltip.show();
  QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );

  const QPoint pos = linkPos( tooltip );
  QVERIFY( !pos.isNull() );

  UrlCatcher catcher;
  QDesktopServices::setUrlHandler( QStringLiteral( "https" ), &catcher, "handle" );

  QTest::mouseMove( tooltip.viewport(), pos );
  QTest::mousePress( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, pos );
  QTest::mouseMove( tooltip.viewport(), pos + QPoint( 1, 0 ) );
  QTest::mouseRelease( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, pos + QPoint( 1, 0 ) );

  QCOMPARE( catcher.urls, QStringList { QStringLiteral( "https://example.com/x" ) } );
  QDesktopServices::unsetUrlHandler( QStringLiteral( "https" ) );
}

void TestKadasMapItemTooltip::linkStaysShutWhenTheClickWasADrag()
{
  // Dragging across the text selects it; that must not also open whatever link
  // the press happened to start on.
  QgsMapCanvas canvas;
  canvas.resize( 800, 600 );
  canvas.show();
  KadasMapItemTooltip tooltip( &canvas );
  tooltip.setHtml( QStringLiteral( "<p>see <a href=\"https://example.com/x\">https://example.com/x</a> for more</p>" ) );
  tooltip.move( 10, 10 );
  tooltip.show();
  QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );

  const QPoint pos = linkPos( tooltip );
  QVERIFY( !pos.isNull() );

  UrlCatcher catcher;
  QDesktopServices::setUrlHandler( QStringLiteral( "https" ), &catcher, "handle" );

  const QPoint away = pos + QPoint( 60, 0 );
  QTest::mouseMove( tooltip.viewport(), pos );
  QTest::mousePress( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, pos );
  QTest::mouseMove( tooltip.viewport(), away );
  QTest::mouseRelease( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, away );

  QVERIFY( catcher.urls.isEmpty() );
  QDesktopServices::unsetUrlHandler( QStringLiteral( "https" ) );
}

void TestKadasMapItemTooltip::linkStaysShutWhileAMapToolOwnsThePointer()
{
  // While a map tool is drawing, the tooltip is a preview: its buttons belong to
  // the canvas underneath, links included.
  QgsMapCanvas canvas;
  canvas.resize( 800, 600 );
  canvas.show();
  KadasMapItemTooltip tooltip( &canvas );
  tooltip.setInteractive( false );
  tooltip.setHtml( QStringLiteral( "<p>see <a href=\"https://example.com/x\">https://example.com/x</a> for more</p>" ) );
  tooltip.move( 10, 10 );
  tooltip.show();
  QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );

  const QPoint pos = linkPos( tooltip );
  QVERIFY( !pos.isNull() );

  UrlCatcher catcher;
  QDesktopServices::setUrlHandler( QStringLiteral( "https" ), &catcher, "handle" );

  QTest::mouseMove( tooltip.viewport(), pos );
  QTest::mousePress( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, pos );
  QTest::mouseRelease( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, pos );

  QVERIFY( catcher.urls.isEmpty() );
  QDesktopServices::unsetUrlHandler( QStringLiteral( "https" ) );
}

void TestKadasMapItemTooltip::plainClickLeavesNoTextSelectionBehind()
{
  // Regression: the release was handed on to QTextEdit only when the click was
  // ruled out as a link click, so an ordinary click on plain text never reached
  // it. A press inside an existing selection deliberately leaves that selection
  // alone, because it might be the start of a drag; it is the release that
  // decides it was a click after all and clears it. Without that release the
  // selection could not be dismissed at all.
  QgsMapCanvas canvas;
  canvas.resize( 800, 600 );
  canvas.show();
  KadasMapItemTooltip tooltip( &canvas );
  tooltip.setHtml( QStringLiteral( "<p>a stretch of ordinary text, long enough to select across</p>" ) );
  tooltip.move( 10, 10 );
  tooltip.show();
  QVERIFY( QTest::qWaitForWindowExposed( &canvas ) );

  const QPoint start( 2, 4 );
  const QPoint end( 70, 4 );
  QTest::mousePress( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, start );
  QTest::mouseMove( tooltip.viewport(), end );
  QTest::mouseRelease( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, end );
  QVERIFY2( tooltip.textCursor().hasSelection(), "dragging across the text did not select it" );

  // A plain click inside that selection dismisses it.
  const QPoint inside( 35, 4 );
  QTest::mousePress( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, inside );
  QTest::mouseRelease( tooltip.viewport(), Qt::LeftButton, Qt::NoModifier, inside );
  QVERIFY2( !tooltip.textCursor().hasSelection(), "clicking inside the selection left it standing, so the release never reached QTextEdit" );
}

QTEST_MAIN( TestKadasMapItemTooltip )
#include "testkadasmapitemtooltip.moc"
