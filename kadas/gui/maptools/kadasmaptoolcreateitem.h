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
#include <kadas/gui/kadaslayerselectionwidget.h>
#include <kadas/gui/mapitems/kadasmapitem.h>

class KadasBottomBar;
class KadasFloatingInputWidget;
class KadasItemLayer;
#ifndef SIP_RUN
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif
#endif

class KADAS_GUI_EXPORT KadasMapToolCreateItem : public QgsMapTool
{
    Q_OBJECT
  public:
#ifndef SIP_RUN
    typedef std::function<KadasMapItem*() > ItemFactory;
    KadasMapToolCreateItem( QgsMapCanvas *canvas, ItemFactory itemFactory, KadasItemLayer *layer = nullptr );
#else
    KadasMapToolCreateItem( QgsMapCanvas *canvas, SIP_PYCALLABLE itemFactory, KadasItemLayer *layer = nullptr );
    % MethodCode

    // Make sure the callable doesn't get garbage collected, this is needed because refcount for a1 is 0
    // and the creation function pointer is passed to the metadata and it needs to be kept in memory.
    Py_INCREF( a1 );

    Py_BEGIN_ALLOW_THREADS

    sipCpp = new sipKadasMapToolCreateItem( a0, static_cast<PyObject *>( nullptr ), a2 );
    sipCpp->setItemFactory( [a1]( ) -> KadasMapItem*
    {
      KadasMapItem *res;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a1, NULL );
      int state;
      int sipIsError = 0;
      res = reinterpret_cast<KadasMapItem *>( sipConvertToType( s, sipType_KadasMapItem, 0, SIP_NOT_NONE, &state, &sipIsError ) );
      SIP_UNBLOCK_THREADS
      return res;

    } );

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

    void setMultipart( bool multipart ) { mMultipart = multipart; }
    void setSnappingEnabled( bool snapping ) { mSnapping = snapping; }
#ifndef SIP_RUN
    void showLayerSelection( bool enabled, KadasLayerSelectionWidget::LayerFilter filter, KadasLayerSelectionWidget::LayerCreator creator = nullptr );
#endif


#ifndef SIP_RUN
    void setItemFactory( ItemFactory itemFactory ) { mItemFactory = itemFactory; }
#endif

  public slots:
    void clear();

  signals:
    void cleared();
    void partFinished();

  protected:
#ifndef SIP_RUN
    // TODO: Prettier way?!
    KadasMapToolCreateItem( QgsMapCanvas *canvas, PyObject *pyCallable, KadasItemLayer *layer = nullptr );
#endif

    void createItem();
    void addPoint( const KadasMapPos &mapPos );
    void startPart( const KadasMapPos &pos );
    void startPart( const KadasMapItem::AttribValues &attributes );
    void finishPart();
    void addPartFromGeometry( const QgsAbstractGeometry &geom, const QgsCoordinateReferenceSystem &crs );
    void commitItem();
    KadasMapPos transformMousePoint( QgsPointXY mapPos ) const;
    KadasMapItem::AttribValues collectAttributeValues() const;
    KadasMapItem *mutableItem() { return mItem; }

  private:
    ItemFactory mItemFactory = nullptr;
    KadasMapItem *mItem = nullptr;
    KadasItemLayer *mLayer = nullptr;

    KadasStateHistory *mStateHistory = nullptr;
    KadasFloatingInputWidget *mInputWidget = nullptr;
    KadasMapItem::AttribDefs mDrawAttribs;
    bool mIgnoreNextMoveEvent = false;

    KadasBottomBar *mBottomBar = nullptr;
    KadasMapItemEditor *mEditor = nullptr;

    bool mShowLayerSelection = false;
    KadasLayerSelectionWidget::LayerFilter mLayerSelectionFilter = nullptr;
    KadasLayerSelectionWidget::LayerCreator mLayerCreator = nullptr;

    bool mMultipart = false;
    bool mSnapping = false;

  private slots:
    void inputChanged();
    void acceptInput();
    void stateChanged( KadasStateHistory::State *state );
    void setTargetLayer( QgsMapLayer *layer );

};

#endif // KADASMAPTOOLCREATEITEM_H
