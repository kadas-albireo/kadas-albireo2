/***************************************************************************
    kadasmaptooldrawshape.cpp
    -------------------------
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

#include <QDoubleValidator>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <qmath.h>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>
#include <GeographicLib/Constants.hpp>

#include <qgis/qgscircularstring.h>
#include <qgis/qgscompoundcurve.h>
#include <qgis/qgscoordinatetransform.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapmouseevent.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmultilinestring.h>
#include <qgis/qgsmultipolygon.h>
#include <qgis/qgsmultipoint.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsrubberband.h>
#include <qgis/qgssnappingutils.h>

#include <kadas/gui/kadasfloatinginputwidget.h>
#include <kadas/gui/maptools/kadasmaptooldrawshape.h>

KadasMapToolDrawShape::KadasMapToolDrawShape( QgsMapCanvas *canvas, bool isArea, State *initialState )
    : QgsMapTool( canvas )
    , mIsArea( isArea )
    , mMultipart( false )
    , mInputWidget( 0 )
    , mStateStack( initialState )
    , mSnapPoints( false )
    , mShowInput( false )
    , mResetOnDeactivate( true )
    , mIgnoreNextMoveEvent( false )
    , mEditContext( 0 )
    , mParentTool( 0 )
{
  setCursor( Qt::CrossCursor );

  QSettings settings;
  int red = settings.value( "/Qgis/default_measure_color_red", 255 ).toInt();
  int green = settings.value( "/Qgis/default_measure_color_green", 0 ).toInt();
  int blue = settings.value( "/Qgis/default_measure_color_blue", 0 ).toInt();

  mRubberBand = new KadasGeometryRubberBand( canvas, isArea ? QgsWkbTypes::PolygonGeometry : QgsWkbTypes::LineGeometry );
  mRubberBand->setFillColor( QColor( red, green, blue, 63 ) );
  mRubberBand->setOutlineColor( QColor( red, green, blue, 255 ) );
  mRubberBand->setOutlineWidth( 3 );
  mRubberBand->setIconSize( 10 );
  mRubberBand->setIconOutlineWidth( 2 );
  mRubberBand->setIconOutlineColor( mRubberBand->outlineColor() );
  mRubberBand->setIconFillColor( Qt::white );
  mRubberBand->setIconType( KadasGeometryRubberBand::ICON_NONE );

  // Shapes typically won't survive projection changes without distortion (circle, square, etc)
  connect( canvas, &QgsMapCanvas::destinationCrsChanged, this, &KadasMapToolDrawShape::reset );

  connect( &mStateStack, &KadasStateStack::canUndoChanged, this, &KadasMapToolDrawShape::canUndo );
  connect( &mStateStack, &KadasStateStack::canRedoChanged, this, &KadasMapToolDrawShape::canRedo );
  connect( &mStateStack, &KadasStateStack::stateChanged, this, &KadasMapToolDrawShape::update );
}

KadasMapToolDrawShape::~KadasMapToolDrawShape()
{
  delete mRubberBand.data();
  delete mInputWidget;
  mInputWidget = 0;
}

void KadasMapToolDrawShape::activate()
{
  QgsMapTool::activate();
  if ( mShowInput )
  {
    mInputWidget = new KadasFloatingInputWidget( canvas() );
    initInputWidget();
    if ( state()->status != StatusEditingReady )
    {
      mInputWidget->show();
    }
  }
}

void KadasMapToolDrawShape::deactivate()
{
  if ( state()->status == StatusEditingReady || state()->status == StatusEditingMoving )
  {
    emit finished();
  }
  QgsMapTool::deactivate();
  if ( mResetOnDeactivate )
    reset();
  delete mInputWidget;
  mInputWidget = 0;
}

void KadasMapToolDrawShape::editGeometry( const QgsAbstractGeometry *geometry, const QgsCoordinateReferenceSystem &sourceCrs )
{
  reset();
  addGeometry( geometry, sourceCrs );
  State* newState = cloneState();
  newState->status = StatusEditingReady;
  mStateStack.clear( newState );
}

void KadasMapToolDrawShape::setMeasurementMode(KadasGeometryRubberBand::MeasurementMode measurementMode, QgsUnitTypes::DistanceUnit distanceUnit, QgsUnitTypes::AreaUnit areaUnit, QgsUnitTypes::AngleUnit angleUnit, KadasGeometryRubberBand::AzimuthNorth azimuthNorth)
{
  mRubberBand->setMeasurementMode( measurementMode, distanceUnit, areaUnit, angleUnit, azimuthNorth );
  emit geometryChanged();
}

void KadasMapToolDrawShape::update()
{
  QList<QgsVertexId> hiddenNodes;
  QgsAbstractGeometry* geom = createGeometry( mCanvas->mapSettings().destinationCrs(), &hiddenNodes );
  mRubberBand->setGeometry( geom, hiddenNodes );
  emit geometryChanged();
}

void KadasMapToolDrawShape::updateStyle( int outlineWidth, const QColor &outlineColor, const QColor &fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle )
{
  mRubberBand->setOutlineWidth( outlineWidth );
  mRubberBand->setOutlineColor( outlineColor );
  mRubberBand->setFillColor( fillColor );
  mRubberBand->setLineStyle( lineStyle );
  mRubberBand->setBrushStyle( brushStyle );
  mRubberBand->update();
}

void KadasMapToolDrawShape::reset()
{
  mRubberBand->setGeometry( 0 );
  mStateStack.clear( emptyState() );
  emit geometryChanged();
  emit cleared();
}

void KadasMapToolDrawShape::canvasPressEvent( QgsMapMouseEvent* e )
{
  if ( state()->status == StatusEditingReady )
  {
    if ( mEditContext )
    {
      if ( e->button() == Qt::RightButton )
      {
        QMenu menu;
        addContextMenuActions( mEditContext, menu );
        if ( !menu.isEmpty() )
        {
          menu.addSeparator();
        }
        QAction* deleteAction = menu.addAction( QIcon( ":/kadas/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ) );
        QAction* clickedAction = menu.exec( canvas()->mapToGlobal( e->pos() ) );
        if ( !clickedAction )
        {
          // pass
        }
        else if ( clickedAction == deleteAction )
        {
          deleteShape();
        }
        else
        {
          executeContextMenuAction( mEditContext, clickedAction->data().toInt(), transformPoint( e->pos() ) );
        }
      }
      else
      {
        mStateStack.updateState( cloneState() );
        mutableState()->status = StatusEditingMoving;
        mLastEditPos = transformPoint( e->pos() );
      }
    }
    return;
  }
  if ( state()->status == StatusFinished )
  {
    reset();
  }
  if ( state()->status == StatusReady && e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
    return;
  }
  buttonEvent( transformPoint( e->pos() ), true, e->button() );

  if ( mShowInput )
  {
    updateInputWidget( transformPoint( e->pos() ) );
  }
  if ( state()->status == StatusFinished )
  {
    emit finished();
  }
}

void KadasMapToolDrawShape::canvasMoveEvent( QgsMapMouseEvent* e )
{
  if ( mIgnoreNextMoveEvent )
  {
    mIgnoreNextMoveEvent = false;
    return;
  }
  if ( state()->status == StatusEditingMoving && mEditContext )
  {
    QgsPointXY newPos = transformPoint( e->pos() );
    edit( mEditContext, newPos, newPos - mLastEditPos );
    mLastEditPos = newPos;
  }
  else if ( state()->status == StatusEditingReady )
  {
    delete mEditContext;
    mEditContext = getEditContext( transformPoint( e->pos() ) );
    if ( mShowInput )
    {
      if ( mEditContext && !mEditContext->move )
      {
        mInputWidget->show();
        updateInputWidget( mEditContext );
        mInputWidget->move( e->x(), e->y() + 20 );
      }
      else
      {
        mInputWidget->hide();
      }
    }
    return;
  }
  else if ( state()->status == StatusDrawing )
  {
    moveEvent( transformPoint( e->pos() ) );
  }
  if ( mShowInput )
  {
    if ( mEditContext )
    {
      updateInputWidget( mEditContext );
    }
    else
    {
      updateInputWidget( transformPoint( e->pos() ) );
    }
    mInputWidget->move( e->x(), e->y() + 20 );
  }
}

void KadasMapToolDrawShape::canvasReleaseEvent( QgsMapMouseEvent* e )
{
  if ( state()->status == StatusEditingMoving )
  {
    mutableState()->status = StatusEditingReady;
  }
  else if ( state()->status != StatusEditingReady && state()->status != StatusFinished )
  {
    buttonEvent( transformPoint( e->pos() ), false, e->button() );

    if ( mShowInput )
    {
      updateInputWidget( transformPoint( e->pos() ) );
    }
    if ( state()->status == StatusFinished )
    {
      emit finished();
    }
  }
  else if ( state()->status == StatusEditingReady )
  {
    canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
  }
}

void KadasMapToolDrawShape::keyPressEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    if ( state()->status == StatusReady || state()->status == StatusEditingReady )
      canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
    else if ( state()->status != StatusEditingMoving )
      reset();
  }
  else if ( e->key() == Qt::Key_Delete && state()->status == StatusEditingReady )
  {
    State* state = emptyState();
    state->status = StatusEditingReady;
    mStateStack.updateState( state );
    canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
  }
  else if ( e->key() == Qt::Key_Z && e->modifiers() == Qt::CTRL )
  {
    undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::CTRL )
  {
    redo();
  }
}

void KadasMapToolDrawShape::deleteShape()
{
  State* state = emptyState();
  state->status = StatusEditingReady;
  mStateStack.updateState( state );
  canvas()->unsetMapTool( mParentTool ? mParentTool : this ); // unset
}

void KadasMapToolDrawShape::acceptInput()
{
  if ( state()->status == StatusFinished )
  {
    reset();
  }
  inputAccepted();
  if ( state()->status == StatusFinished )
  {
    emit finished();
  }
}

QgsPointXY KadasMapToolDrawShape::transformPoint( const QPoint& p )
{
  if ( !mSnapPoints )
  {
    return toMapCoordinates( p );
  }
  QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToMap( p );
  return m.isValid() ? m.point() : toMapCoordinates( p );
}

void KadasMapToolDrawShape::addGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateReferenceSystem& sourceCrs )
{
  if ( !mMultipart )
  {
    reset();
  }
  doAddGeometry( geometry, QgsCoordinateTransform( sourceCrs, canvas()->mapSettings().destinationCrs(), QgsProject::instance() ) );
}

void KadasMapToolDrawShape::moveMouseToPos(const QgsPointXY &geoPos )
{
  // Ignore the move event emitted by re-positioning the mouse cursor:
  // The widget mouse coordinates (stored in a integer QPoint) loses precision,
  // and mapping it back to map coordinates in the mouseMove event handler
  // results in a position different from geoPos, and hence the user-input
  // may get altered
  mIgnoreNextMoveEvent = true;

  mInputWidget->adjustCursorAndExtent( geoPos );

  if ( state()->status == StatusDrawing )
  {
    moveEvent( geoPos );
    update();
  }
  updateInputWidget( geoPos );
}

bool KadasMapToolDrawShape::pointInPolygon( const QgsPointXY& p, const QList<QgsPointXY> &poly )
{
  bool contains = false;
  for ( int n = poly.size(), i = 0, j = n - 1; i < n; j = i++ )
  {
    const QgsPointXY& pi = poly[i];
    const QgsPointXY& pj = poly[j];
    if (
      (( pi.y() > p.y() ) != ( pj.y() > p.y() ) ) &&
      ( p.x() < ( pj.x() - pi.x() ) * ( p.y() - pi.y() ) / ( pj.y() - pi.y() ) + pi.x() )
    )
    {
      contains = !contains;
    }
  }
  return contains;
}

QgsPointXY KadasMapToolDrawShape::projPointOnSegment( const QgsPointXY &p, const QgsPointXY &s1, const QgsPointXY &s2 )
{
  double nx = s2.y() - s1.y();
  double ny = -( s2.x() - s1.x() );
  double t = ( p.x() * ny - p.y() * nx - s1.x() * ny + s1.y() * nx ) / (( s2.x() - s1.x() ) * ny - ( s2.y() - s1.y() ) * nx );
  return t < 0. ? s1 : t > 1. ? s2 : QgsPointXY( s1.x() + ( s2.x() - s1.x() ) * t, s1.y() + ( s2.y() - s1.y() ) * t );
}

///////////////////////////////////////////////////////////////////////////////

KadasMapToolDrawPoint::KadasMapToolDrawPoint( QgsMapCanvas *canvas )
    : KadasMapToolDrawShape( canvas, false, emptyState() )
{
  mRubberBand->setIconType( KadasGeometryRubberBand::ICON_CIRCLE );
}

KadasMapToolDrawShape::State* KadasMapToolDrawPoint::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  return newState;
}

void KadasMapToolDrawPoint::buttonEvent(const QgsPointXY &pos, bool press, Qt::MouseButton button )
{
  if ( !press && button == Qt::LeftButton )
  {
    State* newState = cloneState();
    newState->points.append( QList<QgsPointXY>() << pos );
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

QgsAbstractGeometry* KadasMapToolDrawPoint::createGeometry( const QgsCoordinateReferenceSystem &targetCrs, QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs, QgsProject::instance()->transformContext() );
  QgsGeometryCollection* multiGeom = new QgsMultiPoint();
  for ( const QList<QgsPointXY>& part : state()->points )
  {
    if ( part.size() > 0 )
    {
      QgsPointXY p = t.transform( part.front() );
      multiGeom->addGeometry( new QgsPoint( p.x(), p.y() ) );
    }
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometry* geom = multiGeom->isEmpty() ? new QgsPoint() : multiGeom->geometryN( 0 )->clone();
    delete multiGeom;
    return geom;
  }
}

void KadasMapToolDrawPoint::doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t )
{
  State* newState = cloneState();
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    if ( newState->points.isEmpty() || !newState->points.back().isEmpty() )
    {
      newState->points.append( QList<QgsPointXY>() );
    }
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      for ( int iVtx = 0, nVtx = geometry->vertexCount( iPart, iRing ); iVtx < nVtx; ++iVtx )
      {
        QgsPoint vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, iVtx ) );
        newState->points.back().append( QgsPoint( t.transform( vertex ) ) );
      }
    }
  }
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void KadasMapToolDrawPoint::initInputWidget()
{
  mXEdit = new KadasFloatingInputWidgetField();
  connect( mXEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawPoint::inputChanged );
  connect( mXEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawPoint::acceptInput );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new KadasFloatingInputWidgetField();
  connect( mYEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawPoint::inputChanged );
  connect( mYEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawPoint::acceptInput );
  mInputWidget->addInputField( "y:", mYEdit );
}

void KadasMapToolDrawPoint::updateInputWidget(const QgsPointXY &mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QgsUnitTypes::DistanceDegrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void KadasMapToolDrawPoint::updateInputWidget( const KadasMapToolDrawShape::EditContext* context )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  updateInputWidget( state()->points[ctx->index].front() );
}

void KadasMapToolDrawPoint::inputAccepted()
{
  if ( state()->status >= StatusEditingReady )
  {
    return;
  }
  State* newState = cloneState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  newState->points.append( QList<QgsPointXY>() << QgsPointXY( x, y ) );
  mInputWidget->setFocusedInputField( mXEdit );
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void KadasMapToolDrawPoint::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
  if ( currentEditContext() )
  {
    edit( currentEditContext(), QgsPoint( x, y ), QgsVector( 0, 0 ) );
  }
}

KadasMapToolDrawShape::EditContext* KadasMapToolDrawPoint::getEditContext(const QgsPointXY &pos ) const
{
  int closestIndex = -1;
  double closestDistance = std::numeric_limits<double>::max();
  for ( int i = 0, n = state()->points.size(); i < n; ++i )
  {
    double distance = state()->points[i].front().sqrDist( pos );
    if ( distance < closestDistance )
    {
      closestDistance = distance;
      closestIndex = i;
    }
  }
  if ( closestIndex == -1 )
  {
    return 0;
  }
  QPoint p1 = toCanvasCoordinates( pos );
  QPoint p2 = toCanvasCoordinates( state()->points[closestIndex].front() );
  if ( qAbs(( p2 - p1 ).manhattanLength() ) > 10 )
  {
    return 0;
  }
  EditContext* context = new EditContext;
  context->index = closestIndex;
  return context;
}

void KadasMapToolDrawPoint::edit(const KadasMapToolDrawShape::EditContext* context, const QgsPointXY &pos, const QgsVector &/*delta*/ )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  mutableState()->points[ctx->index].front() = pos;
  update();
}

void KadasMapToolDrawPoint::setPart(int part, const QgsPointXY &p )
{
  State* newState = cloneState();
  newState->points[part].front() = p;
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

KadasMapToolDrawPolyLine::KadasMapToolDrawPolyLine( QgsMapCanvas *canvas, bool closed, bool geodesic )
    : KadasMapToolDrawShape( canvas, closed, emptyState() ), mGeodesic( geodesic )
{
  if ( geodesic )
  {
    mDa.setEllipsoid( "WGS84" );
    mDa.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );
  }
}

KadasMapToolDrawShape::State* KadasMapToolDrawPolyLine::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  newState->points.append( QList<QgsPointXY>() );
  return newState;
}

void KadasMapToolDrawPolyLine::buttonEvent(const QgsPointXY &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton )
  {
    State* newState = cloneState();
    if ( newState->points.back().isEmpty() )
    {
      newState->points.back().append( pos );
    }
    newState->points.back().append( pos );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( !press && button == Qt::RightButton )
  {
    if (( mIsArea && state()->points.back().size() >= 3 ) || ( !mIsArea && state()->points.back().size() >= 2 ) )
    {
      State* newState = cloneState();
      // If last two points are very close, discard last point
      // (To avoid confusion if user left-clicks to set point and right-clicks to terminate)
      int nPoints = newState->points.back().size();
      QPoint p1 = toCanvasCoordinates( newState->points.back()[nPoints - 2] );
      QPoint p2 = toCanvasCoordinates( newState->points.back()[nPoints - 1] );
      if ( qAbs(( p2  - p1 ).manhattanLength() ) < 10 )
      {
        newState->points.back().pop_back();
      }
      if (( mIsArea && newState->points.back().size() < 3 ) || ( !mIsArea && newState->points.back().size() < 2 ) )
      {
        // Don't terminate geometry if insufficient points remain
        return;
      }
      if ( mMultipart )
      {
        newState->points.append( QList<QgsPointXY>() );
        newState->status = StatusReady;
      }
      else
      {
        newState->status = StatusFinished;
      }
      mStateStack.updateState( newState );
    }
  }
}

void KadasMapToolDrawPolyLine::moveEvent(const QgsPointXY &pos )
{
  mutableState()->points.back().back() = pos;
  update();
}

QgsAbstractGeometry* KadasMapToolDrawPolyLine::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> *hiddenNodes ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs, QgsProject::instance() );
  QgsGeometryCollection* multiGeom = mIsArea ? static_cast<QgsGeometryCollection*>( new QgsMultiPolygon() ) : static_cast<QgsGeometryCollection*>( new QgsMultiLineString() );
  for ( int iPart = 0, nParts = state()->points.size(); iPart < nParts; ++iPart )
  {
    const QList<QgsPointXY>& part = state()->points[iPart];
    QgsLineString* ring = new QgsLineString();
    if ( mGeodesic )
    {
      int nPoints = part.size();
      if ( nPoints >= 2 )
      {
        QgsCoordinateTransform t1( canvas()->mapSettings().destinationCrs(), QgsCoordinateReferenceSystem( "EPSG:4326"), QgsProject::instance() );
        QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326"), targetCrs, QgsProject::instance() );
        QList<QgsPointXY> wgsPoints;

        GeographicLib::Geodesic geod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() );

        for ( const QgsPointXY& point : part )
        {
          wgsPoints.append( t1.transform( point ) );
        }

        double sdist = 500000; // 500km segments
        for ( int i = 0; i < nPoints - 1; ++i )
        {
          int ringSize = ring->vertexCount();
          GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[i].y(), wgsPoints[i].x(), wgsPoints[i+1].y(), wgsPoints[i+1].x() );
          double dist = line.Distance();
          int nIntervals = qMax( 1, int( std::ceil( dist / sdist ) ) );
          for ( int j = 0; j < nIntervals; ++j )
          {
            double lat, lon;
            line.Position( j * sdist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
            if ( hiddenNodes && j != 0 )
            {
              hiddenNodes->append( QgsVertexId( iPart, 0, ringSize + j ) );
            }
          }
          if ( !mIsArea && i == nPoints - 2 )
          {
            double lat, lon;
            line.Position( dist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
          }
        }
        if ( mIsArea && !part.isEmpty() )
        {
          GeographicLib::GeodesicLine line = geod.InverseLine( wgsPoints[nPoints -1].y(), wgsPoints[nPoints -1].x(), wgsPoints[0].y(), wgsPoints[0].x() );
          double dist = line.Distance();
          int nIntervals = qMax( 1, int( std::ceil( dist / sdist ) ) );
          int ringSize = ring->vertexCount();
          for ( int j = 0; j < nIntervals; ++j )
          {
            double lat, lon;
            line.Position( j * sdist, lat, lon );
            ring->addVertex( QgsPoint( t2.transform( QgsPointXY( lon, lat ) ) ) );
            if ( hiddenNodes && j != 0 )
            {
              hiddenNodes->append( QgsVertexId( iPart, 0, ringSize + j ) );
            }
          }
          ring->addVertex( ring->vertexAt( QgsVertexId( 0, 0, 0 ) ) );
        }
      }
    }
    else
    {
      for ( const QgsPointXY& point : part )
      {
        ring->addVertex( QgsPoint( t.transform( point ) ) );
      }
      if ( mIsArea && !part.isEmpty() )
      {
        ring->addVertex( QgsPoint( t.transform( part.front() ) ) );
      }
    }

    if ( mIsArea )
    {
      QgsPolygon* poly = new QgsPolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
    else
    {
      multiGeom->addGeometry( ring );
    }
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometry* geom = multiGeom->isEmpty() ? (mIsArea ? static_cast<QgsAbstractGeometry*>(new QgsPolygon()) : new QgsLineString()) : multiGeom->geometryN( 0 )->clone();
    delete multiGeom;
    return geom;
  }
}

void KadasMapToolDrawPolyLine::doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t )
{
  State* newState = cloneState();
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    if ( !newState->points.back().isEmpty() )
    {
      newState->points.append( QList<QgsPointXY>() );
    }
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      for ( int iVtx = 0, nVtx = geometry->vertexCount( iPart, iRing ); iVtx < nVtx; ++iVtx )
      {
        QgsPoint vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, iVtx ) );
        newState->points.back().append( t.transform( vertex ) );
      }
      // Don't add closing vertex
      if ( mIsArea && !newState->points.back().isEmpty() )
      {
        newState->points.back().removeLast();
      }
    }
  }
  if ( mMultipart )
  {
    newState->points.append( QList<QgsPointXY>() );
    newState->status = StatusReady;
  }
  else
  {
    newState->status = StatusFinished;
  }
  mStateStack.updateState( newState );
}

void KadasMapToolDrawPolyLine::initInputWidget()
{
  mXEdit = new KadasFloatingInputWidgetField();
  connect( mXEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawPolyLine::inputChanged );
  connect( mXEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawPolyLine::acceptInput );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new KadasFloatingInputWidgetField();
  connect( mYEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawPolyLine::inputChanged );
  connect( mYEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawPolyLine::acceptInput );
  mInputWidget->addInputField( "y:", mYEdit );
}

void KadasMapToolDrawPolyLine::updateInputWidget(const QgsPointXY &mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QgsUnitTypes::DistanceDegrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void KadasMapToolDrawPolyLine::updateInputWidget( const KadasMapToolDrawShape::EditContext* context )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  updateInputWidget( state()->points[ctx->part][ctx->node] );
}

void KadasMapToolDrawPolyLine::inputAccepted()
{
  if ( state()->status >= StatusEditingReady )
  {
    return;
  }
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->points.back().append( QgsPoint( x, y ) );
    newState->points.back().append( QgsPoint( x, y ) );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    // At least two points and enter pressed twice -> finish
    int n = newState->points.back().size();
    if ( n > 1 && newState->points.back()[n - 2] == newState->points.back()[n - 1] )
    {
      mInputWidget->setFocusedInputField( mXEdit );
      newState->points.back().removeLast();
      newState->points.append( QList<QgsPointXY>() );
      newState->status = mMultipart ? StatusReady : StatusFinished;
      mStateStack.updateState( newState, true );
      return;
    }
    newState->points.back().back() = QgsPoint( x, y );
    newState->points.back().append( QgsPoint( x, y ) );
    mStateStack.updateState( newState );
  }
}

void KadasMapToolDrawPolyLine::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
  if ( currentEditContext() )
  {
    edit( currentEditContext(), QgsPoint( x, y ), QgsVector( 0, 0 ) );
  }
}

KadasMapToolDrawShape::EditContext* KadasMapToolDrawPolyLine::getEditContext(const QgsPointXY &pos ) const
{
  int closestPart = -1;
  int closestNode = -1;
  double closestDistance = std::numeric_limits<double>::max();
  for ( int i = 0, n = state()->points.size(); i < n; ++i )
  {
    for ( int j = 0, m = state()->points[i].size(); j < m; ++j )
    {
      double distance = state()->points[i][j].sqrDist( pos );
      if ( distance < closestDistance )
      {
        closestDistance = distance;
        closestPart = i;
        closestNode = j;
      }
    }
  }
  // Check if a node was picked
  if ( closestNode != -1 )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( state()->points[closestPart][closestNode] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->node = closestNode;
      return context;
    }
  }
  // Check whether entire geometry was picked
  if ( mIsArea )
  {
    for ( int i = 0, n = state()->points.size(); i < n; ++i )
    {
      if ( pointInPolygon( pos, state()->points[i] ) )
      {
        EditContext* context = new EditContext;
        context->part = closestPart;
        context->node = -1;
        context->move = true;
        return context;
      }
    }
  }
  else
  {
    QPoint p1 = toCanvasCoordinates( pos );
    for ( int i = 0, n = state()->points.size(); i < n; ++i )
    {
      const QList<QgsPointXY>& poly = state()->points[i];
      for ( int m = poly.size(), j = m - 1, k = 0; k < m - 1; j = k++ )
      {
        QgsPointXY pProj = projPointOnSegment( pos, poly[j], poly[k] );
        QPoint p2 = toCanvasCoordinates( pProj );
        if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
        {
          EditContext* context = new EditContext;
          context->part = closestPart;
          context->node = -1;
          context->move = true;
          return context;
        }
      }
    }
  }
  return 0;
}

void KadasMapToolDrawPolyLine::edit(const KadasMapToolDrawShape::EditContext* context, const QgsPointXY &pos, const QgsVector& delta )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( ctx->node != -1 )
  {
    mutableState()->points[ctx->part][ctx->node] = pos;
  }
  else
  {
    QList<QgsPointXY>& poly = mutableState()->points[ctx->part];
    for ( int i = 0, n = poly.size(); i < n; ++i )
    {
      poly[i] += delta;
    }
  }
  update();
}

void KadasMapToolDrawPolyLine::addContextMenuActions( const KadasMapToolDrawShape::EditContext* context, QMenu& menu ) const
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( ctx->node != -1 )
  {
    QAction* deleteNodeAction = menu.addAction( QIcon( ":/kadas/themes/default/mActionDeleteVertex.png" ), tr( "Delete node" ) );
    deleteNodeAction->setData( DeleteNode );
    deleteNodeAction->setEnabled( state()->points[ctx->part].length() >= 3 + mIsArea );
    if ( mIsArea || ( ctx->node == 0 || ctx->node == state()->points[ctx->part].length() - 1 ) )
    {
      QAction* continueAction = menu.addAction( QIcon( ":/kadas/themes/default/mActionMoveVertex.png" ), tr( "Continue drawing" ) );
      continueAction->setData( ContinueGeometry );
    }
  }
  else
  {
    QAction* addNodeAction = menu.addAction( QIcon( ":/kadas/themes/default/mActionAddVertex.png" ), tr( "Add node" ) );
    addNodeAction->setData( AddNode );
  }
}

void KadasMapToolDrawPolyLine::executeContextMenuAction( const KadasMapToolDrawShape::EditContext *context, int action, const QgsPointXY& pos )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( action == DeleteNode )
  {
    State* newstate = cloneState();
    newstate->points[ctx->part].removeAt( ctx->node );
    mStateStack.updateState( newstate );
  }
  else if ( action == AddNode )
  {
    // Find closest segment
    int closestIdx = -1;
    double closestDist = std::numeric_limits<double>::max();
    State* newstate = cloneState();
    const QList<QgsPointXY>& poly = newstate->points[ctx->part];
    for ( int m = poly.size(), j = m - 1, k = 0; k < m - 1; j = k++ )
    {
      QgsPointXY pProj = projPointOnSegment( pos, poly[j], poly[k] );
      double dist = pProj.sqrDist( pos );
      if ( dist < closestDist )
      {
        closestIdx = k;
        closestDist = dist;
      }
    }
    if ( closestIdx != -1 )
    {
      newstate->points[ctx->part].insert( closestIdx, pos );
      mStateStack.updateState( newstate );
    }
  }
  else if ( action == ContinueGeometry )
  {
    State* newstate = cloneState();
    if ( !mIsArea )
    {
      // Reverse points if continuing from start
      if ( ctx->node == 0 )
      {
        QList<QgsPointXY> reversed;
        for ( const QgsPointXY& p : newstate->points[ctx->part] )
        {
          reversed.prepend( p );
        }
        newstate->points[ctx->part] = reversed;
      }
    }
    else
    {
      // Rearrange points so that picked point is last
      QList<QgsPointXY> rearranged;
      int n = newstate->points[ctx->part].size();
      for ( int i = ( ctx->node + 1 ) % n; i  != ctx->node; i = ( i + 1 ) % n )
      {
        rearranged.append( newstate->points[ctx->part][i] );
      }
      rearranged.append( newstate->points[ctx->part][ctx->node] );
      newstate->points[ctx->part] = rearranged;
    }
    newstate->points[ctx->part].append( newstate->points[ctx->part].last() );
    newstate->status = StatusDrawing;
    mStateStack.updateState( newstate );
  }
}

void KadasMapToolDrawPolyLine::setPart( int part, const QList<QgsPointXY>& p )
{
  State* newState = cloneState();
  newState->points[part] = p;
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

KadasMapToolDrawRectangle::KadasMapToolDrawRectangle( QgsMapCanvas* canvas )
    : KadasMapToolDrawShape( canvas, true, emptyState() ) {}

KadasMapToolDrawShape::State* KadasMapToolDrawRectangle::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  return newState;
}

void KadasMapToolDrawRectangle::buttonEvent(const QgsPointXY &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton && state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->p1.append( pos );
    newState->p2.append( pos );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( !press && state()->status == StatusDrawing && state()->p1.back() != pos )
  {
    State* newState = cloneState();
    newState->p2.back() = pos;
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void KadasMapToolDrawRectangle::moveEvent(const QgsPointXY &pos )
{
  mutableState()->p2.back() = pos;
  update();
}

QgsAbstractGeometry* KadasMapToolDrawRectangle::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs, QgsProject::instance() );
  QgsGeometryCollection* multiGeom = new QgsMultiPolygon();
  for ( int i = 0, n = state()->p1.size(); i < n; ++i )
  {
    const QgsPointXY& p1 = state()->p1[i];
    const QgsPointXY& p2 = state()->p2[i];
    QgsLineString* ring = new QgsLineString();
    ring->addVertex( QgsPoint( t.transform( p1 ) ) );
    ring->addVertex( QgsPoint( t.transform( p2.x(), p1.y() ) ) );
    ring->addVertex( QgsPoint( t.transform( p2 ) ) );
    ring->addVertex( QgsPoint( t.transform( p1.x(), p2.y() ) ) );
    ring->addVertex( QgsPoint( t.transform( p1 ) ) );
    QgsPolygon* poly = new QgsPolygon();
    poly->setExteriorRing( ring );
    multiGeom->addGeometry( poly );
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometry* geom = multiGeom->isEmpty() ? new QgsPolygon() : multiGeom->geometryN( 0 )->clone();
    delete multiGeom;
    return geom;
  }
}

void KadasMapToolDrawRectangle::doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t )
{
  State* newState = cloneState();
  for ( int iPart = 0, nParts = geometry->partCount(); iPart < nParts; ++iPart )
  {
    for ( int iRing = 0, nRings = geometry->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      if ( geometry->vertexCount( iPart, iRing ) == 5 )
      {
        QgsPoint vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, 0 ) );
        newState->p1.append( t.transform( vertex ) );
        vertex = geometry->vertexAt( QgsVertexId( iPart, iRing, 2 ) );
        newState->p2.append( t.transform( vertex ) );
      }
    }
  }
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void KadasMapToolDrawRectangle::initInputWidget()
{
  mXEdit = new KadasFloatingInputWidgetField();
  connect( mXEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawRectangle::inputChanged );
  connect( mXEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawRectangle::acceptInput );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new KadasFloatingInputWidgetField();
  connect( mYEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawRectangle::inputChanged );
  connect( mYEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawRectangle::acceptInput );
  mInputWidget->addInputField( "y:", mYEdit );
}

void KadasMapToolDrawRectangle::updateInputWidget(const QgsPointXY &mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QgsUnitTypes::DistanceDegrees;
  mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
  mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void KadasMapToolDrawRectangle::updateInputWidget( const KadasMapToolDrawShape::EditContext* context )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  QgsPointXY pos;
  if ( ctx->point == 0 )
  {
    pos = state()->p1[ctx->part];
  }
  else if ( ctx->point == 1 )
  {
    pos = QgsPoint( state()->p2[ctx->part].x(), state()->p1[ctx->part].y() );
  }
  else if ( ctx->point == 2 )
  {
    pos = state()->p2[ctx->part];
  }
  else if ( ctx->point == 3 )
  {
    pos = QgsPoint( state()->p1[ctx->part].x(), state()->p2[ctx->part].y() );
  }
  updateInputWidget( pos );
}

void KadasMapToolDrawRectangle::inputAccepted()
{
  if ( state()->status >= StatusEditingReady )
  {
    return;
  }
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  mInputWidget->setFocusedInputField( mXEdit );
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->p1.append( QgsPoint( x, y ) );
    newState->p2.append( QgsPoint( x, y ) );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    newState->p2.back() = QgsPoint( x, y );
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void KadasMapToolDrawRectangle::inputChanged()
{
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  moveMouseToPos( QgsPoint( x, y ) );
  if ( currentEditContext() )
  {
    edit( currentEditContext(), QgsPoint( x, y ), QgsVector( 0, 0 ) );
  }
}


KadasMapToolDrawShape::EditContext* KadasMapToolDrawRectangle::getEditContext(const QgsPointXY &pos ) const
{
  int closestPart = -1;
  int closestPoint = -1;
  double closestDistance = std::numeric_limits<double>::max();
  QList< QList<QgsPointXY> > points;
  for ( int i = 0, n = state()->p1.size(); i < n; ++i )
  {
    points.append( QList<QgsPointXY>()
                   << state()->p1[i]
                   << QgsPoint( state()->p2[i].x(), state()->p1[i].y() )
                   << state()->p2[i]
                   << QgsPoint( state()->p1[i].x(), state()->p2[i].y() ) );
    for ( int j = 0, m = points[i].size(); j < m; ++j )
    {
      double dist = points[i][j].sqrDist( pos );
      if ( dist < closestDistance )
      {
        closestDistance = dist;
        closestPart = i;
        closestPoint = j;
      }
    }
  }
  // Check if a node was picked
  if ( closestPart != -1 )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( points[closestPart][closestPoint] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->point = closestPoint;
      return context;
    }
  }
  // Check whether entire geometry was picked
  for ( int i = 0, n = points.size(); i < n; ++i )
  {
    if ( pointInPolygon( pos, points[i] ) )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->point = -1;
      context->move = true;
      return context;
    }
  }
  return 0;
}

void KadasMapToolDrawRectangle::edit(const KadasMapToolDrawShape::EditContext* context, const QgsPointXY &pos, const QgsVector& delta )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( ctx->point == 0 )
  {
    mutableState()->p1[ctx->part] = pos;
  }
  else if ( ctx->point == 1 )
  {
    mutableState()->p2[ctx->part].setX( pos.x() );
    mutableState()->p1[ctx->part].setY( pos.y() );
  }
  else if ( ctx->point == 2 )
  {
    mutableState()->p2[ctx->part] = pos;
  }
  else if ( ctx->point == 3 )
  {
    mutableState()->p1[ctx->part].setX( pos.x() );
    mutableState()->p2[ctx->part].setY( pos.y() );
  }
  else if ( ctx->point == -1 )
  {
    mutableState()->p1[ctx->part] += delta;
    mutableState()->p2[ctx->part] += delta;
  }
  update();
}

void KadasMapToolDrawRectangle::setPart(int part, const QgsPointXY &p1, const QgsPointXY &p2 )
{
  State* newState = cloneState();
  newState->p1[part] = p1;
  newState->p2[part] = p2;
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

class GeodesicCircleMeasurer : public KadasGeometryRubberBand::Measurer
{
  public:
    GeodesicCircleMeasurer( KadasMapToolDrawCircle* tool ) : mTool( tool ) {}
    QList<Measurement> measure( KadasGeometryRubberBand::MeasurementMode measurementMode, int part, const QgsAbstractGeometry* /*geometry*/, QList<double>& partMeasurements )
    {
      QList<Measurement> measurements;
      if ( measurementMode == KadasGeometryRubberBand::MEASURE_CIRCLE )
      {
        const QgsPointXY& c = mTool->state()->centers[mTool->mPartMap[part]];
        const QgsPointXY& p = mTool->state()->ringPos[mTool->mPartMap[part]];
        QgsCoordinateTransform t1( mTool->canvas()->mapSettings().destinationCrs(), QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() );
        double radius = mTool->mDa.measureLine( t1.transform( c ), t1.transform( p ) );

        double area = radius * radius * M_PI;
        partMeasurements.append( area );

        Measurement areaMeasurement = {Measurement::Area, "", area};
        measurements.append( areaMeasurement );
        Measurement radiusMeasurement = {Measurement::Length, QObject::tr( "Radius" ), radius};
        measurements.append( radiusMeasurement );
      }
      return measurements;
    }
  private:
    KadasMapToolDrawCircle* mTool;
};

KadasMapToolDrawCircle::KadasMapToolDrawCircle( QgsMapCanvas* canvas , bool geodesic )
    : KadasMapToolDrawShape( canvas, true, emptyState() ), mGeodesic( geodesic )
{
  if ( geodesic )
  {
    mDa.setEllipsoid( "WGS84" );
    mDa.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );
    mRubberBand->setMeasurer( new GeodesicCircleMeasurer( this ) );
  }
}

KadasMapToolDrawShape::State* KadasMapToolDrawCircle::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  return newState;
}

void KadasMapToolDrawCircle::buttonEvent(const QgsPointXY &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton && state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->centers.append( pos );
    newState->ringPos.append( pos );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( !press && state()->status == StatusDrawing && state()->centers.back() != pos )
  {
    State* newState = cloneState();
    newState->ringPos.back() = pos;
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void KadasMapToolDrawCircle::moveEvent(const QgsPointXY &pos )
{
  mutableState()->ringPos.back() = pos;
  update();
}

void KadasMapToolDrawCircle::getPart( int part, QgsPointXY &center, double &radius ) const
{
  center = state()->centers[part];
  radius = qSqrt( state()->centers[part].sqrDist( state()->ringPos[part] ) );
}

QgsAbstractGeometry* KadasMapToolDrawCircle::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  mPartMap.clear();
  QgsGeometryCollection* multiGeom = new QgsMultiPolygon();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    // 1 deg segmentized circle around center
    if ( mGeodesic )
    {
      QgsCoordinateTransform t1( canvas()->mapSettings().destinationCrs(), QgsCoordinateReferenceSystem( "EPSG:4326"), QgsProject::instance() );
      QgsCoordinateTransform t2( QgsCoordinateReferenceSystem( "EPSG:4326"), targetCrs, QgsProject::instance() );
      const QgsPointXY& center = state()->centers[i];
      const QgsPointXY& ringPos = state()->ringPos[i];
      QgsPointXY p1 = t1.transform( center );
      QgsPointXY p2 = t1.transform( ringPos );
      double clampLatitude = targetCrs.authid() == "EPSG:3857" ? 85 : 90;
      if ( p2.y() > 90 )
      {
        p2.setY( 90. - ( p2.y() - 90. ) );
      }
      if ( p2.y() < -90 )
      {
        p2.setY( -90. - ( p2.y() + 90. ) );
      }
      double radius = mDa.measureLine( p1, p2 );
      QList<QgsPointXY> wgsPoints;
      for ( int a = 0; a < 360; ++a )
      {
        wgsPoints.append( mDa.destination( p1, radius, a ) );
      }
      // Check if area would cross north or south pole
      // -> Check if destination point at bearing 0 / 180 with given radius would flip longitude
      // -> If crosses north/south pole, add points at lat 90 resp. -90 between points with max resp. min latitude
      QgsPointXY pn = mDa.destination( p1, radius, 0 );
      QgsPointXY ps = mDa.destination( p1, radius, 180 );
      int shift = 0;
      int nPoints = wgsPoints.size();
      if ( qFuzzyCompare( qAbs( pn.x() - p1.x() ), 180 ) )   // crosses north pole
      {
        wgsPoints[nPoints-1].setX( p1.x() - 179.999 );
        wgsPoints[1].setX( p1.x() + 179.999 );
        wgsPoints.append( QgsPoint( p1.x() - 179.999, clampLatitude ) );
        wgsPoints[0] = QgsPoint( p1.x() + 179.999, clampLatitude );
        wgsPoints.prepend( QgsPoint( p1.x(), clampLatitude ) ); // Needed to ensure first point does not overflow in longitude below
        wgsPoints.append( QgsPoint( p1.x(), clampLatitude ) ); // Needed to ensure last point does not overflow in longitude below
        shift = 3;
      }
      if ( qFuzzyCompare( qAbs( ps.x() - p1.x() ), 180 ) )   // crosses south pole
      {
        wgsPoints[181 + shift].setX( p1.x() - 179.999 );
        wgsPoints[179 + shift].setX( p1.x() + 179.999 );
        wgsPoints[180 + shift] = QgsPoint( p1.x() - 179.999, -clampLatitude );
        wgsPoints.insert( 180 + shift, QgsPoint( p1.x() + 179.999, -clampLatitude ) );
      }
      // Check if area overflows in longitude
      // 0: left-overflow, 1: center, 2: right-overflow
      QList<QgsPointXY> poly[3];
      int current = 1;
      poly[1].append( wgsPoints[0] ); // First point is always in center region
      nPoints = wgsPoints.size();
      for ( int j = 1; j < nPoints; ++j )
      {
        const QgsPointXY& p = wgsPoints[j];
        if ( p.x() > 180. )
        {
          // Right-overflow
          if ( current == 1 )
          {
            poly[1].append( QgsPoint( 180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            poly[2].append( QgsPoint( -180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            current = 2;
          }
          poly[2].append( QgsPoint( p.x() - 360., p.y() ) );
        }
        else if ( p.x() < -180 )
        {
          // Left-overflow
          if ( current == 1 )
          {
            poly[1].append( QgsPoint( -180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            poly[0].append( QgsPoint( 180, 0.5 * ( poly[1].back().y() + p.y() ) ) );
            current = 0;
          }
          poly[0].append( QgsPoint( p.x() + 360., p.y() ) );
        }
        else
        {
          // No overflow
          if ( current == 0 )
          {
            poly[0].append( QgsPoint( 180, 0.5 * ( poly[0].back().y() + p.y() ) ) );
            poly[1].append( QgsPoint( -180, 0.5 * ( poly[0].back().y() + p.y() ) ) );
            current = 1;
          }
          else if ( current == 2 )
          {
            poly[2].append( QgsPoint( -180, 0.5 * ( poly[2].back().y() + p.y() ) ) );
            poly[1].append( QgsPoint( 180, 0.5 * ( poly[2].back().y() + p.y() ) ) );
            current = 1;
          }
          poly[1].append( p );
        }
      }
      for ( int j = 0; j < 3; ++j )
      {
        if ( !poly[j].isEmpty() )
        {
          QgsPointSequence points;
          for ( int k = 0, n = poly[j].size(); k < n; ++k )
          {
            poly[j][k].setY( qMin( clampLatitude, qMax( -clampLatitude, poly[j][k].y() ) ) );
            try
            {
              points.append( QgsPoint( t2.transform( poly[j][k] ) ) );
            }
            catch ( ... )
            {}
          }
          QgsLineString* ring = new QgsLineString();
          ring->setPoints( points );
          QgsPolygon* poly = new QgsPolygon();
          poly->setExteriorRing( ring );
          multiGeom->addGeometry( poly );
          mPartMap.append( i );
        }
      }
    }
    else
    {
      QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs, QgsProject::instance() );
      const QgsPointXY& center = state()->centers[i];
      const QgsPointXY& ringPos = state()->ringPos[i];
      double radius = qSqrt( center.sqrDist( ringPos ) );
      QgsCircularString* ring = new QgsCircularString();
      ring->setPoints( QgsPointSequence()
                       << QgsPoint( t.transform( center.x() + radius, center.y() ) )
                       << QgsPoint( t.transform( center ) )
                       << QgsPoint( t.transform( center.x() + radius, center.y() ) ) );
      QgsCurvePolygon* poly = new QgsCurvePolygon();
      poly->setExteriorRing( ring );
      multiGeom->addGeometry( poly );
    }
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometry* geom = multiGeom->isEmpty() ? new QgsCurvePolygon : multiGeom->geometryN( 0 )->clone();
    delete multiGeom;
    return geom;
  }
}

void KadasMapToolDrawCircle::doAddGeometry( const QgsAbstractGeometry* geometry, const QgsCoordinateTransform& t )
{
  State* newState = cloneState();
  if ( dynamic_cast<const QgsGeometryCollection*>( geometry ) )
  {
    const QgsGeometryCollection* geomCollection = static_cast<const QgsGeometryCollection*>( geometry );
    for ( int i = 0, n = geomCollection->numGeometries(); i < n; ++i )
    {
      QgsRectangle bb = geomCollection->geometryN( i )->boundingBox();
      QgsPointXY c = t.transform( bb.center() );
      QgsPointXY r = t.transform( QgsPoint( bb.xMaximum(), bb.center().y() ) );
      newState->centers.append( c );
      newState->ringPos.append( r );
    }
  }
  else
  {
    QgsRectangle bb = geometry->boundingBox();
    QgsPointXY c = t.transform( bb.center() );
    QgsPointXY r = t.transform( QgsPoint( bb.xMaximum(), bb.center().y() ) );
    newState->centers.append( c );
    newState->ringPos.append( r );
  }
  newState->status = mMultipart ? StatusReady : StatusFinished;
  mStateStack.updateState( newState );
}

void KadasMapToolDrawCircle::initInputWidget()
{
  mXEdit = new KadasFloatingInputWidgetField();
  connect( mXEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircle::centerInputChanged );
  connect( mXEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircle::acceptInput );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new KadasFloatingInputWidgetField();
  connect( mYEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircle::centerInputChanged );
  connect( mYEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircle::acceptInput );
  mInputWidget->addInputField( "y:", mYEdit );
  QDoubleValidator* validator = new QDoubleValidator();
  validator->setBottom( 0 );
  mREdit = new KadasFloatingInputWidgetField( validator );
  connect( mREdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircle::radiusInputChanged );
  connect( mREdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircle::acceptInput );
  mInputWidget->addInputField( "r:", mREdit );
}

void KadasMapToolDrawCircle::updateInputWidget(const QgsPointXY &mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QgsUnitTypes::DistanceDegrees;
  if ( state()->status == StatusReady )
  {
    mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
    mREdit->setText( "0" );
  }
  else if ( state()->status == StatusDrawing )
  {
    mREdit->setText( QString::number( qSqrt( state()->centers.last().sqrDist( state()->ringPos.last() ) ), 'f', isDegrees ? 4 : 0 ) );
  }
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void KadasMapToolDrawCircle::updateInputWidget( const KadasMapToolDrawShape::EditContext *context )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QgsUnitTypes::DistanceDegrees;
  if ( ctx->point == 0 )
  {
    const QgsPointXY& pos = state()->centers[ctx->part];
    mXEdit->setText( QString::number( pos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( pos.y(), 'f', isDegrees ? 4 : 0 ) );
    mInputWidget->setInputFieldVisible( 0, true );
    mInputWidget->setInputFieldVisible( 1, true );
    mInputWidget->setInputFieldVisible( 2, false );
  }
  else if ( ctx->point == 1 )
  {
    mREdit->setText( QString::number( qSqrt( state()->centers[ctx->part].sqrDist( state()->ringPos[ctx->part] ) ), 'f', 0 ) );
    mInputWidget->setInputFieldVisible( 0, false );
    mInputWidget->setInputFieldVisible( 1, false );
    mInputWidget->setInputFieldVisible( 2, true );
  }
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void KadasMapToolDrawCircle::inputAccepted()
{
  if ( state()->status >= StatusEditingReady )
  {
    return;
  }
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  mInputWidget->setFocusedInputField( mXEdit );
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->centers.append( QgsPoint( x, y ) );
    newState->ringPos.append( QgsPoint( x + r, y ) );
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    mREdit->setText( "0" );
    newState->centers.back() = QgsPoint( x, y );
    newState->ringPos.back() = QgsPoint( x + r, y );
    newState->status = mMultipart ? StatusReady : StatusFinished;
    mStateStack.updateState( newState );
  }
}

void KadasMapToolDrawCircle::centerInputChanged()
{
  State* state = mutableState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->ringPos.append( QgsPoint( x + r, y ) );
    state->status = StatusDrawing;
    moveMouseToPos( QgsPoint( x + r, y ) );
  }
  else if ( state->status == StatusDrawing )
  {
    state->centers.back() = QgsPoint( x, y );
    moveMouseToPos( QgsPoint( x + r, y ) );
  }
  else if ( currentEditContext() )
  {
    const EditContext* ctx = static_cast<const EditContext*>( currentEditContext() );
    QgsVector delta = QgsPointXY( x, y ) - state->centers[ctx->part];
    state->centers[ctx->part] += delta;
    state->ringPos[ctx->part] += delta;
    moveMouseToPos( state->centers[ctx->part] );
  }
  update();
}

void KadasMapToolDrawCircle::radiusInputChanged()
{
  State* state = mutableState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->ringPos.append( QgsPoint( x + r, y ) );
    state->status = StatusDrawing;
    moveMouseToPos( QgsPoint( x + r, y ) );
  }
  else if ( state->status == StatusDrawing )
  {
    state->ringPos.back() = QgsPoint( x + r, y );
    moveMouseToPos( QgsPoint( x + r, y ) );
  }
  else if ( currentEditContext() )
  {
    const EditContext* ctx = static_cast<const EditContext*>( currentEditContext() );
    QgsPointXY center = state->centers[ctx->part];
    state->ringPos[ctx->part] = QgsPoint( center.x() + r, center.y() );
    moveMouseToPos( state->ringPos[ctx->part] );
  }
  update();
}

KadasMapToolDrawShape::EditContext* KadasMapToolDrawCircle::getEditContext(const QgsPointXY &pos ) const
{
  int closestPart = -1;
  double closestDistance = std::numeric_limits<double>::max();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    double dist = state()->ringPos[i].sqrDist( pos );
    if ( dist < closestDistance )
    {
      closestDistance = dist;
      closestPart = i;
    }
  }
  // Check if a node was picked
  if ( closestPart != -1 )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( state()->ringPos[closestPart] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = closestPart;
      context->point = 1;
      return context;
    }
  }
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    QPoint p1 = toCanvasCoordinates( pos );
    QPoint p2 = toCanvasCoordinates( state()->centers[closestPart] );
    if ( qAbs(( p2 - p1 ).manhattanLength() ) < 10 )
    {
      EditContext* context = new EditContext;
      context->part = i;
      context->point = 0;
      return context;
    }
    double radiusSqr = state()->centers[i].sqrDist( state()->ringPos[i] );
    if ( pos.sqrDist( state()->centers[i] ) <= radiusSqr )
    {
      EditContext* context = new EditContext;
      context->part = i;
      context->point = -1;
      context->move = true;
      return context;
    }
  }
  return 0;
}

void KadasMapToolDrawCircle::edit(const KadasMapToolDrawShape::EditContext* context, const QgsPointXY &pos, const QgsVector& delta )
{
  const EditContext* ctx = static_cast<const EditContext*>( context );
  if ( ctx->point == 0 )
  {
    QgsVector delta = pos - state()->centers[ctx->part];
    mutableState()->centers[ctx->part] += delta;
    mutableState()->ringPos[ctx->part] += delta;
  }
  else if ( ctx->point == 1 )
  {
    mutableState()->ringPos[ctx->part] = pos;
  }
  else if ( ctx->point == -1 )
  {
    mutableState()->centers[ctx->part] += delta;
    mutableState()->ringPos[ctx->part] += delta;
  }
  update();
}

void KadasMapToolDrawCircle::setPart(int part, const QgsPointXY &center, double radius )
{
  State* newState = cloneState();
  newState->centers[part] = center;
  newState->ringPos[part] = QgsPoint( center.x() + radius, center.y() );
  mStateStack.updateState( newState );
}

///////////////////////////////////////////////////////////////////////////////

KadasMapToolDrawCircularSector::KadasMapToolDrawCircularSector( QgsMapCanvas* canvas )
    : KadasMapToolDrawShape( canvas, true, emptyState() ) {}

KadasMapToolDrawShape::State* KadasMapToolDrawCircularSector::emptyState() const
{
  State* newState = new State;
  newState->status = StatusReady;
  newState->sectorStatus = HaveNothing;
  return newState;
}

void KadasMapToolDrawCircularSector::buttonEvent(const QgsPointXY &pos, bool press, Qt::MouseButton button )
{
  if ( press && button == Qt::LeftButton )
  {
    if ( state()->sectorStatus == HaveNothing )
    {
      State* newState = cloneState();
      newState->centers.append( pos );
      newState->radii.append( 0 );
      newState->startAngles.append( 0 );
      newState->stopAngles.append( 0 );
      newState->sectorStatus = HaveCenter;
      newState->status = StatusDrawing;
      mStateStack.updateState( newState );
    }
    else if ( state()->sectorStatus == HaveCenter )
    {
      State* newState = cloneState();
      newState->sectorStatus = HaveRadius;
      newState->status = StatusDrawing;
      mStateStack.updateState( newState );
    }
    else if ( state()->sectorStatus == HaveRadius )
    {
      State* newState = cloneState();
      newState->sectorStatus = HaveNothing;
      newState->status = mMultipart ? StatusReady : StatusFinished;
      mStateStack.updateState( newState );
    }
  }
}

void KadasMapToolDrawCircularSector::moveEvent(const QgsPointXY &pos )
{
  State* state = mutableState();
  if ( state->sectorStatus == HaveCenter )
  {
    state->radii.back() = qSqrt( pos.sqrDist( state->centers.back() ) );
    state->startAngles.back() = state->stopAngles.back() = qAtan2( pos.y() - state->centers.back().y(), pos.x() - state->centers.back().x() );
  }
  else if ( state->sectorStatus == HaveRadius )
  {
    state->stopAngles.back() = qAtan2( pos.y() - state->centers.back().y(), pos.x() - state->centers.back().x() );
    if ( state->stopAngles.back() <= state->startAngles.back() )
    {
      state->stopAngles.back() += 2 * M_PI;
    }

    // Snap to full circle if within 5px
    const QgsPointXY& center = state->centers.back();
    const double& radius = state->radii.back();
    const double& startAngle = state->startAngles.back();
    const double& stopAngle = state->stopAngles.back();
    QgsPoint pStart( center.x() + radius * qCos( startAngle ),
                     center.y() + radius * qSin( startAngle ) );
    QgsPoint pEnd( center.x() + radius * qCos( stopAngle ),
                   center.y() + radius * qSin( stopAngle ) );
    QPoint diff = toCanvasCoordinates( pEnd ) - toCanvasCoordinates( pStart );
    if (( diff.x() * diff.x() + diff.y() * diff.y() ) < 25 )
    {
      state->stopAngles.back() = state->startAngles.back() + 2 * M_PI;
    }
  }
  update();
}

QgsAbstractGeometry* KadasMapToolDrawCircularSector::createGeometry( const QgsCoordinateReferenceSystem &targetCrs , QList<QgsVertexId> */*hiddenNodes*/ ) const
{
  QgsCoordinateTransform t( canvas()->mapSettings().destinationCrs(), targetCrs, QgsProject::instance() );
  QgsGeometryCollection* multiGeom = new QgsMultiPolygon();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    const QgsPointXY& center = state()->centers[i];
    const double& radius = state()->radii[i];
    const double& startAngle = state()->startAngles[i];
    const double& stopAngle = state()->stopAngles[i];
    QgsPointXY pStart, pMid, pEnd;
    if ( stopAngle == startAngle + 2 * M_PI )
    {
      pStart = pEnd = QgsPoint( center.x() + radius * qCos( stopAngle ),
                                center.y() + radius * qSin( stopAngle ) );
      pMid = center;
    }
    else
    {
      double alphaMid = 0.5 * ( startAngle + stopAngle - 2 * M_PI );
      pStart = QgsPoint( center.x() + radius * qCos( startAngle ),
                         center.y() + radius * qSin( startAngle ) );
      pMid = QgsPoint( center.x() + radius * qCos( alphaMid ),
                       center.y() + radius * qSin( alphaMid ) );
      pEnd = QgsPoint( center.x() + radius * qCos( stopAngle - 2 * M_PI ),
                       center.y() + radius * qSin( stopAngle - 2 * M_PI ) );
    }
    QgsCompoundCurve* exterior = new QgsCompoundCurve();
    if ( startAngle != stopAngle )
    {
      QgsCircularString* arc = new QgsCircularString();
      arc->setPoints( QgsPointSequence()
                      << QgsPoint(t.transform( pStart ))
                      << QgsPoint(t.transform( pMid ))
                      << QgsPoint( t.transform( pEnd )) );
      exterior->addCurve( arc );
    }
    if ( startAngle != stopAngle + 2 * M_PI )
    {
      QgsLineString* line = new QgsLineString();
      line->setPoints( QgsPointSequence()
                       << QgsPoint(t.transform( pEnd ))
                       << QgsPoint(t.transform( center ))
                       << QgsPoint(t.transform( pStart )) );
      exterior->addCurve( line );
    }
    QgsPolygon* poly = new QgsPolygon;
    poly->setExteriorRing( exterior );
    multiGeom->addGeometry( poly );
  }
  if ( mMultipart )
  {
    return multiGeom;
  }
  else
  {
    QgsAbstractGeometry* geom = multiGeom->isEmpty() ? new QgsPolygon() : multiGeom->geometryN( 0 )->clone();
    delete multiGeom;
    return geom;
  }
}

void KadasMapToolDrawCircularSector::doAddGeometry( const QgsAbstractGeometry* /*geometry*/, const QgsCoordinateTransform& /*t*/ )
{
  /* Not yet implemented */
}

void KadasMapToolDrawCircularSector::initInputWidget()
{
  mXEdit = new KadasFloatingInputWidgetField();
  connect( mXEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircularSector::centerInputChanged );
  connect( mXEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircularSector::acceptInput );
  mInputWidget->addInputField( "x:", mXEdit, true );
  mYEdit = new KadasFloatingInputWidgetField();
  connect( mYEdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircularSector::centerInputChanged );
  connect( mYEdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircularSector::acceptInput );
  mInputWidget->addInputField( "y:", mYEdit );
  QDoubleValidator* validator = new QDoubleValidator();
  validator->setBottom( 0 );
  mREdit = new KadasFloatingInputWidgetField( validator );
  connect( mREdit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircularSector::arcInputChanged );
  connect( mREdit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircularSector::acceptInput );
  mInputWidget->addInputField( "r:", mREdit );
  mA1Edit = new KadasFloatingInputWidgetField();
  connect( mA1Edit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircularSector::arcInputChanged );
  connect( mA1Edit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircularSector::acceptInput );
  mInputWidget->addInputField( QString( QChar( 0x03B1 ) ) + "1:", mA1Edit );
  mA2Edit = new KadasFloatingInputWidgetField();
  connect( mA2Edit, &KadasFloatingInputWidgetField::inputChanged, this, &KadasMapToolDrawCircularSector::arcInputChanged );
  connect( mA2Edit, &KadasFloatingInputWidgetField::inputConfirmed, this, &KadasMapToolDrawCircularSector::acceptInput );
  mInputWidget->addInputField( QString( QChar( 0x03B1 ) ) + "2:", mA2Edit );
}

void KadasMapToolDrawCircularSector::updateInputWidget(const QgsPointXY &mousePos )
{
  bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QgsUnitTypes::DistanceDegrees;
  if ( state()->sectorStatus == HaveNothing )
  {
    mXEdit->setText( QString::number( mousePos.x(), 'f', isDegrees ? 4 : 0 ) );
    mYEdit->setText( QString::number( mousePos.y(), 'f', isDegrees ? 4 : 0 ) );
    mREdit->setText( "0" );
    mA1Edit->setText( "0" );
    mA2Edit->setText( "0" );
  }
  else if ( state()->sectorStatus == HaveCenter )
  {
    mREdit->setText( QString::number( state()->radii.last(), 'f', isDegrees ? 4 : 0 ) );
  }
  else if ( state()->sectorStatus == HaveRadius )
  {
    double startAngle = 2.5 * M_PI - state()->startAngles.last();
    if ( startAngle > 2 * M_PI )
      startAngle -= 2 * M_PI;
    else if ( startAngle < 0 )
      startAngle += 2 * M_PI;
    mA1Edit->setText( QString::number( startAngle / M_PI * 180., 'f', 1 ) );
    double stopAngle = 2.5 * M_PI - state()->stopAngles.last();
    if ( stopAngle > 2 * M_PI )
      stopAngle -= 2 * M_PI;
    else if ( stopAngle < 0 )
      stopAngle += 2 * M_PI;
    mA2Edit->setText( QString::number( stopAngle / M_PI * 180., 'f', 1 ) );
  }
  if ( mInputWidget->focusedInputField() )
    mInputWidget->focusedInputField()->selectAll();
}

void KadasMapToolDrawCircularSector::updateInputWidget( const EditContext */*context*/ )
{
  /* Currently not implemented */
}

void KadasMapToolDrawCircularSector::inputAccepted()
{
  if ( state()->status >= StatusEditingReady )
  {
    return;
  }
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  double a1 = 2.5 * M_PI - mA1Edit->text().toDouble() / 180. * M_PI;
  double a2 = 2.5 * M_PI - mA2Edit->text().toDouble() / 180. * M_PI;
  while ( a1 < 0 )
    a1 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a1 -= 2 * M_PI;
  while ( a2 < 0 )
    a2 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a2 -= 2 * M_PI;
  if ( a2 <= a1 )
    a2 += 2 * M_PI;
  mInputWidget->setFocusedInputField( mXEdit );
  if ( state()->status == StatusReady )
  {
    State* newState = cloneState();
    newState->centers.append( QgsPoint( x, y ) );
    newState->radii.append( r );
    if ( r > 0 )
    {
      newState->startAngles.append( a1 );
      newState->stopAngles.append( a2 );
      newState->sectorStatus = HaveRadius;
    }
    else
    {
      newState->startAngles.append( 0 );
      newState->stopAngles.append( 0 );
      newState->sectorStatus = HaveCenter;
    }
    newState->status = StatusDrawing;
    mStateStack.updateState( newState );
  }
  else if ( state()->status == StatusDrawing )
  {
    State* newState = cloneState();
    newState->centers.back() = QgsPoint( x, y );
    newState->radii.back() = r;
    if ( r > 0 )
    {
      newState->startAngles.back() = a1;
      newState->stopAngles.back() = a2;
      newState->sectorStatus = HaveRadius;
    }
    else
    {
      newState->startAngles.back() = 0;
      newState->stopAngles.back() = 0;
      newState->sectorStatus = HaveCenter;
    }
    if ( newState->sectorStatus == HaveRadius )
    {
      mREdit->setText( "0" );
      mA1Edit->setText( "0" );
      mA2Edit->setText( "0" );
      newState->status = mMultipart ? StatusReady : StatusFinished;
    }
    mStateStack.updateState( newState );
  }
}

void KadasMapToolDrawCircularSector::centerInputChanged()
{
  State* state = mutableState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  double a1 = 2.5 * M_PI - mA1Edit->text().toDouble() / 180. * M_PI;
  double a2 = 2.5 * M_PI - mA2Edit->text().toDouble() / 180. * M_PI;
  while ( a1 < 0 )
    a1 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a1 -= 2 * M_PI;
  while ( a2 < 0 )
    a2 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a2 -= 2 * M_PI;
  if ( a2 <= a1 )
    a2 += 2 * M_PI;
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->radii.append( r );
    if ( r > 0 )
    {
      state->startAngles.append( a1 );
      state->stopAngles.append( a2 );
      state->sectorStatus = HaveRadius;
    }
    else
    {
      state->startAngles.append( 0 );
      state->stopAngles.append( 0 );
      state->sectorStatus = HaveCenter;
    }
    state->status = StatusDrawing;
  }
  state->centers.back() = QgsPoint( x, y );
  update();
  moveMouseToPos( QgsPoint( x + r * qCos( state->stopAngles.back() ), y + r * qSin( state->stopAngles.back() ) ) );
}

void KadasMapToolDrawCircularSector::arcInputChanged()
{
  State* state = mutableState();
  double x = mXEdit->text().toDouble();
  double y = mYEdit->text().toDouble();
  double r = mREdit->text().toDouble();
  double a1 = 2.5 * M_PI - mA1Edit->text().toDouble() / 180. * M_PI;
  double a2 = 2.5 * M_PI - mA2Edit->text().toDouble() / 180. * M_PI;
  while ( a1 < 0 )
    a1 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a1 -= 2 * M_PI;
  while ( a2 < 0 )
    a2 += 2 * M_PI;
  while ( a1 >= 2 * M_PI )
    a2 -= 2 * M_PI;
  if ( a2 <= a1 )
    a2 += 2 * M_PI;
  if ( state->status == StatusReady )
  {
    state->centers.append( QgsPoint( x, y ) );
    state->radii.append( r );
    if ( r > 0 )
    {
      state->startAngles.append( a1 );
      state->stopAngles.append( a2 );
      state->sectorStatus = HaveRadius;
    }
    else
    {
      state->startAngles.append( 0 );
      state->stopAngles.append( 0 );
      state->sectorStatus = HaveCenter;
    }
    state->status = StatusDrawing;
  }
  state->radii.back() = r;
  if ( r > 0 )
  {
    state->startAngles.back() = a1;
    state->stopAngles.back() = a2;
    state->sectorStatus = HaveRadius;
  }
  else
  {
    state->startAngles.back() = 0;
    state->stopAngles.back() = 0;
    state->sectorStatus = HaveCenter;
  }
  update();
  moveMouseToPos( QgsPoint( x + r * qCos( state->stopAngles.back() ), y + r * qSin( state->stopAngles.back() ) ) );
}

KadasMapToolDrawShape::EditContext* KadasMapToolDrawCircularSector::getEditContext(const QgsPointXY & /*pos*/ ) const
{
  /* Currently not implemented */
  return 0;
}

void KadasMapToolDrawCircularSector::edit(const EditContext* /*context*/, const QgsPointXY & /*pos*/, const QgsVector& /*delta*/ )
{
  /* Currently not implemented */
}

void KadasMapToolDrawCircularSector::setPart(int part, const QgsPointXY &center, double radius, double startAngle, double stopAngle )
{
  State* newState = cloneState();
  newState->centers[part] = center;
  newState->radii[part] = radius;
  newState->startAngles[part] = startAngle;
  newState->stopAngles[part] = stopAngle;
  mStateStack.updateState( newState );
}
