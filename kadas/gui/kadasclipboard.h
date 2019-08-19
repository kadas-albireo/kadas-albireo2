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

#include <kadas/gui/kadas_gui.h>

class QMimeData;
class QgsPoint;

class KADAS_GUI_EXPORT KadasPasteHandler
{
public:
  virtual ~KadasPasteHandler() {}
  virtual void paste ( const QString& mimeData, const QByteArray& data, const QgsPointXY* mapPos ) = 0;
};

// Constants used to describe copy-paste MIME types
#define KADASCLIPBOARD_STYLE_MIME "application/qgis.style"
#define KADASCLIPBOARD_FEATURESTORE_MIME "application/qgis.featurestore"

class KADAS_GUI_EXPORT KadasClipboard : public QObject
{
  Q_OBJECT
public:
  KadasClipboard ( QObject* parent = 0 );

  // Returns whether there is any data in the clipboard
  bool isEmpty() const;

  // Queries whether the clipboard has specified format.
  bool hasFormat ( const QString& format ) const;

  // Sets the clipboard contents
  void setMimeData ( QMimeData* mimeData );

  // Retreives the clipboard contents
  const QMimeData* mimeData();

  // Utility function for storing features in clipboard
  void setStoredFeatures ( const QgsFeatureStore& featureStore );

  // Utility function for getting features from clipboard
  const QgsFeatureStore& getStoredFeatures() const;

signals:
  void dataChanged();

private:
  QgsFeatureStore mFeatureStore;

private slots:
  void onDataChanged();
};

#endif // KADASCLIPBOARD_H
