/***************************************************************************
    kadaskmlexportdialog.h
    ----------------------
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

#ifndef KADASKMLEXPORTDIALOG_H
#define KADASKMLEXPORTDIALOG_H

#include <kadas/app/kml/kadaskmlexport.h>
#include "ui_kadaskmlexportdialogbase.h"

class QgsMapLayer;

class KadasKMLExportDialog : public QDialog, private Ui::KadasKMLExportDialogBase
{
    Q_OBJECT
  public:
    KadasKMLExportDialog( const QList<QgsMapLayer *> &activeLayers, QWidget *parent = 0, Qt::WindowFlags f = 0 );
    QString getFilename() const { return mFileLineEdit->text(); }
    QList<QgsMapLayer *> getSelectedLayers() const;
    double getExportScale() const { return 1. / mComboBoxExportScale->scale(); }

  private slots:
    void selectFile();
};

#endif // KADASKMLEXPORTDIALOG_H
