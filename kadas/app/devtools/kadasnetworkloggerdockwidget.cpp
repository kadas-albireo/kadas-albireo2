
#include <qgis/qgsnetworklogger.h>
#include <qgis/qgsnetworkloggerpanelwidget.h>

#include "kadasnetworkloggerdockwidget.h"

KadasNetworkLoggerDockWidget::KadasNetworkLoggerDockWidget( QgsNetworkLogger *networkLogger, QWidget *parent )
  : QDockWidget( parent )
  , mNetworkLogger( networkLogger )

{
  mNetworkLoggerPanelWidget = new QgsNetworkLoggerPanelWidget( mNetworkLogger, this );

  QDockWidget::setWidget( mNetworkLoggerPanelWidget );
  QDockWidget::setStyleSheet( "#mToolbar { background-color: white }" );
}

KadasNetworkLoggerDockWidget::~KadasNetworkLoggerDockWidget()
{
  delete mNetworkLoggerPanelWidget;
}

void KadasNetworkLoggerDockWidget::showEvent( QShowEvent *event )
{
  mNetworkLogger->enableLogging( true );
}
