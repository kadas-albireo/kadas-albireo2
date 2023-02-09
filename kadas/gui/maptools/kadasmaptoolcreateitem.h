/***************************************************************************
    kadasmaptoolcreateitem.h
    ------------------------
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

#ifndef KADASMAPTOOLCREATEITEM_H
#define KADASMAPTOOLCREATEITEM_H

#include <qgis/qgsmaptool.h>

#include <kadas/core/kadasstatehistory.h>
#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadasitemlayer.h>
#include <kadas/gui/kadaslayerselectionwidget.h>
#include <kadas/gui/mapitems/kadasmapitem.h>

class KadasBottomBar;
class KadasFloatingInputWidget;
class KadasItemLayer;

class KADAS_GUI_EXPORT KadasMapToolCreateItem : public QgsMapTool
{
    Q_OBJECT
  public:
#ifndef SIP_RUN
    typedef std::function<KadasMapItem*() > ItemFactory;
    KadasMapToolCreateItem( QgsMapCanvas *canvas, ItemFactory itemFactory, KadasItemLayer *layer = nullptr );
#else
    KadasMapToolCreateItem( QgsMapCanvas *canvas, SIP_PYCALLABLE itemFactory, KadasItemLayer *layer = nullptr )[( QgsMapCanvas *, ItemFactory, KadasItemLayer * )];
    % MethodCode

    // Make sure the callable doesn't get garbage collected, this is needed because refcount for a1 is 0
    // and the creation function pointer is passed to the metadata and it needs to be kept in memory.
    Py_INCREF( a1 );

    Py_BEGIN_ALLOW_THREADS

    auto factory = [a1]() -> KadasMapItem *
    {
      KadasMapItem *result = nullptr;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a1, NULL );
      if ( s )
      {
        int state;
        int sipIsError = 0;
        result = reinterpret_cast<KadasMapItem *>( sipConvertToType( s, sipType_KadasMapItem, NULL, SIP_NOT_NONE, &state, &sipIsError ) );
        sipReleaseType( result, sipType_KadasMapItem, state );
      }
      SIP_UNBLOCK_THREADS
      return result;
    };

    sipCpp = new sipKadasMapToolCreateItem( a0, factory, a2 );

    Py_END_ALLOW_THREADS

    % End
#endif
    KadasMapToolCreateItem( QgsMapCanvas *canvas, KadasMapItem *item, KadasItemLayer *layer = nullptr );
    ~KadasMapToolCreateItem();

    void activate() override;
    void deactivate() override;

    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;
    void keyPressEvent( QKeyEvent *e ) override;

    const KadasMapItem *currentItem() const { return mItem; }
    KadasMapItem *takeItem();
    const KadasMapItemEditor *currentEditor() const { return mEditor; }

    void setMultipart( bool multipart ) { mMultipart = multipart; }
    void setSnappingEnabled( bool snapping ) { mSnapping = snapping; }
    void setSelectItems( bool select ) { mSelectItems = select; }
    void setToolLabel( const QString &label ) { mToolLabel = label; }
    void setUndoRedoVisible( bool undoRedoVisible ) { mUndoRedoVisible = undoRedoVisible; }
    void setExtraBottomBarContents( QWidget *widget );
#ifndef SIP_RUN
    void setItemFactory( ItemFactory itemFactory );
#else
    void setItemFactory( SIP_PYCALLABLE itemFactory );
    % MethodCode

    // Make sure the callables doesn't get garbage collected
    // and the creation function pointer is passed to the metadata and it needs to be kept in memory.
    Py_INCREF( a0 );

    Py_BEGIN_ALLOW_THREADS

    auto factory = [a0]() -> KadasMapItem *
    {
      KadasMapItem *result = nullptr;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a0, NULL );
      if ( s )
      {
        int state;
        int sipIsError = 0;
        result = reinterpret_cast<KadasMapItem *>( sipConvertToType( s, sipType_KadasMapItem, NULL, SIP_NOT_NONE, &state, &sipIsError ) );
        sipReleaseType( result, sipType_KadasMapItem, state );
      }
      SIP_UNBLOCK_THREADS
      return result;
    };

    sipCpp->setItemFactory( factory );
    Py_END_ALLOW_THREADS

    % End
#endif
#ifndef SIP_RUN
    void showLayerSelection( bool enabled, QgsLayerTreeView *layerTreeView, KadasLayerSelectionWidget::LayerFilter filter, KadasLayerSelectionWidget::LayerCreator creator = nullptr );
#else
    // TODO: creator should be optional
    void showLayerSelection( bool enabled, QgsLayerTreeView *layerTreeView, SIP_PYCALLABLE filter, SIP_PYCALLABLE creator );
    % MethodCode

    // Make sure the callables doesn't get garbage collected, this is needed because refcount for a1/a2 is 0
    // and the creation function pointer is passed to the metadata and it needs to be kept in memory.
    Py_INCREF( a2 );
    Py_INCREF( a3 );

    Py_BEGIN_ALLOW_THREADS

    auto layerFilter = [a2]( QgsMapLayer *layer ) -> bool
    {
      bool result = false;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a2, "D", layer, sipType_QgsMapLayer, NULL );
      if ( s )
      {
        result = sipConvertToBool( s );
      }
      SIP_UNBLOCK_THREADS
      return result;
    };

    auto layerCreator = [a3]( const QString &name ) -> QgsMapLayer *
    {
      QgsMapLayer *result = nullptr;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a3, "D", &name, sipType_QString, NULL );
      if ( s )
      {
        int state;
        int sipIsError = 0;
        result = reinterpret_cast<QgsMapLayer *>( sipConvertToType( s, sipType_QgsMapLayer, NULL, SIP_NOT_NONE, &state, &sipIsError ) );
        sipReleaseType( result, sipType_QgsMapLayer, state );
      }
      SIP_UNBLOCK_THREADS
      return result;
    };

    sipCpp->showLayerSelection( a0, a1, layerFilter, layerCreator );
    Py_END_ALLOW_THREADS

    % End
#endif
    void addPartFromGeometry( const QgsAbstractGeometry &geom, const QgsCoordinateReferenceSystem &crs );
    void addPoint( const KadasMapPos &mapPos );

  public slots:
    void clear();

  signals:
    void cleared();
    void partFinished();
    void targetLayerChanged( QgsMapLayer *layer );

  protected:
    void createItem();
    void startPart( const KadasMapPos &pos );
    void startPart( const KadasMapItem::AttribValues &attributes );
    void finishPart();
    void commitItem();
    KadasMapPos transformMousePoint( QgsPointXY mapPos ) const;
    KadasMapItem::AttribValues collectAttributeValues() const;
    KadasMapItem *mutableItem() { return mItem; }

  private:
    QgsLayerTreeView *mLayerTreeView = nullptr;
    ItemFactory mItemFactory = nullptr;
    KadasMapItem *mItem = nullptr;
    KadasItemLayer *mLayer = nullptr;

    struct ItemData
    {
      KadasItemLayer::ItemId itemId = KadasItemLayer::ITEM_ID_NULL;
      QMap<QString, QVariant> props;
    };
    struct ToolState : KadasStateHistory::State
    {
      ToolState( State *_itemState, QSharedPointer<ItemData> _itemData ) : itemState( _itemState ), itemData( _itemData ) {}
      ~ToolState() { delete itemState; }
      State *itemState = nullptr;
      QSharedPointer<ItemData> itemData;
    };
    KadasStateHistory *mStateHistory = nullptr;
    QSharedPointer<ItemData> mCurrentItemData;

    KadasFloatingInputWidget *mInputWidget = nullptr;
    KadasMapItem::AttribDefs mDrawAttribs;
    bool mIgnoreNextMoveEvent = false;

    KadasBottomBar *mBottomBar = nullptr;
    KadasMapItemEditor *mEditor = nullptr;
    QWidget *mBottomBarExtra = nullptr;

    bool mShowLayerSelection = false;
    KadasLayerSelectionWidget::LayerFilter mLayerSelectionFilter = nullptr;
    KadasLayerSelectionWidget::LayerCreator mLayerCreator = nullptr;
    QString mToolLabel;

    bool mMultipart = false;
    bool mSnapping = false;
    bool mSelectItems = true;
    bool mUndoRedoVisible = true;

    void setupNumericInputWidget();

  private slots:
    void inputChanged();
    void acceptInput();
    void stateChanged( KadasStateHistory::ChangeType changeType, KadasStateHistory::State *state, KadasStateHistory::State *prevState );
    void setTargetLayer( QgsMapLayer *layer );
    void storeItemProps();

};

#endif // KADASMAPTOOLCREATEITEM_H
