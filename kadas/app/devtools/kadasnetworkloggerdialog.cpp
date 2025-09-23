
#include <qgis/qgsnetworklogger.h>
#include <qgis/qgsnetworkloggerpanelwidget.h>

#include "kadasnetworkloggerdialog.h"
#include "ui_kadasnetworkloggerdialog.h"

KadasNetworkLoggerDialog::KadasNetworkLoggerDialog( QgsNetworkLogger *networkLogger, QWidget *parent )
  : QDialog( parent )
  , ui( new Ui::KadasNetworkLoggerDialog )
  , mLogger( networkLogger )

{
  ui->setupUi( this );

  mTreeView = new QgsNetworkLoggerTreeView( mLogger, this );

  ui->mainLayout->addWidget( mTreeView );
}

KadasNetworkLoggerDialog::~KadasNetworkLoggerDialog()
{
  delete mTreeView;
  delete ui;
}

void KadasNetworkLoggerDialog::showEvent( QShowEvent *event )
{
  qDebug() << "Enable logging";
  mLogger->enableLogging( true );
}
