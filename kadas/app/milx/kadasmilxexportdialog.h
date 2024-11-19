/***************************************************************************
  kadasmilxexportdialog.h
  --------------------------------------
  Date                 : September 2024
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

#ifndef KADASMILXEXPORTDIALOG_H
#define KADASMILXEXPORTDIALOG_H

#include "ui_kadasmilxexportdialog.h"


class KadasMilxExportDialog : public QDialog
{
    Q_OBJECT
  public:
    enum class ExportMode
    {
      Milx,
      Kml
    };

    KadasMilxExportDialog( QWidget *parent = 0 );

    void setExportMode( ExportMode exportMode );

    void setVersions( const QStringList &versionNames, const QStringList &versionTags );

    QStringList selectedLayers() const;
    QString selectedCartoucheLayerId() const;

    QString selectedVersionTag() const;

  private:
    Ui::KadasMilxExportDialog ui;
};

#endif // KADASMILXEXPORTDIALOG_H
