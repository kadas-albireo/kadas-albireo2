/***************************************************************************
    kadascrssselection.h
    --------------------
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

#ifndef KADASCRSSELECTION_H
#define KADASCRSSELECTION_H

#include <QToolButton>

#include <kadas/gui/kadas_gui.h>

class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasCrsSelection : public QToolButton
{
    Q_OBJECT
  public:
    KadasCrsSelection( QWidget* parent = 0 );

    void setMapCanvas( QgsMapCanvas* canvas );

  private:
    QgsMapCanvas* mMapCanvas;

  private slots:
    void syncCrsButton();
    void setMapCrs();
    void selectMapCrs();
};

#endif // KADASCRSSELECTION_H
