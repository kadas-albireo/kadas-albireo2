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

#include "ui_kadaswindowbase.h"
#include "ui_kadastopwidget.h"
#include "ui_kadasstatuswidget.h"

class QSplashScreen;


class KadasMainWindow : public QMainWindow, private Ui::KadasWindowBase, private Ui::KadasTopWidget, private Ui::KadasStatusWidget
{
public:
  explicit KadasMainWindow(QSplashScreen* splash);

  void openProject(const QString& fileName);
};

#endif // KADASMAINWINDOW_H
