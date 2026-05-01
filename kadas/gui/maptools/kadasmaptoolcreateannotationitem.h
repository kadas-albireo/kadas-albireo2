/***************************************************************************
    kadasmaptoolcreateannotationitem.h
    ----------------------------------
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

#ifndef KADASMAPTOOLCREATEANNOTATIONITEM_H
#define KADASMAPTOOLCREATEANNOTATIONITEM_H

#include <QPointer>
#include <QString>
#include <functional>

#include <qgis/qgsmaptool.h>

#include "kadas/core/kadasstatehistory.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/mapitems/kadasmapitem.h"

class KadasAnnotationItemController;
class KadasBottomBar;
class KadasFloatingInputWidget;
class QgsAnnotationItem;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Map tool that creates a new \c QgsAnnotationItem on a
 *        \c QgsAnnotationLayer, driven by a \c KadasAnnotationItemController.
 *
 * Equivalent of the legacy \c KadasMapToolCreateItem but speaks the
 * controller / annotation-layer API.  The controller is looked up from the
 * \c KadasAnnotationControllerRegistry by item type id and is used to drive
 * the draw state machine (start / continue / set-current / end) and the
 * KadasMapPos ↔ item-CRS transforms.
 *
 * The in-progress item is added to the target annotation layer immediately
 * so that QGIS's standard annotation rendering shows it during drawing.
 * Undo/redo replaces the item in place via \c QgsAnnotationLayer::replaceItem.
 * If the user cancels (Esc / right-click on an empty draw) before finishing
 * a part, the item is removed from the layer on deactivate.
 */
class KADAS_GUI_EXPORT KadasMapToolCreateAnnotationItem : public QgsMapTool
{
    Q_OBJECT
  public:
    KadasMapToolCreateAnnotationItem( QgsMapCanvas *canvas, KadasAnnotationItemController *controller, QgsAnnotationLayer *layer );
    ~KadasMapToolCreateAnnotationItem() override;

    void activate() override;
    void deactivate() override;

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

    void setMultipart( bool multipart ) { mMultipart = multipart; }
    void setToolLabel( const QString &label ) { mToolLabel = label; }

    /**
     * Overrides the default \c controller->createItem() factory used by the
     * tool. When set, this factory is invoked instead of the controller to
     * obtain a fresh in-progress item; the resulting item must report a
     * \c type() compatible with the bound controller. Use this to create
     * pre-configured variants (e.g. a marker with a non-default symbol
     * shape) without subclassing the controller.
     */
    void setItemFactory( std::function<QgsAnnotationItem *()> factory ) { mItemFactory = std::move( factory ); }

    //! Returns the in-progress item (owned by the target layer), or nullptr.
    QgsAnnotationItem *currentItem() const { return mItem; }

    //! Returns the id under which \c currentItem() is registered on the
    //! target layer, or an empty string if no item is currently bound.
    QString currentItemId() const { return mItemId; }

  signals:
    //! Emitted whenever a part is finished (single-part mode: the only part).
    void partFinished();

    //! Emitted after a fresh item replaces the previous one (multi-part flow).
    void cleared();

  private:
    enum class DrawState
    {
      Empty,
      Drawing,
      Finished,
    };

    struct ToolState : KadasStateHistory::State
    {
        ToolState( QgsAnnotationItem *clone, DrawState ds )
          : itemClone( clone )
          , drawState( ds )
        {}
        ~ToolState() override;
        QgsAnnotationItem *itemClone = nullptr;
        DrawState drawState = DrawState::Empty;
    };

    KadasAnnotationItemController *mController = nullptr;
    QPointer<QgsAnnotationLayer> mLayer;

    QgsAnnotationItem *mItem = nullptr;
    QString mItemId;
    DrawState mDrawState = DrawState::Empty;

    KadasBottomBar *mBottomBar = nullptr;
    KadasStateHistory *mStateHistory = nullptr;
    KadasFloatingInputWidget *mInputWidget = nullptr;
    bool mIgnoreNextMoveEvent = false;

    bool mMultipart = false;
    QString mToolLabel;
    std::function<QgsAnnotationItem *()> mItemFactory;

    void createItem();

  public:
    /**
     * Adds a point to the in-progress item. Public so that callers (e.g. the
     * canvas context menu) can seed an active create tool with a starting
     * vertex right after triggering its action.
     */
    void addPoint( const KadasMapPos &pos );

  private:
    void startPart( const KadasMapPos &pos );
    void startPart( const KadasMapItem::AttribValues &values );
    void finishPart();
    void pushState();
    void clearInProgress();
    void setupNumericInputWidget();
    KadasMapItem::AttribValues collectAttributeValues() const;

  private slots:
    void stateChanged( KadasStateHistory::ChangeType changeType, KadasStateHistory::State *state, KadasStateHistory::State *prevState );
    void inputChanged();
    void acceptInput();
};

#endif // KADASMAPTOOLCREATEANNOTATIONITEM_H
