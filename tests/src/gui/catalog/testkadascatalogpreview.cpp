/***************************************************************************
    testkadascatalogpreview.cpp
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

#include <memory>

#include <QtTest/QTest>

#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmessagebar.h>

#include <kadas/gui/kadascatalogpreview.h>

/**
 * Lifetime tests for the catalog preview.
 *
 * Its canvas overlay item is owned by the canvas' graphics scene, which the
 * canvas destroys in its own destructor body -- before the base QObject
 * destructor deletes its children. A preview parented to the canvas therefore
 * runs its destructor with the item already gone, and freeing it again crashed
 * the application on exit.
 */
class TestKadasCatalogPreview : public QObject
{
    Q_OBJECT
  private slots:
    void survivesCanvasDestroyedFirst();
    void survivesPreviewDestroyedFirst();
    void clearWithoutSourceIsHarmless();
};

void TestKadasCatalogPreview::survivesCanvasDestroyedFirst()
{
  // The application exit path: the canvas owns the preview and goes down first.
  auto canvas = std::make_unique<QgsMapCanvas>();
  QgsMessageBar messageBar;
  new KadasCatalogPreview( canvas.get(), &messageBar, canvas.get() );

  canvas.reset();

  QVERIFY( true ); // Reaching here without crashing is the assertion.
}

void TestKadasCatalogPreview::survivesPreviewDestroyedFirst()
{
  // The other order: the preview goes first and has to take its overlay item
  // out of the still living scene itself.
  auto canvas = std::make_unique<QgsMapCanvas>();
  QgsMessageBar messageBar;
  auto preview = std::make_unique<KadasCatalogPreview>( canvas.get(), &messageBar );

  preview.reset();
  canvas.reset();

  QVERIFY( true );
}

void TestKadasCatalogPreview::clearWithoutSourceIsHarmless()
{
  auto canvas = std::make_unique<QgsMapCanvas>();
  QgsMessageBar messageBar;
  KadasCatalogPreview preview( canvas.get(), &messageBar );

  // Hiding the catalog clears the preview whether or not one is showing.
  preview.clear();
  preview.clear();

  QVERIFY( true );
}

QTEST_MAIN( TestKadasCatalogPreview )
#include "testkadascatalogpreview.moc"
