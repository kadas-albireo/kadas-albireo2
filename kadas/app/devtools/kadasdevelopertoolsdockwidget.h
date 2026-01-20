#ifndef KADASDEVELOPERTOOLSDOCKWIDGET_H
#define KADASDEVELOPERTOOLSDOCKWIDGET_H

#include <QDockWidget>

class QgsNetworkLogger;
class QgsNetworkLoggerPanelWidget;
class QgsSettingsTreeWidget;
class QTabWidget;


class KadasDeveloperToolsDockWidget : public QDockWidget
{
    Q_OBJECT

  public:
    explicit KadasDeveloperToolsDockWidget( QgsNetworkLogger *networkLogger, QWidget *parent = nullptr );
    ~KadasDeveloperToolsDockWidget();

  private:
    void showEvent( QShowEvent *event );

    QgsNetworkLogger *mNetworkLogger = nullptr;
    QgsNetworkLoggerPanelWidget *mNetworkLoggerPanelWidget = nullptr;
    QgsSettingsTreeWidget *mSettingsTreeWidget = nullptr;
    QTabWidget *mTabWidget = nullptr;
};

#endif // KADASDEVELOPERTOOLSDOCKWIDGET_H
