/***************************************************************************
    testkadassidepanelreflow.cpp
    ----------------------------
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

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QtTest/QTest>

#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsrectangle.h>
#include <qgis/qgstest.h>

#include <kadas/gui/kadassidepanel.h>
#include <kadas/gui/kadassidepanelhost.h>

/**
 * Reproduces the map "scale drop" that used to happen when a tool side-panel
 * was shown. The panel is docked next to the canvas (it steals horizontal
 * space), so without compensation QGIS re-fits the same extent into the now
 * narrower canvas and the map zooms out. The host is supposed to keep the
 * map *cropped* instead of rescaled: same scale, same anchored edge.
 *
 * The test wires the canvas + host exactly like the main window and drives the
 * real consumer lifecycle (construct, populate, show), which is what defeated
 * earlier, naive synchronous fixes.
 */
class TestKadasSidePanelReflow : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void showAndHideKeepsScale();
    void togglingPanelWidthKeepsScale();

  private:
    //! Pumps the event loop long enough for all deferred reflows to settle.
    static void settle();
};

void TestKadasSidePanelReflow::initTestCase()
{
  QgsApplication::init();
  QgsApplication::initQgis();
}

void TestKadasSidePanelReflow::settle()
{
  for ( int i = 0; i < 10; ++i )
  {
    qApp->processEvents();
    QTest::qWait( 20 );
  }
}

void TestKadasSidePanelReflow::showAndHideKeepsScale()
{
  // Wire the canvas and the right-edge host like KadasMainWindow does.
  QWidget window;
  QVBoxLayout *outerLayout = new QVBoxLayout( &window );
  outerLayout->setContentsMargins( 0, 0, 0, 0 );

  QHBoxLayout *canvasRow = new QHBoxLayout();
  canvasRow->setContentsMargins( 0, 0, 0, 0 );
  canvasRow->setSpacing( 0 );

  QgsMapCanvas *canvas = new QgsMapCanvas();
  canvas->setDestinationCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  canvasRow->addWidget( canvas, 1 );

  KadasSidePanelHost *host = new KadasSidePanelHost( KadasSidePanelHost::Edge::Right );
  host->setMapCanvas( canvas );
  canvasRow->addWidget( host, 0 );

  outerLayout->addLayout( canvasRow );

  window.resize( 900, 600 );
  window.show();
  QVERIFY( QTest::qWaitForWindowExposed( &window ) );

  // Establish a known, aspect-fitted extent on the full-width canvas.
  canvas->setExtent( QgsRectangle( 0, 0, 1000, 1000 ) );
  canvas->refresh();
  settle();

  const int fullWidthPx = canvas->mapSettings().outputSize().width();
  const double mupp0 = canvas->mapUnitsPerPixel();
  const double xMin0 = canvas->extent().xMinimum();
  QVERIFY2( fullWidthPx > 0, "canvas has no output size" );
  QVERIFY2( mupp0 > 0, "canvas has no scale" );

  // --- Show: construct (auto-docks), populate, then show, like the tools do.
  KadasSidePanel *panel = new KadasSidePanel( canvas );
  panel->setTitle( QStringLiteral( "Measure" ) );
  QLabel *value = new QLabel( QStringLiteral( "123456.78 m" ) );
  value->setMinimumWidth( 220 );
  panel->addRow( QStringLiteral( "Result" ), value );
  panel->adjustSize();
  panel->show();
  settle();

  const int croppedWidthPx = canvas->mapSettings().outputSize().width();
  const double muppShown = canvas->mapUnitsPerPixel();
  const double xMinShown = canvas->extent().xMinimum();

  // Sanity: the panel must actually have taken canvas width, otherwise there
  // is nothing to compensate and the test proves nothing.
  QVERIFY2( croppedWidthPx < fullWidthPx, "panel did not consume any canvas width" );

  // The map must be cropped, not rescaled: scale and left edge unchanged.
  QGSCOMPARENEAR( muppShown, mupp0, mupp0 * 1e-3 );
  QGSCOMPARENEAR( xMinShown, xMin0, mupp0 ); // within one pixel

  // --- Hide: removing the panel must restore the full width at the same scale.
  delete panel;
  settle();

  const int restoredWidthPx = canvas->mapSettings().outputSize().width();
  const double muppHidden = canvas->mapUnitsPerPixel();
  const double xMinHidden = canvas->extent().xMinimum();

  QGSCOMPARENEAR( restoredWidthPx, fullWidthPx, 1 );
  QGSCOMPARENEAR( muppHidden, mupp0, mupp0 * 1e-3 );
  QGSCOMPARENEAR( xMinHidden, xMin0, mupp0 );
}

/**
 * Reproduces the compounding zoom-out seen when the catalog column inside the
 * layer tree panel is shown and hidden repeatedly. That toggle does not add or
 * remove a panel: it resizes the docked panel in place
 * (KadasMainWindow::setLayersWidgetWidth), bracketed by
 * beginPanelResize()/endPanelResize(). Every cycle that ends at the original
 * panel width must also end at the original scale and anchored edge.
 */
void TestKadasSidePanelReflow::togglingPanelWidthKeepsScale()
{
  QWidget window;
  QVBoxLayout *outerLayout = new QVBoxLayout( &window );
  outerLayout->setContentsMargins( 0, 0, 0, 0 );

  QHBoxLayout *canvasRow = new QHBoxLayout();
  canvasRow->setContentsMargins( 0, 0, 0, 0 );
  canvasRow->setSpacing( 0 );

  // The layer tree panel sits on the left edge, so the canvas' right edge is
  // the anchored one.
  KadasSidePanelHost *host = new KadasSidePanelHost( KadasSidePanelHost::Edge::Left );
  canvasRow->addWidget( host, 0 );

  QgsMapCanvas *canvas = new QgsMapCanvas();
  canvas->setDestinationCrs( QgsCoordinateReferenceSystem( QStringLiteral( "EPSG:3857" ) ) );
  canvasRow->addWidget( canvas, 1 );
  host->setMapCanvas( canvas );

  outerLayout->addLayout( canvasRow );

  window.resize( 900, 600 );
  window.show();
  QVERIFY( QTest::qWaitForWindowExposed( &window ) );

  // Dock a fixed-width panel, like the layer tree widget.
  QWidget *panel = new QWidget();
  panel->setFixedWidth( 200 );
  host->addPanel( panel );
  panel->show();

  canvas->setExtent( QgsRectangle( 0, 0, 1000, 1000 ) );
  canvas->refresh();
  settle();

  const int narrowWidthPx = canvas->mapSettings().outputSize().width();
  const double mupp0 = canvas->mapUnitsPerPixel();
  const double xMax0 = canvas->extent().xMaximum();
  QVERIFY2( narrowWidthPx > 0, "canvas has no output size" );
  QVERIFY2( mupp0 > 0, "canvas has no scale" );

  // Four toggles: wide, narrow, wide, narrow -- back to the starting width.
  for ( int i = 0; i < 4; ++i )
  {
    host->beginPanelResize();
    panel->setFixedWidth( i % 2 == 0 ? 500 : 200 );
    host->endPanelResize();
    settle();

    // Every intermediate state must hold the scale too, not just the round trip.
    const QByteArray msg = QStringLiteral( "scale drifted to %1 (from %2) after toggle %3" ).arg( canvas->mapUnitsPerPixel() ).arg( mupp0 ).arg( i ).toUtf8();
    QVERIFY2( qAbs( canvas->mapUnitsPerPixel() - mupp0 ) < mupp0 * 1e-3, msg.constData() );
  }

  QGSCOMPARENEAR( canvas->mapSettings().outputSize().width(), narrowWidthPx, 1 );
  QGSCOMPARENEAR( canvas->mapUnitsPerPixel(), mupp0, mupp0 * 1e-3 );
  QGSCOMPARENEAR( canvas->extent().xMaximum(), xMax0, mupp0 );
}

int main( int argc, char *argv[] )
{
  // Run headless and deterministically regardless of the host environment.
  if ( qEnvironmentVariableIsEmpty( "QT_QPA_PLATFORM" ) )
    qputenv( "QT_QPA_PLATFORM", QByteArray( "offscreen" ) );

  QgsApplication app( argc, argv, true );
  TestKadasSidePanelReflow tc;
  return QTest::qExec( &tc, argc, argv );
}

#include "testkadassidepanelreflow.moc"
