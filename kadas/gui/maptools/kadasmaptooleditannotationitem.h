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

class KadasAnnotationItemController;
class KadasAnnotationStyleEditor;
class KadasBottomBar;
class KadasFloatingInputWidget;
class QBoxLayout;
class QgsAnnotationItem;
class QgsAnnotationLayer;
class QgsRubberBand;

/**
 * \ingroup gui
 * \brief Map tool to create/edit a \c QgsAnnotationItem on a \c QgsAnnotationLayer via a \c KadasAnnotationItemController.
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
    void setItemFactory( std::function<QgsAnnotationItem *()> factory ) SIP_SKIP { mItemFactory = std::move( factory ); }

    //! Inserts a custom widget into the bottom-bar top row; call before \c activate(), ownership transfers.
    void setExtraTopWidget( QWidget *widget ) { mExtraTopWidget = widget; }

    //! The annotation item the tool is currently driving (may be null).
    QgsAnnotationItem *currentItem() const { return mItem; }

    //! Create-mode: place a vertex at \a pos as if left-clicked; no-op in edit mode.
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
    QPointer<QWidget> mExtraTopWidget;

    KadasEditContext mEditContext;
    QgsVector mMoveOffset;
    Qt::MouseButton mPressedButton = Qt::NoButton;

    KadasBottomBar *mBottomBar = nullptr;
    KadasStateHistory *mStateHistory = nullptr;
    KadasFloatingInputWidget *mInputWidget = nullptr;
    bool mIgnoreNextMoveEvent = false;

    KadasAnnotationStyleEditor *mStyleEditor = nullptr;

    // Preview band drawn above all layers; the layer is repainted only at commit points, never per mouse move.
    QgsRubberBand *mTempRubberBand = nullptr;

    class HandlesOverlay;
    HandlesOverlay *mHandles = nullptr;

    void refreshHandles();
    void updateTempRubberBand();
    void clearTempRubberBand();
    void pushState();
    void deleteItem();
    void setupNumericInput();
    void clearNumericInput();
    KadasAttribValues collectAttributeValues() const;

    void setupStyleEditor( QBoxLayout *outer );

    void createInitialItem();
    void clearInProgressItem();
    void startPart( const QgsPointXY &pos );
    void finishPart();

    // Result of picking an annotation item across all visible annotation layers.
    struct PickedItem
    {
        QgsAnnotationLayer *layer = nullptr;
        QString itemId;
        bool isEmpty() const { return !layer || itemId.isEmpty(); }
    };
    PickedItem pickItemAt( const QgsPointXY &mapPos ) const;
    void switchToItem( QgsAnnotationLayer *layer, const QString &itemId );
    void showContextMenu( QgsAnnotationLayer *layer, const QString &itemId, const QPoint &globalPos );

  private slots:
    void stateChanged( KadasStateHistory::ChangeType, KadasStateHistory::State *state, KadasStateHistory::State *prevState );
    void inputChanged();
};

#endif // KADASMAPTOOLEDITANNOTATIONITEM_H
