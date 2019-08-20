/***************************************************************************
    kadasmapwidgetmanager.h
    -----------------------
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

#ifndef KADASMAPWIDGETMANAGER_H
#define KADASMAPWIDGETMANAGER_H

#include <QObject>
#include <QPointer>

class QAction;
class QDomDocument;
class QMainWindow;
class QgsMapCanvas;
class KadasMapWidget;

class KadasMapWidgetManager : public QObject
{
  Q_OBJECT
public:
  KadasMapWidgetManager ( QgsMapCanvas* masterCanvas, QMainWindow* parent = 0 );
  ~KadasMapWidgetManager();

public slots:
  KadasMapWidget* addMapWidget ( const QString& id );
  KadasMapWidget* addMapWidget() { return addMapWidget ( QString() ); }
  void removeMapWidget ( const QString& id );
  void clearMapWidgets();

private:
  QMainWindow* mMainWindow;
  QgsMapCanvas* mMasterCanvas;
  QList<KadasMapWidget*> mMapWidgets;
  QAction* mActionAddMapWidget;

private slots:
  void mapWidgetDestroyed();
  void writeProjectSettings ( QDomDocument& doc );
  void readProjectSettings ( const QDomDocument& doc );
};

#endif // KADASMAPWIDGETMANAGER_H
