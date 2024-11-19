/***************************************************************************
    kadasgeoadmincatalogprovider.h
    -------------------------------
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

#ifndef KADASGEOADMINRESTCATALOGPROVIDER_H
#define KADASGEOADMINRESTCATALOGPROVIDER_H

#include "kadas/gui/kadascatalogprovider.h"

class QStandardItem;

class KADAS_GUI_EXPORT KadasGeoAdminRestCatalogProvider : public KadasCatalogProvider
{
    Q_OBJECT
  public:
    KadasGeoAdminRestCatalogProvider( const QString &baseUrl, KadasCatalogBrowser *browser, const QMap<QString, QString> &params );
    void load() override;
  private slots:
    void replyFinished();

  private:
    QString mBaseUrl;

    void parseTheme( QStandardItem *parent, const QDomElement &theme, QMap<QString, QStandardItem *> &layerParentMap );
};

#endif // KADASGEOADMINRESTCATALOGPROVIDER_H
