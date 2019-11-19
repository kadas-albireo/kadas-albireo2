/***************************************************************************
    kadasglobeinteractionhandlers.cpp
    ---------------------------------
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

#include <QTimer>
#include <osgEarthUtil/EarthManipulator>

#include <kadas/app/globe/kadasglobeintegration.h>
#include <kadas/app/globe/kadasglobeinteractionhandlers.h>

#define MOVE_DELTA 0.05

void KadasGlobeZoomControlHandler::onMouseDown()
{
  _manip->zoom( _dx * MOVE_DELTA, _dy * MOVE_DELTA );
}

void KadasGlobeHomeControlHandler::onClick( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa )
{
  _manip->home( ea, aa );
}

void KadasGlobeSyncExtentControlHandler::onClick( const osgGA::GUIEventAdapter & /*ea*/, osgGA::GUIActionAdapter & /*aa*/ )
{
  mGlobe->syncExtent();
}

void KadasGlobePanControlHandler::onMouseDown()
{
  _manip->pan( _dx * MOVE_DELTA, _dy * MOVE_DELTA );
}

void KadasGlobeRotateControlHandler::onMouseDown()
{
  if ( 0 == _dx && 0 == _dy )
    _manip->setRotation( osg::Quat() );
  else
    _manip->rotate( _dx * MOVE_DELTA, _dy * MOVE_DELTA );
}

bool KadasGlobeQueryCoordinatesHandler::handle( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa )
{
  if ( ea.getEventType() == osgGA::GUIEventAdapter::MOVE )
  {
    osgViewer::View *view = static_cast<osgViewer::View *>( aa.asView() );
    osgUtil::LineSegmentIntersector::Intersections hits;
    if ( view->computeIntersections( ea.getX(), ea.getY(), hits ) )
    {
      osgEarth::GeoPoint isectPoint;
      isectPoint.fromWorld( mGlobe->mapNode()->getMapSRS()->getGeodeticSRS(), hits.begin()->getWorldIntersectPoint() );
      mGlobe->showCurrentCoordinates( isectPoint.x(), isectPoint.y() );
    }
  }
  return false;
}

bool KadasGlobeKeyboardControlHandler::handle( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa )
{
  if ( ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN )
  {
    //move map
    if ( ea.getKey() == '4' )
      _manip->pan( -MOVE_DELTA, 0 );
    else if ( ea.getKey() == '6' )
      _manip->pan( MOVE_DELTA, 0 );
    else if ( ea.getKey() == '2' )
      _manip->pan( 0, MOVE_DELTA );
    else if ( ea.getKey() == '8' )
      _manip->pan( 0, -MOVE_DELTA );
    //rotate
    else if ( ea.getKey() == '/' )
      _manip->rotate( MOVE_DELTA, 0 );
    else if ( ea.getKey() == '*' )
      _manip->rotate( -MOVE_DELTA, 0 );
    //tilt
    else if ( ea.getKey() == '9' )
      _manip->rotate( 0, MOVE_DELTA );
    else if ( ea.getKey() == '3' )
      _manip->rotate( 0, -MOVE_DELTA );
    //zoom
    else if ( ea.getKey() == '-' )
      _manip->zoom( 0, MOVE_DELTA );
    else if ( ea.getKey() == '+' )
      _manip->zoom( 0, -MOVE_DELTA );
    //reset
    else if ( ea.getKey() == '5' )
      _manip->home( ea, aa );
  }
  return false;
}

KadasGlobeNavigationControl::~KadasGlobeNavigationControl()
{
  delete mIntervalTimer;
}

bool KadasGlobeNavigationControl::handle( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa, osgEarth::Util::Controls::ControlContext &cx )
{
  if ( ea.getEventType() == osgGA::GUIEventAdapter::PUSH )
  {
    mMousePressed = true;
    delete mIntervalTimer;
    mIntervalTimer = new QTimer();
    QObject::connect( mIntervalTimer, &QTimer::timeout, mIntervalTimer, [this] { mOsgViewer->requestRedraw(); } );
    mIntervalTimer->setSingleShot( false );
    mIntervalTimer->start( 10 );
  }
  else if ( ea.getEventType() == osgGA::GUIEventAdapter::FRAME && mMousePressed )
  {
    float canvasY = cx._vp->height() - ( ea.getY() - cx._view->getCamera()->getViewport()->y() );
    float canvasX = ea.getX() - cx._view->getCamera()->getViewport()->x();

    if ( intersects( canvasX, canvasY ) )
    {
      for ( osgEarth::Util::Controls::ControlEventHandlerList::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i )
      {
        KadasGlobeNavigationControlHandler *handler = dynamic_cast<KadasGlobeNavigationControlHandler *>( i->get() );
        if ( handler )
        {
          handler->onMouseDown();
        }
      }
    }
  }
  else if ( ea.getEventType() == osgGA::GUIEventAdapter::RELEASE )
  {
    for ( osgEarth::Util::Controls::ControlEventHandlerList::const_iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i )
    {
      KadasGlobeNavigationControlHandler *handler = dynamic_cast<KadasGlobeNavigationControlHandler *>( i->get() );
      if ( handler )
      {
        handler->onClick( ea, aa );
      }
    }
    mMousePressed = false;
    delete mIntervalTimer;
    mIntervalTimer = nullptr;
  }
  return Control::handle( ea, aa, cx );
}
