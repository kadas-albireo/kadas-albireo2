/***************************************************************************
  qgs3dnavigationwidget.h
  --------------------------------------
  Date                 : June 2019
  Copyright            : (C) 2019 by Ismail Sunni
  Email                : imajimatika at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADAS3DNAVIGATIONWIDGET_H
#define KADAS3DNAVIGATIONWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QToolButton>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>

#include <qgscameracontroller.h>

#include "kadas/app/3d/kadas3dmapcanvas.h"

#include <ui_3dnavigationwidget.h>

class QwtCompass;

class Kadas3DNavigationWidget : public QWidget, private Ui::Q3DNavigationWidget
{
    Q_OBJECT
  public:
    Kadas3DNavigationWidget( Kadas3DMapCanvas *parent = nullptr );

  public slots:

    /**
     * Update the state of navigation widget from camera's state
     */
    void updateFromCamera();

  signals:
    void sizeChanged( const QSize &newSize );

  protected:
    void resizeEvent( QResizeEvent *event ) override;
    void hideEvent( QHideEvent *event ) override;
    void showEvent( QShowEvent *event ) override;

  private:
    Kadas3DMapCanvas *mParent3DMapCanvas = nullptr;
    QStandardItemModel *mCameraInfoItemModel = nullptr;
};

#endif // KADAS3DNAVIGATIONWIDGET_H
