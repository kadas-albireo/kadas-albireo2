/***************************************************************************
  kadasdevtools.h
  --------------------------------------
  Date                 : July 2024
  Copyright            : (C) 2024 by Damiano Lombardi
  Email                : damiano@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASDEVTOOLS_H
#define KADASDEVTOOLS_H

#include "ui_kadasdevtools.h"
#include <kadas/gui/kadasbottombar.h>

class KadasRibbonButton;
class QgsTemporalControllerWidget;

class KadasDevTools: public KadasBottomBar, private Ui::KadasDevToolsBase
{
    Q_OBJECT
  public:
    KadasDevTools( QgsMapCanvas *canvas );
    ~KadasDevTools();

  private slots:
    void on_mCloseButton_clicked();

  private:

    QgsTemporalControllerWidget *mQgsTemporalControllerWidget;
};

#endif // KADASDEVTOOLS_H
