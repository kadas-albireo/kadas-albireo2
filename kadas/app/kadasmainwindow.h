/***************************************************************************
    kadasmainwindow.h
    -----------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASMAINWINDOW_H
#define KADASMAINWINDOW_H

#include <QMainWindow>
#include <QPointer>

#include <qgis/qgsmessagebaritem.h>

#include "ui_kadaswindowbase.h"
#include "ui_kadastopwidget.h"
#include "ui_kadasstatuswidget.h"

class QSplashScreen;
class QgsDecorationGrid;
class QgsMessageBar;
class KadasCoordinateDisplayer;
class KadasGpsIntegration;


class KadasMainWindow : public QMainWindow, private Ui::KadasWindowBase, private Ui::KadasTopWidget, private Ui::KadasStatusWidget
{
public:
  explicit KadasMainWindow(QSplashScreen* splash);

  QgsMapCanvas* mapCanvas() const { return mMapCanvas; }
  QgsMessageBar* messageBar() const{ return mInfoBar; }
  QgsLayerTreeView* layerTreeView() const{ return mLayerTreeView; }
  int messageTimeout() const;

  QWidget* addRibbonTab( const QString& name );
  void addActionToTab( QAction* action, QWidget* tabWidget, QgsMapTool *associatedMapTool = nullptr );
  void addMenuButtonToTab( const QString &text, const QIcon &icon, QMenu* menu, QWidget* tabWidget );

private slots:
  void checkLayerProjection( QgsMapLayer* layer );
  void onDecimalPlacesChanged(int places);
  void onLanguageChanged(int idx);
  void onNumericInputCheckboxToggled( bool checked );
  void onSnappingChanged(bool enabled);
  void setMapScale();
  void showFavoriteContextMenu(const QPoint& p);
  void showProjectSelectionWidget();
  void showScale( double scale );
  void switchToTabForTool( QgsMapTool* tool );
  void toggleLayerTree();
  void checkOnTheFlyProjection();
  void zoomFull();
  void zoomIn();
  void zoomNext();
  void zoomOut();
  void zoomPrev();

private:
  bool eventFilter( QObject *obj, QEvent *ev ) override;
  void mousePressEvent( QMouseEvent* event ) override;
  void mouseMoveEvent( QMouseEvent* event ) override;
  void dropEvent( QDropEvent* event ) override;
  void dragEnterEvent( QDragEnterEvent* event ) override;
  void restoreFavoriteButton( QToolButton* button );
  void configureButtons();
  void setActionToButton( QAction* action, QToolButton* button, const QKeySequence& shortcut = QKeySequence(), QgsMapTool *tool = 0 );
  void updateWidgetPositions();
  KadasRibbonButton* addRibbonButton( QWidget* tabWidget );
  void showSourceSelectDialog(const QString& provider);

  QgsMessageBar* mInfoBar = nullptr;
  QPointer<QgsMessageBarItem> mReprojMsgItem;

  KadasCoordinateDisplayer* mCoordinateDisplayer = nullptr;
  KadasGpsIntegration* mGpsIntegration = nullptr;
  QgsDecorationGrid* mDecorationGrid = nullptr;

  QTimer mLoadingTimer;
  QPoint mResizePressPos;
  QPoint mDragStartPos;
  QMap<QString, QAction*> mAddedActions;

  friend class KadasGpsIntegration;
};

#endif // KADASMAINWINDOW_H
