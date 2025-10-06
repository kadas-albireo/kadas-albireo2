/***************************************************************************
    kadasclipboard.h
    ----------------
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

#ifndef KADASCLIPBOARD_H
#define KADASCLIPBOARD_H

#include <QObject>

#include <qgis/qgsfeaturestore.h>

#include "kadas/gui/kadas_gui.h"

class QMimeData;
class QgsPoint;
class KadasMapItem;

class KADAS_GUI_EXPORT KadasPasteHandler {
public:
  virtual ~KadasPasteHandler() {}
  virtual void paste(const QString &mimeData, const QByteArray &data,
                     const QgsPointXY *mapPos) = 0;
};

// Constants used to describe copy-paste MIME types
#define KADASCLIPBOARD_FEATURESTORE_MIME "application/kadas.featurestore"
#define KADASCLIPBOARD_ITEMSTORE_MIME "application/kadas.itemstore"

class KADAS_GUI_EXPORT KadasClipboard : public QObject {
  Q_OBJECT
public:
  static KadasClipboard *instance();

  ~KadasClipboard();

  // Returns whether there is any data in the clipboard
  bool isEmpty() const;

  // Queries whether the clipboard has specified format.
  bool hasFormat(const QString &format) const;

  // Sets the clipboard contents
  void setMimeData(QMimeData *mimeData);

  // Retreives the clipboard contents
  const QMimeData *mimeData();

  // Utility function for storing features in clipboard
  void setStoredFeatures(const QgsFeatureStore &featureStore);

  // Utility function for getting features from clipboard
  const QgsFeatureStore &getStoredFeatures() const { return mFeatureStore; }

  // Utility function fro storing map items in clipboard
  void setStoredMapItems(const QList<KadasMapItem *> &items);

  // Utility function for getting stored map items from clipboard
  const QList<KadasMapItem *> &storedMapItems() const { return mItemStore; }

signals:
  void dataChanged();

protected:
  KadasClipboard();

  QgsFeatureStore mFeatureStore;
  QList<KadasMapItem *> mItemStore;

  void clear();
};

#endif // KADASCLIPBOARD_H
