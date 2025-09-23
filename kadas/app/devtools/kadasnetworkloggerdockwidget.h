#ifndef KADASNETWORKLOGGERDOCKWIDGET_H
#define KADASNETWORKLOGGERDOCKWIDGET_H

#include <QDockWidget>

class QgsNetworkLogger;
class QgsNetworkLoggerPanelWidget;


class KadasNetworkLoggerDockWidget : public QDockWidget
{
    Q_OBJECT

  public:
    explicit KadasNetworkLoggerDockWidget( QgsNetworkLogger *networkLogger, QWidget *parent = nullptr );
    ~KadasNetworkLoggerDockWidget();

  private:
    void showEvent( QShowEvent *event );

    QgsNetworkLogger *mNetworkLogger = nullptr;
    QgsNetworkLoggerPanelWidget *mNetworkLoggerPanelWidget = nullptr;
};

#endif // KADASNETWORKLOGGERDOCKWIDGET_H
