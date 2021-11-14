/* -*-c++-*- */
/* osgEarth - Dynamic map generation toolkit for OpenSceneGraph
 * Copyright 2016 Pelican Mapping
 * http://osgearth.org
 *
 * osgEarth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#ifndef OSG_QT_VIEWER_WIDGET_H
#define OSG_QT_VIEWER_WIDGET_H

#include "osgQtGraphicsWindow.h"


#include <osgEarth/Map>
#include <osgViewer/ViewerBase>

#include <QTimer>

/**
 * Qt widget that encapsulates an osgViewer::Viewer.
 */
class osgQtViewerWidget : public osgQtGLWidget
{
    Q_OBJECT;

  public:
    osgQtViewerWidget( osgViewer::ViewerBase *viewer );
    virtual ~osgQtViewerWidget();

  protected:

    QTimer _timer;

    void installFrameTimer();

    void createViewer();
    void reconfigure( osgViewer::View * );
    void paintEvent( QPaintEvent * );

    osg::observer_ptr<osgViewer::ViewerBase> _viewer;
    osg::ref_ptr<osg::GraphicsContext>       _gc;
};

#endif // OSG_QT_VIEWER_WIDGET_H
