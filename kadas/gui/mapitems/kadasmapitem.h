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
#include <qgis/qgsvertexid.h>

#include <kadas/core/kadasstatehistory.h>
#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/mapitemeditors/kadasmapitemeditor.h>

class QMenu;
class QuaZip;
class QgsRenderContext;
class KadasItemLayer;
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
    KadasMapRect( const KadasMapPos &center, double span ) : mXmin( center.x() - span ), mYmin( center.y() - span ), mXmax( center.x() + span ), mYmax( center.y() + span ) {}
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


#ifdef SIP_RUN
//
// adapted from PyQt5 QPair<_TYPE1_, _TYPE2_> and QPair<float, float>
//
% MappedType QPair<KadasMapPos, double> / TypeHint = "Tuple[KadasMapPos, float]" /
{
  % TypeHeaderCode
#include <QPair>
  % End

  % ConvertFromTypeCode
  KadasMapPos *first = new KadasMapPos( sipCpp->first );
  PyObject *t = sipBuildResult( NULL, "(Nf)", first, sipType_KadasMapPos,
                                sipTransferObj, sipCpp->second );

  if ( !t )
  {
    delete first;

    return 0;
  }

  return t;
  % End

  % ConvertToTypeCode
  if ( !sipIsErr )
    return ( PySequence_Check( sipPy ) && !PyUnicode_Check( sipPy ) );

  Py_ssize_t len = PySequence_Size( sipPy );

  if ( len != 2 )
  {
    // A negative length should only be an internal error so let the
    // original exception stand.
    if ( len >= 0 )
      PyErr_Format( PyExc_TypeError,
                    "sequence has %zd elements but 2 elements are expected",
                    len );

    *sipIsErr = 1;

    return 0;
  }

  PyObject *firstobj = PySequence_GetItem( sipPy, 0 );

  if ( !firstobj )
  {
    *sipIsErr = 1;

    return 0;
  }

  int firststate;
  KadasMapPos *first = reinterpret_cast<KadasMapPos *>(
    sipForceConvertToType( firstobj, sipType_KadasMapPos, sipTransferObj,
                           SIP_NOT_NONE, &firststate, sipIsErr ) );

  if ( *sipIsErr )
  {
    PyErr_Format( PyExc_TypeError,
                  "the first element has type '%s' but 'KadasMapPos' is expected",
                  sipPyTypeName( Py_TYPE( firstobj ) ) );

    return 0;
  }

  PyObject *secondobj = PySequence_GetItem( sipPy, 1 );

  if ( !secondobj )
  {
    sipReleaseType( first, sipType_KadasMapPos, firststate );
    Py_DECREF( firstobj );
    *sipIsErr = 1;

    return 0;
  }

  double second = PyFloat_AsDouble( secondobj );

  if ( PyErr_Occurred() )
  {
    if ( PyErr_ExceptionMatches( PyExc_TypeError ) )
      PyErr_Format( PyExc_TypeError,
                    "the second element has type '%s' but 'float' is expected",
                    sipPyTypeName( Py_TYPE( secondobj ) ) );

    Py_DECREF( secondobj );
    sipReleaseType( first, sipType_KadasMapPos, firststate );
    Py_DECREF( firstobj );
    *sipIsErr = 1;

    return 0;
  }

  *sipCppPtr = new QPair<KadasMapPos, double>( *first, second );

  Py_DECREF( secondobj );
  sipReleaseType( first, sipType_KadasMapPos, firststate );
  Py_DECREF( firstobj );

  return sipGetState( sipTransferObj );
  % End
};
#endif


class KADAS_GUI_EXPORT KadasMapItem : public QObject SIP_ABSTRACT
{
    Q_OBJECT
    Q_PROPERTY( int zIndex READ zIndex WRITE setZIndex )
    Q_PROPERTY( double symbolScale READ symbolScale WRITE setSymbolScale )
    Q_PROPERTY( QString editor READ editor WRITE setEditor )
    Q_PROPERTY( QString authId READ authId WRITE setAuthId )
    Q_PROPERTY( QString tooltip READ tooltip WRITE setTooltip )

  public:
    KadasMapItem( const QgsCoordinateReferenceSystem &crs );
    ~KadasMapItem();
    KadasMapItem *clone() const;
    QJsonObject serialize() const;
    bool deserialize( const QJsonObject &json );

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
    virtual bool intersects( const KadasMapRect &rect, const QgsMapSettings &settings, bool contains = false ) const = 0;
    virtual bool hitTest( const KadasMapPos &pos, const QgsMapSettings &settings ) const;

    /* Return the item point to the specified one */
    virtual QPair<KadasMapPos, double> closestPoint( const KadasMapPos &pos, const QgsMapSettings &settings ) const;

    /* Render the item */
    virtual void render( QgsRenderContext &context ) const = 0;

#ifndef SIP_RUN
    /* Create KML representation */
    virtual QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const = 0;
#endif

    /* Associate to layer */
    void associateToLayer( QgsMapLayer *layer );
    QgsMapLayer *associatedLayer() const { return mAssociatedLayer; }

    /* Owner layer */
    void setOwnerLayer( KadasItemLayer *layer );
    KadasItemLayer *ownerLayer() const { return mOwnerLayer; }

    /* Selected state */
    void setSelected( bool selected );
    bool selected() const { return mSelected; }

    /* z-index */
    void setZIndex( int zIndex );
    int zIndex() const { return mZIndex; }

    /* symbol scale */
    void setSymbolScale( double scale );
    double symbolScale() const { return mSymbolScale; }

    /* authid */
    void setAuthId( const QString &authId );
    QString authId() const { return mCrs.authid(); }

    /* tooltip */
    void setTooltip( const QString &tooltip );
    const QString &tooltip() const { return mTooltip; }

    bool isPointSymbol() const { return mIsPointSymbol; }
    virtual QImage symbolImage() const { return QImage(); }
    virtual QPointF symbolAnchor() const { return QPointF( 0.5, 0.5 ); }

    /* Trigger a redraw */
    void update();

    // State interface
    struct State : KadasStateHistory::State
    {
      enum DrawStatus { Empty, Drawing, Finished };
      DrawStatus drawStatus = Empty;
      virtual void assign( const State *other ) = 0;
      virtual State *clone() const = 0 SIP_FACTORY;
      virtual QJsonObject serialize() const = 0;
      virtual bool deserialize( const QJsonObject &json ) = 0;
    };
    const State *constState() const { return mState; }
    virtual void setState( const State *state );

    struct KADAS_GUI_EXPORT NumericAttribute
    {
      QString name;
      enum Type {TypeCoordinate, TypeDistance, TypeAngle, TypeOther};
      Type type = TypeCoordinate;
      double min = std::numeric_limits<double>::lowest();
      double max = std::numeric_limits<double>::max();
      int decimals = -1;
      int precision( const QgsMapSettings &mapSettings ) const;
      QString suffix( const QgsMapSettings &mapSettings ) const;
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
      EditContext() : mValid( false ) {}
      EditContext( const QgsVertexId &_vidx, const KadasMapPos &_pos = KadasMapPos(), const AttribDefs &_attributes = KadasMapItem::AttribDefs(), Qt::CursorShape _cursor = Qt::CrossCursor )
        : mValid( true )
        , vidx( _vidx )
        , pos( _pos )
        , attributes( _attributes )
        , cursor( _cursor )
      {
      }
      bool mValid = false;
      QgsVertexId vidx;
      KadasMapPos pos;
      AttribDefs attributes;
      Qt::CursorShape cursor;
      bool isValid() const { return mValid; }
      bool operator== ( const EditContext &other ) const { return vidx == other.vidx && mValid == other.mValid; }
      bool operator!= ( const EditContext &other ) const { return vidx != other.vidx || mValid != other.mValid; }
    };
    virtual EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const = 0;
    virtual void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) = 0;
    virtual void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) = 0;
    enum ContextMenuActions
    {
      EditNoAction,
      EditSwitchToDrawingTool
    };
    virtual void populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings ) {}
    virtual void onDoubleClick( const QgsMapSettings &mapSettings ) {}

    virtual AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const = 0;
    virtual KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const = 0;

    // Editor
    void setEditor( const QString &editor ) { mEditor = editor; }
    const QString &editor() const { return mEditor; }

    // Position interface
    virtual KadasItemPos position() const = 0;
    virtual void setPosition( const KadasItemPos &pos ) = 0;

    // TODO: SIP
#ifndef SIP_RUN
    typedef std::function<KadasMapItem*( const QgsCoordinateReferenceSystem & ) > RegistryItemFactory;
    typedef QMap<QString, RegistryItemFactory> Registry;
    static Registry *registry();
#endif

    QDomElement writeXml( QDomDocument &document ) const;
    static KadasMapItem *fromXml( const QDomElement &element );

  signals:
    void aboutToBeDestroyed();
    void changed();

  protected:
    State *mState = nullptr;
    QgsCoordinateReferenceSystem mCrs;
    bool mSelected = false;
    int mZIndex = 0;
    QString mTooltip;
    double mSymbolScale = 1.0;
    QgsMapLayer *mAssociatedLayer = nullptr;
    KadasItemLayer *mOwnerLayer = nullptr;
    bool mIsPointSymbol = false;

    virtual KadasMapItem::State *createEmptyState() const = 0 SIP_FACTORY;

    static void defaultNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );
    static void anchorNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize );
    static double outputDpiScale( const QgsRenderContext &context );

    KadasMapPos toMapPos( const KadasItemPos &itemPos, const QgsMapSettings &settings ) const;
    KadasItemPos toItemPos( const KadasMapPos &mapPos, const QgsMapSettings &settings ) const;
    KadasMapRect toMapRect( const KadasItemRect &itemRect, const QgsMapSettings &settings ) const;
    KadasItemRect toItemRect( const KadasMapRect &itemRect, const QgsMapSettings &settings ) const;
    double pickTolSqr( const QgsMapSettings &settings ) const;
    double pickTol( const QgsMapSettings &settings ) const;

  private:
    QString mEditor;

    virtual KadasMapItem *_clone() const = 0 SIP_FACTORY;
};

#ifndef SIP_RUN
#define KADAS_REGISTER_MAP_ITEM(classname, factory) \
  static int register##classname(){ KadasMapItem::registry()->insert(#classname, factory); return 0; } \
  static int __reg##classname = register##classname();
#endif

#endif // KADASMAPITEM_H
