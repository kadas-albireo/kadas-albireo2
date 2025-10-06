/***************************************************************************
    kadashandlebadlayers.h
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

#ifndef KADASHANDLEBADLAYERS_H
#define KADASHANDLEBADLAYERS_H

#include <QDialog>

#include <qgis/qgsprojectbadlayerhandler.h>

class QTableWidget;
class QTableWidgetItem;

class KadasHandleBadLayersHandler : public QgsProjectBadLayerHandler {
public:
  void handleBadLayers(const QList<QDomNode> &layers) override;
};

class KadasHandleBadLayers : public QDialog {
  Q_OBJECT

public:
  KadasHandleBadLayers(const QList<QDomNode> &layers);

private:
  static constexpr int LayerIndexRole = Qt::UserRole;
  static constexpr int ProviderRole = Qt::UserRole + 1;
  static constexpr int FileBasedRole = Qt::UserRole + 2;
  const QList<QDomNode> &mLayers;
  QTableWidget *mLayerList = nullptr;

private slots:
  void itemClicked(QTableWidgetItem *item);
  void accept() override;
};

#endif // KADASHANDLEBADLAYERS_H
