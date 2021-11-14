/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSG_QT_GRAPHICS_WINDOW_H
#define OSG_QT_GRAPHICS_WINDOW_H

#include <osgViewer/GraphicsWindow>

#include <QMutex>
#include <QEvent>
#include <QQueue>
#include <QSet>
#include <QGLWidget>
#include <osg/Version>

class QInputEvent;
class QGestureEvent;

namespace osgViewer
{
  class ViewerBase;
}

// forward declarations
class osgQtGraphicsWindow;


class osgQtGLWidget : public QGLWidget
{
    typedef QGLWidget inherited;

  public:

    osgQtGLWidget( QWidget *parent = nullptr, const QGLWidget *shareWidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), bool forwardKeyEvents = false );
    osgQtGLWidget( const QGLFormat &format, QWidget *parent = nullptr, const QGLWidget *shareWidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), bool forwardKeyEvents = false );
    virtual ~osgQtGLWidget();

    inline void setGraphicsWindow( osgQtGraphicsWindow *gw ) { _gw = gw; }

    inline bool getForwardKeyEvents() const { return _forwardKeyEvents; }
    virtual void setForwardKeyEvents( bool f ) { _forwardKeyEvents = f; }

    inline bool getTouchEventsEnabled() const { return _touchEventsEnabled; }
    void setTouchEventsEnabled( bool e );

    void setKeyboardModifiers( QInputEvent *event );

    void keyPressEvent( QKeyEvent *event ) override;
    void keyReleaseEvent( QKeyEvent *event ) override;
    void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void mouseDoubleClickEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void wheelEvent( QWheelEvent *event ) override;
    bool gestureEvent( QGestureEvent *event );

  protected:

    int getNumDeferredEvents()
    {
      QMutexLocker lock( &_deferredEventQueueMutex );
      return _deferredEventQueue.count();
    }
    void enqueueDeferredEvent( QEvent::Type eventType, QEvent::Type removeEventType = QEvent::None )
    {
      QMutexLocker lock( &_deferredEventQueueMutex );

      if ( removeEventType != QEvent::None )
      {
        if ( _deferredEventQueue.removeOne( removeEventType ) )
          _eventCompressor.remove( eventType );
      }

      if ( _eventCompressor.find( eventType ) == _eventCompressor.end() )
      {
        _deferredEventQueue.enqueue( eventType );
        _eventCompressor.insert( eventType );
      }
    }
    void processDeferredEvents();

    friend class osgQtGraphicsWindow;
    osgQtGraphicsWindow *_gw;

    QMutex _deferredEventQueueMutex;
    QQueue<QEvent::Type> _deferredEventQueue;
    QSet<QEvent::Type> _eventCompressor;

    bool _touchEventsEnabled;

    bool _forwardKeyEvents;
    qreal _devicePixelRatio;

    virtual void resizeEvent( QResizeEvent *event );
    virtual void moveEvent( QMoveEvent *event );
    virtual void glDraw();
    virtual bool event( QEvent *event );
};


class osgQtGraphicsWindow : public osgViewer::GraphicsWindow
{
  public:
    osgQtGraphicsWindow( osg::GraphicsContext::Traits *traits, QWidget *parent = nullptr, const QGLWidget *shareWidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags() );
    virtual ~osgQtGraphicsWindow();

    struct WindowData : public osg::Referenced
    {
      WindowData( osgQtGLWidget *widget = nullptr, QWidget *parent = nullptr ): _widget( widget ), _parent( parent ) {}
      osgQtGLWidget *_widget;
      QWidget *_parent;
    };

    bool init( QWidget *parent, const QGLWidget *shareWidget, Qt::WindowFlags f );

    static QGLFormat traits2qglFormat( const osg::GraphicsContext::Traits *traits );
    static void qglFormat2traits( const QGLFormat &format, osg::GraphicsContext::Traits *traits );

    bool setWindowRectangleImplementation( int x, int y, int width, int height ) override;
    void getWindowRectangle( int &x, int &y, int &width, int &height ) override;
    bool setWindowDecorationImplementation( bool windowDecoration ) override;
    bool getWindowDecoration() const override;
    void grabFocus() override;
    void grabFocusIfPointerInWindow() override;
    void raiseWindow() override;
    void setWindowName( const std::string &name ) override;
    std::string getWindowName() override;
    void useCursor( bool cursorOn ) override;
    void setCursor( MouseCursor cursor ) override;
    inline bool getTouchEventsEnabled() const { return _widget->getTouchEventsEnabled(); }
    void setTouchEventsEnabled( bool e ) { _widget->setTouchEventsEnabled( e ); }


    bool valid() const override;
    bool realizeImplementation() override;
    bool isRealizedImplementation() const override;
    void closeImplementation() override;
    bool makeCurrentImplementation() override;
    bool releaseContextImplementation() override;
    void swapBuffersImplementation() override;
    void runOperations() override;

    void requestWarpPointer( float x, float y ) override;

  protected:
    friend class osgQtGLWidget;
    osgQtGLWidget *_widget;
    bool _ownsWidget;
    QCursor _currentCursor;
    bool _realized;
};

#endif // OSG_QT_GRAPHICS_WINDOW_H
