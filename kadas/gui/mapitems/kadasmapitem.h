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

class QMenu;
class QuaZip;
class QgsRenderContext;
struct QgsVertexId;
class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapPos
{
  public:
    static KadasMapPos fromPoint( const QgsPointXY &pos ) { return KadasMapPos( pos.x(), pos.y() ); }

    KadasMapPos( double x = 0., double y = 0. ) : mX( x ), mY( y ) {}
    double x() const { return mX; }
    void setX( double x ) { mX = x; }
    double y() const { return mY; }
    void setY( double y ) { mY = y; }
    operator QgsPointXY() const { return QgsPointXY( mX, mY ); }
    double sqrDist( const KadasMapPos &p ) const { return ( mX - p.mX ) * ( mX - p.mX ) + ( mY - p.mY ) * ( mY - p.mY ); }
  private:
    double mX = 0.;
    double mY = 0.;
};

class KADAS_GUI_EXPORT KadasMapRect
{
  public:
    KadasMapRect( double xMin = 0., double yMin = 0., double xMax = 0., double yMax = 0. ) : mXmin( xMin ), mYmin( yMin ), mXmax( xMax ), mYmax( yMax ) {}
    KadasMapRect( const KadasMapPos &p1, const KadasMapPos &p2 )
      : mXmin( std::min( p1.x(), p2.x() ) ), mYmin( std::min( p1.y(), p2.y() ) ),
        mXmax( std::max( p1.x(), p2.x() ) ), mYmax( std::max( p1.y(), p2.y() ) ) {}
    double xMinimum() const { return mXmin; }
    void setXMinimum( double xMin ) { mXmin = xMin; }
    double yMinimum() const { return mYmin; }
    void setYMinimum( double yMin ) { mYmin = yMin; }
    double xMaximum() const { return mXmax; }
    void setXMaximum( double xMax ) { mXmax = xMax; }
    double yMaximum() const { return mYmax; }
    void setYMaximum( double ymax ) { mYmax = ymax; }
    operator QgsRectangle() const { return QgsRectangle( mXmin, mYmin, mXmax, mYmax ); }
    KadasMapPos center() const { return KadasMapPos( 0.5 * ( mXmin + mXmax ), 0.5 * ( mYmin + mYmax ) ); }
  private:
    double mXmin = 0.;
    double mYmin = 0.;
    double mXmax = 0.;
    double mYmax = 0.;
};

class KADAS_GUI_EXPORT KadasItemPos
{
  public:
    static KadasItemPos fromPoint( const QgsPointXY &pos ) { return KadasItemPos( pos.x(), pos.y() ); }

    KadasItemPos( double x = 0., double y = 0. ) : mX( x ), mY( y ) {}
    double x() const { return mX; }
    void setX( double x ) { mX = x; }
    double y() const { return mY; }
    void setY( double y ) { mY = y; }
    operator QgsPointXY() const { return QgsPointXY( mX, mY ); }
    double sqrDist( const KadasItemPos &p ) const { return ( mX - p.mX ) * ( mX - p.mX ) + ( mY - p.mY ) * ( mY - p.mY ); }
  private:
    double mX = 0.;
    double mY = 0.;
};

class KADAS_GUI_EXPORT KadasItemRect
{
  public:
    KadasItemRect( double xMin = 0., double yMin = 0., double xMax = 0., double yMax = 0. ) : mXmin( xMin ), mYmin( yMin ), mXmax( xMax ), mYmax( yMax ) {}
    KadasItemRect( const KadasItemPos &p1, const KadasItemPos &p2 )
      : mXmin( std::min( p1.x(), p2.x() ) ), mYmin( std::min( p1.y(), p2.y() ) ),
        mXmax( std::max( p1.x(), p2.x() ) ), mYmax( std::max( p1.y(), p2.y() ) ) {}
    double xMinimum() const { return mXmin; }
    void setXMinimum( double xMin ) { mXmin = xMin; }
    double yMinimum() const { return mYmin; }
    void setYMinimum( double yMin ) { mYmin = yMin; }
    double xMaximum() const { return mXmax; }
    void setXMaximum( double xMax ) { mXmax = xMax; }
    double yMaximum() const { return mYmax; }
    void setYMaximum( double ymax ) { mYmax = ymax; }
    operator QgsRectangle() const { return QgsRectangle( mXmin, mYmin, mXmax, mYmax ); }
    KadasItemPos center() const { return KadasItemPos( 0.5 * ( mXmin + mXmax ), 0.5 * ( mYmin + mYmax ) ); }
  private:
    double mXmin = 0.;
    double mYmin = 0.;
    double mXmax = 0.;
    double mYmax = 0.;
};

class KADAS_GUI_EXPORT KadasMapItem : public QObject SIP_ABSTRACT
{
    Q_OBJECT
  public:
    KadasMapItem( const QgsCoordinateReferenceSystem &crs, QObject *parent );
    ~KadasMapItem();
    KadasMapItem *clone() const;

    virtual QString itemName() const = 0;

    /* The item crs */
    const QgsCoordinateReferenceSystem &crs() const { return mCrs; }

    /* Bounding box in geographic coordinates */
    virtual KadasItemRect boundingBox() const = 0;

    /* Margin in screen units */
    struct Margin
    {
      int left = 0;
      int top = 0;
      int right = 0;
      int bottom = 0;
    };
    virtual Margin margin() const { return Margin(); }

    /* Nodes for editing */
    struct Node
    {
      KadasMapPos pos;
#ifndef SIP_RUN
      typedef void ( *node_renderer_t )( QPainter *, const QPointF &, int );
      node_renderer_t render = defaultNodeRenderer;
#else
      // TODO
#endif
    };

    virtual QList<KadasMapItem::Node> nodes( const QgsMapSettings &settings ) const = 0;

    /* Hit test, rect in item crs */
    virtual bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings ) const = 0;

    /* Render the item */
    virtual void render( QgsRenderContext &context ) const = 0;

#ifndef SIP_RUN
    /* Create KML representation */
    virtual QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const = 0;
#endif

    /* Associate to layer */
    void associateToLayer( QgsMapLayer *layer );
    QgsMapLayer *associatedLayer() const { return mAssociatedLayer; }

    /* Selected state */
    void setSelected( bool selected );
    bool selected() const { return mSelected; }

    /* z-index */
    void setZIndex( int zIndex );
    int zIndex() const { return mZIndex; }

    /* Trigger a redraw */
    void update();

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

    // Draw interface (coordinates in map crs, attribute distances in map units)
    virtual void clear();
    virtual bool startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings ) = 0;
    virtual bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;
    virtual void setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings ) = 0;
    virtual void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;
    virtual bool continuePart( const QgsMapSettings &mapSettings ) = 0;
    virtual void endPart() = 0;

    virtual AttribDefs drawAttribs() const = 0;
    virtual AttribValues drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const = 0;
    virtual KadasMapPos positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const = 0;

    // Edit interface (coordinates in map crs, attribute distances in map units)
    struct EditContext
    {
      EditContext( const QgsVertexId &_vidx = QgsVertexId(), const KadasMapPos &_pos = KadasMapPos(), const AttribDefs &_attributes = KadasMapItem::AttribDefs(), Qt::CursorShape _cursor = Qt::CrossCursor )
        : vidx( _vidx )
        , pos( _pos )
        , attributes( _attributes )
        , cursor( _cursor )
      {
      }
      QgsVertexId vidx;
      KadasMapPos pos;
      AttribDefs attributes;
      Qt::CursorShape cursor;
      bool isValid() const { return vidx.isValid(); }
      bool operator== ( const EditContext &other ) const { return vidx == other.vidx; }
      bool operator!= ( const EditContext &other ) const { return vidx != other.vidx; }
    };
    virtual EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const = 0;
    virtual void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) = 0;
    virtual void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;
    enum ContextMenuActions
    {
      EditNoAction,
      EditSwitchToDrawingTool
    };
    virtual void populateContextMenu( QMenu *menu, const EditContext &context ) {}

    virtual AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const = 0;
    virtual KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const = 0;

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

    // Position interface
    virtual KadasItemPos position() const = 0;
    virtual void setPosition( const KadasItemPos &pos ) = 0;

  signals:
    void aboutToBeDestroyed();
    void changed();

  protected:
    State *mState = nullptr;
    QgsCoordinateReferenceSystem mCrs;
    bool mSelected = false;
    int mZIndex = 0;
    QgsMapLayer *mAssociatedLayer = nullptr;

    virtual KadasMapItem::State *createEmptyState() const = 0 SIP_FACTORY;

    static void defaultNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );
    static void anchorNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );

    KadasMapPos toMapPos( const KadasItemPos &itemPos, const QgsMapSettings &settings ) const;
    KadasItemPos toItemPos( const KadasMapPos &mapPos, const QgsMapSettings &settings ) const;
    KadasMapRect toMapRect( const KadasItemRect &itemRect, const QgsMapSettings &settings ) const;
    KadasItemRect toItemRect( const KadasMapRect &itemRect, const QgsMapSettings &settings ) const;
    double pickTol( const QgsMapSettings &settings ) const;


  private:
    EditorFactory mEditorFactory = nullptr;

    virtual KadasMapItem *_clone() const = 0 SIP_FACTORY;
};

#endif // KADASMAPITEM_H
