/***************************************************************************
    kadasbottombar.h
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

#ifndef KADASBOTTOMBAR_H
#define KADASBOTTOMBAR_H

#include <qgsfloatingwidget.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasBottomBar : public QgsFloatingWidget {
public:
  KadasBottomBar(QgsMapCanvas *canvas, const QString &color = "orange");

protected:
  QgsMapCanvas *mCanvas;
};

#endif // KADASBOTTOMBAR_H
