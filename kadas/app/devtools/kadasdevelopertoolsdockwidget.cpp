
#include <QTabWidget>

#include <qgis/qgsnetworklogger.h>
#include <qgis/qgsnetworkloggerpanelwidget.h>
#include <qgis/qgssettingstreewidget.h>

#include "kadasdevelopertoolsdockwidget.h"

KadasDeveloperToolsDockWidget::KadasDeveloperToolsDockWidget( QgsNetworkLogger *networkLogger, QWidget *parent )
  : QDockWidget( parent )
  , mNetworkLogger( networkLogger )

{
  setWindowTitle( tr( "Developer Tools" ) );

  // Create tab widget
  mTabWidget = new QTabWidget( this );

  // Create and add network logger panel
  mNetworkLoggerPanelWidget = new QgsNetworkLoggerPanelWidget( mNetworkLogger, mTabWidget );
  mTabWidget->addTab( mNetworkLoggerPanelWidget, tr( "Network Logger" ) );

  // Create and add advanced settings editor
  mSettingsTreeWidget = new QgsSettingsTreeWidget( mTabWidget );
  mTabWidget->addTab( mSettingsTreeWidget, tr( "Settings" ) );

  QDockWidget::setWidget( mTabWidget );
  QDockWidget::setStyleSheet( "#mToolbar { background-color: white }" );
}

KadasDeveloperToolsDockWidget::~KadasDeveloperToolsDockWidget()
{
  delete mNetworkLoggerPanelWidget;
  delete mSettingsTreeWidget;
  delete mTabWidget;
}

void KadasDeveloperToolsDockWidget::showEvent( QShowEvent *event )
{
  mNetworkLogger->enableLogging( true );
}
