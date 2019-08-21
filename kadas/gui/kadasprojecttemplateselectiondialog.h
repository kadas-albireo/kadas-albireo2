/***************************************************************************
    kadasprojecttemplateselectiondialog.h
    -------------------------------------
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

#ifndef KADASPROJECTTEMPLATESELECTIONDIALOG_H
#define KADASPROJECTTEMPLATESELECTIONDIALOG_H

#include <QDialog>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/ui_kadasprojecttemplateselectiondialog.h>

class QDialogButtonBox;
class QFileSystemModel;
class QModelIndex;
class QTreeView;

class KADAS_GUI_EXPORT KadasProjectTemplateSelectionDialog : public QDialog, Ui::KadasProjectTemplateSelectionDialogBase
{
    Q_OBJECT
  public:
    KadasProjectTemplateSelectionDialog( QWidget *parent = 0 );
    const QString &selectedTemplate() const { return mSelectedTemplate; }

  private:
    QFileSystemModel *mModel;
    QAbstractButton *mCreateButton;
    QString mSelectedTemplate;

  private slots:
    void itemClicked( const QModelIndex &index );
    void itemDoubleClicked( const QModelIndex &index );
    void createProject();

};

#endif // KADASPROJECTTEMPLATESELECTIONDIALOG_H
