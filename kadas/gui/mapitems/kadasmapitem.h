/***************************************************************************
    kadasmapitem.h
    --------------
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

#ifndef KADASMAPITEM_H
#define KADASMAPITEM_H

#include <QObject>
#include <QWidget>

#include <qgis/qgsabstractgeometry.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgsrectangle.h>

#include <kadas/core/kadasstatehistory.h>
#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>

class QgsRenderContext;
struct QgsVertexId;
class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapItem : public QObject SIP_ABSTRACT
{
    Q_OBJECT
  public:
    KadasMapItem( const QgsCoordinateReferenceSystem &crs, QObject *parent );
    ~KadasMapItem();

    /* The item crs */
    const QgsCoordinateReferenceSystem &crs() const { return mCrs; }

    /* Bounding box in geographic coordinates */
    virtual QgsRectangle boundingBox() const = 0;

    /* Margin in screen units */
    virtual QRect margin() const { return QRect(); }

    /* Nodes for editing */
    struct Node
    {
      QgsPointXY pos;
#ifndef SIP_RUN
      typedef void ( *node_renderer_t )( QPainter *, const QgsPointXY &, int );
      node_renderer_t render = defaultNodeRenderer;
#else
      // TODO
#endif
    };

    virtual QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const = 0;

    /* Hit test, rect in item crs */
    virtual bool intersects( const QgsRectangle &rect, const QgsMapSettings &settings ) const = 0;

    /* Render the item */
    virtual void render( QgsRenderContext &context ) const = 0;

    /* Associate to layer */
    void associateToLayer( QgsMapLayer *layer );
    QgsMapLayer *associatedLayer() const { return mAssociatedLayer; }

    /* Selected state */
    void setSelected( bool selected );
    bool selected() const { return mSelected; }

    /* z-index */
    void setZIndex( int zIndex );
    int zIndex() const { return mZIndex; }

    // State interface
    struct State : KadasStateHistory::State
    {
      enum DrawStatus { Empty, Drawing, Finished };
      DrawStatus drawStatus = Empty;
      virtual void assign( const State *other ) = 0;
      virtual State *clone() const = 0 SIP_FACTORY;
    };
    const State *constState() const { return mState; }
    virtual void setState( const State *state );

    struct NumericAttribute
    {
      enum Type {XCooAttr, YCooAttr, DistanceAttr, OtherAttr};
      QString name;
      Type type;
      double min = std::numeric_limits<double>::lowest();
      double max = std::numeric_limits<double>::max();
      int decimals = 0;
    };
    typedef QMap<int, KadasMapItem::NumericAttribute> AttribDefs;
    typedef QMap<int, double> AttribValues;

    // Draw interface (all coordinates in item crs, attribute distances in meters)
    virtual void clear();
    virtual bool startPart( const QgsPointXY &firstPoint, const QgsMapSettings &mapSettings ) = 0;
    virtual bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;
    virtual void setCurrentPoint( const QgsPointXY &p, const QgsMapSettings &mapSettings ) = 0;
    virtual void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;
    virtual bool continuePart( const QgsMapSettings &mapSettings ) = 0;
    virtual void endPart() = 0;

    virtual AttribDefs drawAttribs() const = 0;
    virtual AttribValues drawAttribsFromPosition( const QgsPointXY &pos ) const = 0;
    virtual QgsPointXY positionFromDrawAttribs( const AttribValues &values ) const = 0;

    // Edit interface (all coordinates in item crs, attribute distances in meters)
    struct EditContext
    {
      EditContext( const QgsVertexId &_vidx = QgsVertexId(), const QgsPointXY &_pos = QgsPointXY(), const AttribDefs &_attributes = KadasMapItem::AttribDefs(), Qt::CursorShape _cursor = Qt::CrossCursor )
        : vidx( _vidx )
        , pos( _pos )
        , attributes( _attributes )
        , cursor( _cursor )
      {
      }
      QgsVertexId vidx;
      QgsPointXY pos;
      AttribDefs attributes;
      Qt::CursorShape cursor;
      bool isValid() const { return vidx.isValid(); }
      bool operator== ( const EditContext &other ) const { return vidx == other.vidx; }
      bool operator!= ( const EditContext &other ) const { return vidx != other.vidx; }
    };
    virtual EditContext getEditContext( const QgsPointXY &pos, const QgsMapSettings &mapSettings ) const = 0;
    virtual void edit( const EditContext &context, const QgsPointXY &newPoint, const QgsMapSettings &mapSettings ) = 0;
    virtual void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;

    virtual AttribValues editAttribsFromPosition( const EditContext &context, const QgsPointXY &pos ) const = 0;
    virtual QgsPointXY positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const = 0;

    // Editor
#ifndef SIP_RUN
    typedef std::function<KadasMapItemEditor* ( KadasMapItem *, KadasMapItemEditor::EditorType ) > EditorFactory;
    void setEditorFactory( EditorFactory factory ) { mEditorFactory = factory; }
    EditorFactory getEditorFactory() const { return mEditorFactory; }
#else
    void setEditorFactory( SIP_PYCALLABLE factory / AllowNone / );
    % MethodCode

    Py_BEGIN_ALLOW_THREADS

    sipCpp->setEditorFactory( [a0]( KadasMapItem *v, KadasMapItemEditor::EditorType type )->KadasMapItemEditor*
    {
      KadasMapItemEditor *res;
      SIP_BLOCK_THREADS
      PyObject *s = sipCallMethod( NULL, a0, "Di", v, sipType_KadasMapItem, type, NULL );
      int state;
      int sipIsError = 0;
      res = reinterpret_cast<KadasMapItemEditor *>( sipConvertToType( s, sipType_KadasMapItemEditor, 0, SIP_NOT_NONE, &state, &sipIsError ) );
      SIP_UNBLOCK_THREADS
      return res;
    } );

    Py_END_ALLOW_THREADS
    % End

    SIP_PYCALLABLE getEditorFactory() const;
    % MethodCode
    // The callable, if any,  is held in the user object.
    sipRes = sipGetUserObject( ( sipSimpleWrapper * )sipSelf );
    Py_XINCREF( sipRes );
    % End
#endif

  signals:
    void aboutToBeDestroyed();
    void changed();

  protected:
    State *mState = nullptr;
    QgsCoordinateReferenceSystem mCrs;
    bool mSelected = false;
    int mZIndex = 0;
    QgsMapLayer *mAssociatedLayer = nullptr;

    static void defaultNodeRenderer( QPainter *painter, const QgsPointXY &screenPoint, int nodeSize );
    static void anchorNodeRenderer( QPainter *painter, const QgsPointXY &screenPoint, int nodeSize );

  protected:
    void update();

  private:
    EditorFactory mEditorFactory = nullptr;
};

#endif // KADASMAPITEM_H
