/***************************************************************************
    testkadassidepanelsize.cpp
    --------------------------
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

#include <QStringList>
#include <QWidget>
#include <QtTest/QTest>

#include <qgis/qgsapplication.h>

#include <kadas/gui/annotationitems/kadasannotationcontrollerregistry.h>
#include <kadas/gui/annotationitems/kadasannotationitemcontroller.h>
#include <kadas/gui/annotationitems/kadasannotationitemcontrollers.h>
#include <kadas/gui/annotationitems/kadasannotationstyleeditor.h>

/**
 * Width budget for a tool side-panel's content.
 *
 * KadasSidePanelHost sizes itself from the panel's size hint (see
 * kadassidepanelhost.cpp), so there is no hard cap at runtime: a panel that
 * lays its controls out too wide simply steals space from the map canvas.
 * This test is the guard against that regression — it instantiates each
 * annotation style-editor panel and asserts its preferred width stays within
 * the budget below.
 *
 * The budget is deliberately generous (a docked panel is comfortable around
 * 250-300 px). Bump it only after confirming the new layout genuinely needs
 * the extra width and still looks reasonable beside the canvas.
 */
static constexpr int MAX_PANEL_WIDTH = 360;

class TestKadasSidePanelSize : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();

    //! Guards that every annotation panel is discovered and exercised below.
    void allPanelsCovered();

    void editorFitsBudget_data();
    void editorFitsBudget();

  private:
    //! Polishes \a widget and asserts its preferred width is within budget.
    static void verifyWidth( QWidget *widget, const QString &name );

    //! Type ids of every registered controller that exposes a style-editor panel.
    static QStringList panelTypeIds();
};

void TestKadasSidePanelSize::initTestCase()
{
  QgsApplication::init();
  // Populate the controller registry so panels can be discovered from it.
  KadasAnnotationItemControllers::registerBuiltins();
}

QStringList TestKadasSidePanelSize::panelTypeIds()
{
  QStringList result;
  const KadasAnnotationControllerRegistry *registry = KadasAnnotationControllerRegistry::instance();
  const QStringList types = registry->registeredTypes();
  for ( const QString &type : types )
  {
    KadasAnnotationItemController *controller = registry->controllerFor( type );
    if ( !controller )
      continue;
    // A null editor means this annotation type intentionally has no side panel.
    std::unique_ptr<KadasAnnotationStyleEditor> editor( controller->createStyleEditor() );
    if ( editor )
      result.append( type );
  }
  return result;
}

void TestKadasSidePanelSize::verifyWidth( QWidget *widget, const QString &name )
{
  // Let style/layout compute final geometry hints before measuring.
  widget->ensurePolished();
  widget->adjustSize();

  const int hint = widget->sizeHint().width();
  const int minHint = widget->minimumSizeHint().width();

  const QByteArray msg = QStringLiteral( "%1: sizeHint width %2 px (min %3 px) exceeds budget %4 px" ).arg( name ).arg( hint ).arg( minHint ).arg( MAX_PANEL_WIDTH ).toUtf8();

  qInfo( "%s: sizeHint width %d px, minimumSizeHint width %d px", name.toUtf8().constData(), hint, minHint );
  QVERIFY2( hint <= MAX_PANEL_WIDTH, msg.constData() );
}

void TestKadasSidePanelSize::allPanelsCovered()
{
  // Coverage mechanism: the panels exercised by editorFitsBudget() are discovered
  // straight from the annotation controller registry, so any newly added panel is
  // picked up automatically and any controller that drops its editor stops being
  // tested. This test guards the discovery itself — it confirms the registry is
  // populated and that at least one known panel is found (i.e. registration did
  // not silently break) and reports the full coverage breakdown for the log.
  const KadasAnnotationControllerRegistry *registry = KadasAnnotationControllerRegistry::instance();
  const QStringList types = registry->registeredTypes();
  QVERIFY2( !types.isEmpty(), "No annotation controllers registered" );

  QStringList withPanel;
  QStringList withoutPanel;
  for ( const QString &type : types )
  {
    KadasAnnotationItemController *controller = registry->controllerFor( type );
    QVERIFY( controller );
    std::unique_ptr<KadasAnnotationStyleEditor> editor( controller->createStyleEditor() );
    ( editor ? withPanel : withoutPanel ).append( type );
  }

  qInfo() << "Side-panel editors under test:" << withPanel;
  qInfo() << "Annotation types without a side panel:" << withoutPanel;

  QVERIFY2( !withPanel.isEmpty(), "No annotation style-editor panels found to test" );
}

void TestKadasSidePanelSize::editorFitsBudget_data()
{
  QTest::addColumn<QString>( "typeId" );

  const KadasAnnotationControllerRegistry *registry = KadasAnnotationControllerRegistry::instance();
  const QStringList types = panelTypeIds();
  for ( const QString &type : types )
  {
    const KadasAnnotationItemController *controller = registry->controllerFor( type );
    QTest::newRow( controller->itemName().toUtf8().constData() ) << type;
  }
}

void TestKadasSidePanelSize::editorFitsBudget()
{
  QFETCH( QString, typeId );

  KadasAnnotationItemController *controller = KadasAnnotationControllerRegistry::instance()->controllerFor( typeId );
  QVERIFY( controller );

  std::unique_ptr<KadasAnnotationStyleEditor> editor( controller->createStyleEditor() );
  QVERIFY( editor );

  verifyWidth( editor.get(), controller->itemName() );
}

QTEST_MAIN( TestKadasSidePanelSize )
#include "testkadassidepanelsize.moc"
