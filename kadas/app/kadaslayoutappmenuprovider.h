/***************************************************************************
    kadaslayoutappmenuprovider.h
    ----------------------------
    copyright            : (C) 2017 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASLAYOUTAPPMENUPROVIDER_H
#define KADASLAYOUTAPPMENUPROVIDER_H

#include <QObject>

#include <qgis/qgslayoutview.h>


class KadasLayoutDesignerDialog;


class KadasLayoutAppMenuProvider : public QObject, public QgsLayoutViewMenuProvider
{
    Q_OBJECT

  public:

    KadasLayoutAppMenuProvider( KadasLayoutDesignerDialog *designer );

    QMenu *createContextMenu( QWidget *parent, QgsLayout *layout, QPointF layoutPoint ) const override;

  private:

    KadasLayoutDesignerDialog *mDesigner = nullptr;

};

#endif // KADASLAYOUTAPPMENUPROVIDER_H
