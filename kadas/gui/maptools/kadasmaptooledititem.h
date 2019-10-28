/***************************************************************************
    kadasmaptooledititem.h
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

#ifndef KADASMAPTOOLEDITITEM_H
#define KADASMAPTOOLEDITITEM_H

#include <qgis/qgsmaptool.h>

#include <kadas/core/kadasstatehistory.h>
#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/mapitems/kadasmapitem.h>

class KadasBottomBar;
class KadasFloatingInputWidget;
class KadasItemLayer;


class KADAS_GUI_EXPORT KadasMapToolEditItem : public QgsMapTool
{
    Q_OBJECT
  public:
    KadasMapToolEditItem( QgsMapCanvas *canvas, const QString &itemId, KadasItemLayer *layer );
    KadasMapToolEditItem( QgsMapCanvas *canvas, KadasMapItem *item, KadasItemLayer *layer );

    void activate() override;
    void deactivate() override;

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

  private:
    KadasStateHistory *mStateHistory = nullptr;
    KadasBottomBar *mBottomBar = nullptr;
    KadasItemLayer *mLayer = nullptr;
    KadasMapItem *mItem = nullptr;
    KadasMapItem::EditContext mEditContext;

    KadasFloatingInputWidget *mInputWidget = nullptr;
    KadasMapItemEditor *mEditor = nullptr;
    bool mIgnoreNextMoveEvent = false;
    QgsVector mMoveOffset;

    KadasMapItem::AttribValues collectAttributeValues() const;
    void setupNumericInput();
    void clearNumericInput();

  private slots:
    void inputChanged();
    void stateChanged( KadasStateHistory::State *state );
    void copyItem();
    void cutItem();
    void deleteItem();
};

#endif // KADASMAPTOOLEDITITEM_H
