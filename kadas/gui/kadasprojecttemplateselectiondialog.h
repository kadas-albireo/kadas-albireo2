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

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/ui_kadasprojecttemplateselectiondialog.h"

class QDir;
class QFileIconProvider;

class KADAS_GUI_EXPORT KadasProjectTemplateSelectionDialog
    : public QDialog,
      private Ui::KadasProjectTemplateSelectionDialogBase {
  Q_OBJECT
public:
  KadasProjectTemplateSelectionDialog(QWidget *parent = nullptr);

signals:
  void templateSelected(const QString &templateFile, const QUrl &templateUrl);

private:
  QAbstractButton *mCreateButton = nullptr;

  void populateFileTree(const QDir &dir, QTreeWidgetItem *parent,
                        const QFileIconProvider &iconProvider);

  static constexpr int sItemUrlRole = Qt::UserRole + 1;
  static constexpr int sFilePathRole = Qt::UserRole + 2;

private slots:
  void itemClicked(QTreeWidgetItem *item, int column);
  void itemDoubleClicked(QTreeWidgetItem *item, int column);
  void createProject();
  void parseServiceReply();
};

#endif // KADASPROJECTTEMPLATESELECTIONDIALOG_H
