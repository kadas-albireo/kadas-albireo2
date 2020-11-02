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

#include <QPointer>

#include <qgis/qgsrectangle.h>

#include <kadas/app/kml/kadaskmlexport.h>
#include "ui_kadaskmlexportdialogbase.h"

class QgsMapLayer;
class KadasMapToolSelectRect;

class KadasKMLExportDialog : public QDialog, private Ui::KadasKMLExportDialogBase
{
    Q_OBJECT
  public:
    KadasKMLExportDialog( const QList<QgsMapLayer *> &activeLayers, QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags() );
    ~KadasKMLExportDialog();
    QString getFilename() const { return mFileLineEdit->text(); }
    QList<QgsMapLayer *> getSelectedLayers() const;
    double getExportScale() const { return mComboBoxExportScale->scale(); }
    const QgsRectangle &getFilterRect() const;

  private slots:
    void extentChanged( const QgsRectangle &extent );
    void extentEdited();
    void extentToggled( bool checked );
    void selectFile();

  private:
    QPointer<KadasMapToolSelectRect> mRectTool;
};

#endif // KADASKMLEXPORTDIALOG_H
