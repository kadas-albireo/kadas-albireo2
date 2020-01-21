/***************************************************************************
    kadasattributetabledialog.h
    ---------------------------
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

#ifndef KADASATTRIBUTETABLEDIALOG_H
#define KADASATTRIBUTETABLEDIALOG_H

#include <QDialog>

#include <kadas/gui/kadas_gui.h>

class QgsMapCanvas;
class QgsVectorLayer;


class KADAS_GUI_EXPORT KadasAttributeTableDialog : public QDialog
{
    Q_OBJECT

  public:
    KadasAttributeTableDialog( QgsVectorLayer *layer, QgsMapCanvas *canvas, QWidget *parent = nullptr );
};

#endif // KADASATTRIBUTETABLEDIALOG_H
