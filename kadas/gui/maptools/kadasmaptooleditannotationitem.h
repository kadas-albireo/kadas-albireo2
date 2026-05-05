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
#include <functional>

#include <qgis/qgsmaptool.h>
#include <qgis/qgsvector.h>

#include "kadas/core/kadasstatehistory.h"
#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadasattributetypes.h"
#include "kadas/gui/mapitems/kadasmapitem.h"

class KadasAnnotationItemController;
class KadasAnnotationStyleEditor;
class KadasBottomBar;
class KadasFloatingInputWidget;
class QBoxLayout;
class QgsAnnotationItem;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Map tool that creates and/or edits a \c QgsAnnotationItem on a
 *        \c QgsAnnotationLayer, driven by a \c KadasAnnotationItemController.
 *
 * The tool has two entry modes:
 *  - **Edit mode**: started with an existing \a itemId; clicks on vertices
 *    drag them, the styling row in the bottom bar updates the symbol.
 *  - **Create mode**: started with a \a controller and a target \a layer;
 *    clicks place vertices for a fresh item, a right-click / Enter finalizes
 *    the part. After finalization, clicks on existing vertices keep editing
 *    them, while clicks on empty canvas start a brand-new item (single-part).
 *
 * Last-used styling per geometry kind (marker / line / polygon) is persisted
 * via \c QgsSettingsEntry and re-applied to freshly created items.
 */
class KADAS_GUI_EXPORT KadasMapToolEditAnnotationItem : public QgsMapTool
{
    Q_OBJECT
  public:
    //! Edit an existing annotation item in place.
    KadasMapToolEditAnnotationItem( QgsMapCanvas *canvas, QgsAnnotationLayer *layer, const QString &itemId );
    //! Create a new annotation item via \a controller, then keep editing it.
    KadasMapToolEditAnnotationItem( QgsMapCanvas *canvas, KadasAnnotationItemController *controller, QgsAnnotationLayer *layer );

    void activate() override;
    void deactivate() override;

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void canvasDoubleClickEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

    //! Create-mode: when true, finalized parts accumulate on the same item; default false.
    void setMultipart( bool multipart ) { mMultipart = multipart; }

    //! Create-mode: overrides the controller's createItem() factory.
    void setItemFactory( std::function<QgsAnnotationItem *()> factory ) { mItemFactory = std::move( factory ); }

    /**
     * Create-mode: drives the create state machine with an explicit point,
     * as if the user had left-clicked at \a pos. No-op in pure edit mode.
     */
    void addPoint( const QgsPointXY &pos );

  signals:
    //! Create-mode: emitted whenever a part is finalized.
    void partFinished();
    //! Create-mode: emitted after a fresh item replaces the previous one.
    void cleared();

  private:
    enum class DrawState
    {
      Empty,
      InProgress,
      Finished,
    };

    struct ToolState;

    KadasAnnotationItemController *mController = nullptr;
    QPointer<QgsAnnotationLayer> mLayer;
    QString mItemId;
    QgsAnnotationItem *mItem = nullptr;

    bool mAllowCreate = false;
    bool mMultipart = false;
    DrawState mDrawState = DrawState::Finished;
    std::function<QgsAnnotationItem *()> mItemFactory;

    KadasEditContext mEditContext;
    QgsVector mMoveOffset;
    Qt::MouseButton mPressedButton = Qt::NoButton;

    KadasBottomBar *mBottomBar = nullptr;
    KadasStateHistory *mStateHistory = nullptr;
    KadasFloatingInputWidget *mInputWidget = nullptr;
    bool mIgnoreNextMoveEvent = false;

    KadasAnnotationStyleEditor *mStyleEditor = nullptr;

    class HandlesOverlay;
    HandlesOverlay *mHandles = nullptr;

    void refreshHandles();
    void pushState();
    void deleteItem();
    void setupNumericInput();
    void clearNumericInput();
    KadasAttribValues collectAttributeValues() const;

    void setupStyleEditor( QBoxLayout *outer );

    // Create-flow helpers.
    void createInitialItem();
    void clearInProgressItem();
    void startPart( const QgsPointXY &pos );
    void finishPart();

    // Returns the id of an annotation item on mLayer hit by a click at
    // \a mapPos, or an empty string if none.
    QString pickItemAt( const QgsPointXY &mapPos ) const;
    // Switches the tool to edit \a itemId on the current layer, rebuilding
    // the styling row to match the new item's geometry kind.
    void switchToItem( const QString &itemId );
    // Pops up a context menu for \a itemId at the given global screen
    // position with bring-to-front / send-to-back / forward / backward.
    void showContextMenu( const QString &itemId, const QPoint &globalPos );

  private slots:
    void stateChanged( KadasStateHistory::ChangeType, KadasStateHistory::State *state, KadasStateHistory::State *prevState );
    void inputChanged();
};

#endif // KADASMAPTOOLEDITANNOTATIONITEM_H
