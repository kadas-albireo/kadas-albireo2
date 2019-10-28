/***************************************************************************
    kadasglobeinteractionhandlers.h
    -------------------------------
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

#ifndef KADASGLOBEINTERACTIONHANDLERS_H
#define KADASGLOBEINTERACTIONHANDLERS_H

#include <osgEarthUtil/Controls>

namespace osgEarth { namespace Util { class EarthManipulator; } }
class KadasGlobeIntegration;


class KadasGlobeNavigationControlHandler : public osgEarth::Util::Controls::ControlEventHandler
{
  public:
    virtual void onMouseDown() { }
    virtual void onClick( const osgGA::GUIEventAdapter & /*ea*/, osgGA::GUIActionAdapter & /*aa*/ ) {}
};


class KadasGlobeZoomControlHandler : public KadasGlobeNavigationControlHandler
{
  public:
    KadasGlobeZoomControlHandler( osgEarth::Util::EarthManipulator *manip, double dx, double dy )
      : _manip( manip ), _dx( dx ), _dy( dy ) { }
    void onMouseDown() override;
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
    double _dx;
    double _dy;
};


class KadasGlobeHomeControlHandler : public KadasGlobeNavigationControlHandler
{
  public:
    KadasGlobeHomeControlHandler( osgEarth::Util::EarthManipulator *manip ) : _manip( manip ) { }
    void onClick( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa ) override;
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
};


class KadasGlobeSyncExtentControlHandler : public KadasGlobeNavigationControlHandler
{
  public:
    KadasGlobeSyncExtentControlHandler( KadasGlobeIntegration *globe ) : mGlobe( globe ) { }
    void onClick( const osgGA::GUIEventAdapter & /*ea*/, osgGA::GUIActionAdapter & /*aa*/ ) override;
  private:
    KadasGlobeIntegration *mGlobe = nullptr;
};


class KadasGlobePanControlHandler : public KadasGlobeNavigationControlHandler
{
  public:
    KadasGlobePanControlHandler( osgEarth::Util::EarthManipulator *manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
    void onMouseDown() override;
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
    double _dx;
    double _dy;
};


class KadasGlobeRotateControlHandler : public KadasGlobeNavigationControlHandler
{
  public:
    KadasGlobeRotateControlHandler( osgEarth::Util::EarthManipulator *manip, double dx, double dy ) : _manip( manip ), _dx( dx ), _dy( dy ) { }
    void onMouseDown() override;
  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
    double _dx;
    double _dy;
};


class KadasGlobeQueryCoordinatesHandler : public osgGA::GUIEventHandler
{
  public:
    KadasGlobeQueryCoordinatesHandler( KadasGlobeIntegration *globe ) :  mGlobe( globe ) { }

    bool handle( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa );

  private:
    KadasGlobeIntegration *mGlobe = nullptr;
};


class KadasGlobeKeyboardControlHandler : public osgGA::GUIEventHandler
{
  public:
    KadasGlobeKeyboardControlHandler( osgEarth::Util::EarthManipulator *manip ) : _manip( manip ) { }

    bool handle( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa ) override;

  private:
    osg::observer_ptr<osgEarth::Util::EarthManipulator> _manip;
};


class KadasGlobeNavigationControl : public osgEarth::Util::Controls::ImageControl
{
  public:
    KadasGlobeNavigationControl( osg::Image *image = 0 ) : ImageControl( image ),  mMousePressed( false ) {}

  protected:
    bool handle( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa, osgEarth::Util::Controls::ControlContext &cx ) override;

  private:
    bool mMousePressed;
};


#endif // KADASGLOBEINTERACTIONHANDLERS_H
