/***************************************************************************
    kadasarcgisrestcatalogprovider.h
    --------------------------------
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

#ifndef KADASARCGISRESTCATALOGPROVIDER_H
#define KADASARCGISRESTCATALOGPROVIDER_H

#include <QStringList>

#include "kadas/gui/kadascatalogprovider.h"

class QStandardItem;

class KADAS_GUI_EXPORT KadasArcGisRestCatalogProvider
    : public KadasCatalogProvider {
  Q_OBJECT
public:
  KadasArcGisRestCatalogProvider(const QString &baseUrl,
                                 KadasCatalogBrowser *browser,
                                 const QMap<QString, QString> &params);
  void load() override;

private:
  QString mBaseUrl;
  int mPendingTasks;

  void endTask();
  void parseFolder(const QString &path,
                   const QStringList &catTitles = QStringList());
  void parseService(const QString &path, const QStringList &catTitles);
  void parseWMTS(const QString &path, const QStringList &catTitles);
  void parseWMS(const QString &path, const QStringList &catTitles);

private slots:
  void parseFolderDo();
  void parseServiceDo();
  void parseWMTSDo();
  void parseWMSDo();
};

#endif // KADASARCGISRESTCATALOGPROVIDER_H
