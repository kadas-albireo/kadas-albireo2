/***************************************************************************
    kadasmapwidget.h
    ----------------
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

#ifndef KADASMAPWIDGET_H
#define KADASMAPWIDGET_H

#include <QDockWidget>

#include <kadas/gui/kadas_gui.h>

class QLabel;
class QLineEdit;
class QMenu;
class QStackedWidget;
class QToolButton;
class QgsMapCanvas;
class QgsRectangle;
class KadasMapItem;


class KADAS_GUI_EXPORT KadasMapWidget : public QDockWidget
{
    Q_OBJECT
  public:
    KadasMapWidget( int number, const QString& title, QgsMapCanvas* masterCanvas, QWidget* parent = 0 );
    void setInitialLayers( const QStringList& initialLayers, bool updateMenu = false );
    int getNumber() const { return mNumber; }
    QStringList getLayers() const;

    QgsRectangle getMapExtent() const;
    void setMapExtent( const QgsRectangle& extent );

    bool getLocked() const;
    void setLocked( bool locked );

  private:
    int mNumber;
    QgsMapCanvas* mMasterCanvas;
    QToolButton* mLayerSelectionButton;
    QMenu* mLayerSelectionMenu;
    QToolButton* mLockViewButton;
    QToolButton* mCloseButton;
    QStackedWidget* mTitleStackedWidget;
    QLabel* mTitleLabel;
    QLineEdit* mTitleLineEdit;
    QgsMapCanvas* mMapCanvas;
    QStringList mInitialLayers;
    bool mUnsetFixedSize;

    void showEvent( QShowEvent * ) override;
    bool eventFilter( QObject *obj, QEvent *ev ) override;
    void contextMenuEvent( QContextMenuEvent * e ) override;

  private slots:
    void setCanvasLocked( bool locked );
    void syncCanvasExtents();
    void updateLayerSelectionMenu();
    void updateLayerSet();
    void updateMapProjection();
    void closeMapWidget();
    void addMapCanvasItem(KadasMapItem* item);
    void removeMapCanvasItem(KadasMapItem* item);
};

#endif // KADASMAPWIDGET_H
