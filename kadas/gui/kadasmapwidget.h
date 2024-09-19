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

#include "kadas/gui/kadas_gui.h"

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
    KadasMapWidget( int number, const QString &id, const QString &title, QgsMapCanvas *masterCanvas, QWidget *parent = 0 );
    ~KadasMapWidget();
    void setInitialLayers( const QStringList &initialLayers );
    int getNumber() const { return mNumber; }
    const QString &id() const { return mId; }
    QStringList getLayers() const;
    QgsMapCanvas *mapCanvas() const { return mMapCanvas; }

    QgsRectangle getMapExtent() const;
    void setMapExtent( const QgsRectangle &extent );

    bool getLocked() const;
    void setLocked( bool locked );

  signals:
    void aboutToBeDestroyed();

  protected:
    void showEvent( QShowEvent * ) override;
    bool eventFilter( QObject *obj, QEvent *ev ) override;
    void contextMenuEvent( QContextMenuEvent *e ) override;

  private:
    int mNumber;
    QString mId;
    QgsMapCanvas *mMasterCanvas;
    QToolButton *mLayerSelectionButton;
    QMenu *mLayerSelectionMenu;
    QToolButton *mLockViewButton;
    QToolButton *mCloseButton;
    QStackedWidget *mTitleStackedWidget;
    QLabel *mTitleLabel;
    QLineEdit *mTitleLineEdit;
    QgsMapCanvas *mMapCanvas;
    QStringList mInitialLayers;
    bool mUnsetFixedSize = true;

  private slots:
    void setCanvasLocked( bool locked );
    void syncCanvasExtents();
    void updateLayerSelectionMenu();
    void updateLayerSet();
    void updateMapProjection();
    void closeMapWidget();
    void addMapCanvasItem( const KadasMapItem *item );
    void removeMapCanvasItem( const KadasMapItem *item );
};

#endif // KADASMAPWIDGET_H
