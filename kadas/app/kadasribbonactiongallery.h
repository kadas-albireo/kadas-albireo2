/***************************************************************************
    kadasribbonactiongallery.h
    --------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASRIBBONACTIONGALLERY_H
#define KADASRIBBONACTIONGALLERY_H

#include <QSize>
#include <QWidget>

class QAction;
class QGridLayout;

/**
 * A flat, single-level gallery of action tiles laid out in a grid, with
 * optional titled sections. Designed to be embedded in a QMenu (through a
 * QWidgetAction) so a ribbon dropdown can present all its choices at once,
 * without the nested submenus it replaces.
 */
class KadasRibbonActionGallery : public QWidget
{
    Q_OBJECT
  public:
    explicit KadasRibbonActionGallery( int columns = 3, QWidget *parent = nullptr );

    //! Starts a new titled section. Tiles added afterwards appear below it.
    void addSection( const QString &title );

    //! Adds an action tile to the current section.
    void addActionTile( QAction *action );

    //! Size of the icon shown on each tile.
    void setTileIconSize( const QSize &size ) { mTileIconSize = size; }

    //! Fixed size of each tile.
    void setTileSize( const QSize &size ) { mTileSize = size; }

  signals:
    //! Emitted when a tile is activated; used to dismiss the host menu.
    void actionTriggered( QAction *action );

  private:
    QGridLayout *mGrid = nullptr;
    int mColumns = 3;
    int mRow = 0;
    int mCol = 0;
    QSize mTileIconSize = QSize( 38, 38 );
    QSize mTileSize = QSize( 92, 70 );

    void finishPartialRow();
};

#endif // KADASRIBBONACTIONGALLERY_H
