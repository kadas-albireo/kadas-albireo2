/***************************************************************************
    kadasbookmarksmenu.h
    --------------------
    copyright            : (C) 2021 by Sandro Mani
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

#ifndef KADASBOOKMARKSMENU_H
#define KADASBOOKMARKSMENU_H

#include <QMenu>

#include <qgis/qgsrectangle.h>

#include "kadas/gui/kadas_gui.h"

class QDomDocument;
class QgsLayerTreeView;
class QgsMapCanvas;
class QgsMessageBar;


class KADAS_GUI_EXPORT KadasBookmarksMenu : public QMenu
{
    Q_OBJECT
  public:
    KadasBookmarksMenu( QgsMapCanvas *canvas, QgsMessageBar *messageBar, QWidget *parent = nullptr );
    ~KadasBookmarksMenu();

  private:
    struct Bookmark
    {
      QString name;
      QString crs;
      QgsRectangle extent;

      // for backward compatibility
      // it's easier to keep the old structure until it's activated again
      QMap<QString, bool> layerVisibilities;
      QMap<QString, bool> groupVisibilities;
    };
    QList<Bookmark *> mBookmarks;
    QgsMapCanvas *mCanvas = nullptr;
    QgsMessageBar *mMessageBar = nullptr;

    void clearMenu();
    void addBookmarkAction( Bookmark *bookmark );

  private slots:
    void addBookmark();
    void restoreBookmark(Bookmark *bookmark );
    void deleteBookmark( QAction *action, Bookmark *bookmark );

    void saveToProject( QDomDocument &doc );
    void restoreFromProject( const QDomDocument &doc );
};

#endif // KADASBOOKMARKSMENU_H
