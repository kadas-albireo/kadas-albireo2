/***************************************************************************
    kadasmaptooleditannotationitem.h
    --------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASMAPTOOLEDITANNOTATIONITEM_H
#define KADASMAPTOOLEDITANNOTATIONITEM_H

#include <QPointer>
#include <QString>

#include <qgis/qgsmaptool.h>
#include <qgis/qgsvector.h>

#include "kadas/core/kadasstatehistory.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadasattributetypes.h"
#include "kadas/gui/mapitems/kadasmapitem.h"

class KadasAnnotationItemController;
class KadasBottomBar;
class KadasFloatingInputWidget;
class QgsAnnotationItem;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Map tool that edits an existing \c QgsAnnotationItem on a
 *        \c QgsAnnotationLayer, driven by a \c KadasAnnotationItemController.
 *
 * Equivalent of the legacy \c KadasMapToolEditItem. The item stays on the
 * target layer at all times; edits mutate it in place and undo/redo
 * snapshots are stored as cloned items, restored via
 * \c QgsAnnotationLayer::replaceItem.
 */
class KADAS_GUI_EXPORT KadasMapToolEditAnnotationItem : public QgsMapTool
{
    Q_OBJECT
  public:
    KadasMapToolEditAnnotationItem( QgsMapCanvas *canvas, QgsAnnotationLayer *layer, const QString &itemId );

    void activate() override;
    void deactivate() override;

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void canvasDoubleClickEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

  private:
    struct ToolState : KadasStateHistory::State
    {
        explicit ToolState( QgsAnnotationItem *clone )
          : itemClone( clone )
        {}
        ~ToolState() override;
        QgsAnnotationItem *itemClone = nullptr;
    };

    KadasAnnotationItemController *mController = nullptr;
    QPointer<QgsAnnotationLayer> mLayer;
    QString mItemId;
    QgsAnnotationItem *mItem = nullptr;

    KadasEditContext mEditContext;
    QgsVector mMoveOffset;
    Qt::MouseButton mPressedButton = Qt::NoButton;

    KadasBottomBar *mBottomBar = nullptr;
    KadasStateHistory *mStateHistory = nullptr;
    KadasFloatingInputWidget *mInputWidget = nullptr;
    bool mIgnoreNextMoveEvent = false;

    void pushState();
    void deleteItem();
    void setupNumericInput();
    void clearNumericInput();
    KadasAttribValues collectAttributeValues() const;

  private slots:
    void stateChanged( KadasStateHistory::ChangeType, KadasStateHistory::State *state, KadasStateHistory::State *prevState );
    void inputChanged();
};

#endif // KADASMAPTOOLEDITANNOTATIONITEM_H
