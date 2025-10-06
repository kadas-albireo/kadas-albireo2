/***************************************************************************
    kadasmessagelogviewer.h
    -----------------------
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

#ifndef KADASMESSAGELOGVIEWER_H
#define KADASMESSAGELOGVIEWER_H

#include <qgis/qgsmessagelogviewer.h>

class KadasMessageLogViewer : public QgsMessageLogViewer {
  Q_OBJECT

public:
  KadasMessageLogViewer(QWidget *parent,
                        Qt::WindowFlags fl = Qt::WindowFlags());

protected:
  void closeEvent(QCloseEvent *e) override;
  void reject() override;
};

#endif // KADASMESSAGELOGVIEWER_H
