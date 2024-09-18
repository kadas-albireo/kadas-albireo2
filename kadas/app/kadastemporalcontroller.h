/***************************************************************************
  kadastemporalcontroller.h
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

#include "ui_kadastemporalcontroller.h"
#include "kadas/gui/kadasbottombar.h"

class KadasRibbonButton;
class QgsTemporalControllerWidget;

class KadasTemporalController: public KadasBottomBar, private Ui::KadasTemporalControllerBase
{
    Q_OBJECT
  public:
    KadasTemporalController(QgsMapCanvas *canvas);
    ~KadasTemporalController();

  private slots:
    void on_mCloseButton_clicked();

  private:

    QgsTemporalControllerWidget *mQgsTemporalControllerWidget;
};
