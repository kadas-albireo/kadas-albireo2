/***************************************************************************
    kadassearchprovider.h
    ---------------------
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

#ifndef KADASSEARCHPROVIDER_H
#define KADASSEARCHPROVIDER_H

#include <QObject>

#include <qgis/qgsgeometry.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/kadas_gui.h"

class QgsMapCanvas;

class KADAS_GUI_EXPORT KadasSearchProvider : public QObject {
  Q_OBJECT
public:
  struct SearchResult {
    QString category;
    /**
     * Lower number means higher precedence.
     * 1: coordinate
     * 2: pins
     * 10: local features
     * 11: remote features
     * 20: municipalities
     * 21: cantons
     * 22: districts
     * 23: places
     * 24: plz codes
     * 25: addresses
     * 30: world locations
     * 100: unknown
     */
    int categoryPrecedence;
    QString text;
    QgsPointXY pos;
    QgsRectangle bbox;
    QString crs;
    double zoomScale;
    bool showPin;
    bool fuzzy = false;
    QString geometry;
  };
  struct SearchRegion {
    QgsPolylineXY polygon;
    QString crs;
  };

  KadasSearchProvider(QgsMapCanvas *mapCanvas) : mMapCanvas(mapCanvas) {}
  virtual ~KadasSearchProvider() {}
  virtual void startSearch(const QString &searchtext,
                           const SearchRegion &searchRegion) = 0;
  virtual void cancelSearch() {}

protected:
  QgsMapCanvas *mMapCanvas;
};

Q_DECLARE_METATYPE(KadasSearchProvider::SearchResult)

#endif // KADASSEARCHPROVIDER_H
