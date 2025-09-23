#ifndef KADASNETWORKLOGGERDIALOG_H
#define KADASNETWORKLOGGERDIALOG_H

#include <QDialog>

class QgsNetworkLogger;
class QgsNetworkLoggerTreeView;

namespace Ui
{
  class KadasNetworkLoggerDialog;
}

class KadasNetworkLoggerDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit KadasNetworkLoggerDialog( QgsNetworkLogger *networkLogger, QWidget *parent = nullptr );
    ~KadasNetworkLoggerDialog();

  private:
    void showEvent( QShowEvent *event );

    Ui::KadasNetworkLoggerDialog *ui;

    QgsNetworkLogger *mLogger = nullptr;
    QgsNetworkLoggerTreeView *mTreeView = nullptr;
};

#endif // KADASNETWORKLOGGERDIALOG_H
