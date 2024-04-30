/***************************************************************************
  qgs3dmapcanvaswidget.h
  --------------------------------------
  Date                 : January 2022
  Copyright            : (C) 2022 by Belgacem Nedjima
  Email                : belgacem dot nedjima at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADAS3DMAPCANVASWIDGET_H
#define KADAS3DMAPCANVASWIDGET_H

#include <QMenu>
#include <QToolButton>
#include <QPointer>

#include <qgsdockwidget.h>
#include <qobjectuniqueptr.h>
#include <qgsrectangle.h>

#define SIP_NO_FILE

class QLabel;
class QProgressBar;

class Qgs3DAnimationWidget;
class Kadas3DMapCanvas;
class Qgs3DMapSettings;
class Qgs3DMapToolIdentify;
class Qgs3DMapToolMeasureLine;
class QgsMapCanvas;
class QgsDockableWidgetHelper;
class QgsMessageBar;
class QgsRubberBand;

class Kadas3DMapCanvasWidget : public QWidget
{
    Q_OBJECT
  public:
    Kadas3DMapCanvasWidget( const QString &name, bool isDocked );

    //! takes ownership
    void setMapSettings( Qgs3DMapSettings *map );

    void setMainCanvas( QgsMapCanvas *canvas );

    Kadas3DMapCanvas *mapCanvas3D() { return mCanvas; }

    Qgs3DAnimationWidget *animationWidget() { return mAnimationWidget; }

    Qgs3DMapToolMeasureLine *measurementLineTool() { return mMapToolMeasureLine; }

    QgsDockableWidgetHelper *dockableWidgetHelper() { return mDockableWidgetHelper; }

    void setCanvasName( const QString &name );
    QString canvasName() const { return mCanvasName; }

    void showAnimationWidget() { mActionAnim->trigger(); }

  protected:
    void resizeEvent( QResizeEvent *event ) override;

  private slots:
    void resetView();
    void configure();
    void saveAsImage();
    //void toggleAnimations();
    void cameraControl();
    //void identify();
    //void measureLine();
    //void exportScene();
    void toggleNavigationWidget( bool visibility );
    void toggleFpsCounter( bool visibility );

    void onMainCanvasLayersChanged();
    void onMainCanvasColorChanged();
    void onTotalPendingJobsCountChanged();
    void updateFpsCount( float fpsCount );
    void cameraNavigationSpeedChanged( double speed );
    void mapThemeMenuAboutToShow();
    //! Renames the active map theme called \a theme to \a newTheme
    void currentMapThemeRenamed( const QString &theme, const QString &newTheme );

    void onMainMapCanvasExtentChanged();
    void onViewed2DExtentFrom3DChanged( QVector<QgsPointXY> extent );
    void onViewFrustumVisualizationEnabledChanged();
    void onExtentChanged();
    void onGpuMemoryLimitReached();

  private:
    QString mCanvasName;
    Kadas3DMapCanvas *mCanvas = nullptr;
    Qgs3DAnimationWidget *mAnimationWidget = nullptr;
    QgsMapCanvas *mMainCanvas = nullptr;
    QProgressBar *mProgressPendingJobs = nullptr;
    QLabel *mLabelPendingJobs = nullptr;
    QLabel *mLabelFpsCounter = nullptr;
    QLabel *mLabelNavigationSpeed = nullptr;
    QTimer *mLabelNavSpeedHideTimeout = nullptr;
    Qgs3DMapToolIdentify *mMapToolIdentify = nullptr;
    Qgs3DMapToolMeasureLine *mMapToolMeasureLine = nullptr;
    QMenu *mMapThemeMenu = nullptr;
    QMenu *mOptionsMenu = nullptr;
    QList<QAction *> mMapThemeMenuPresetActions;
    QToolButton *mBtnMapThemes = nullptr;
    QAction *mActionEnableShadows = nullptr;
    QAction *mActionEnableEyeDome = nullptr;
    QAction *mActionEnableAmbientOcclusion = nullptr;
    QAction *mActionSync2DNavTo3D = nullptr;
    QAction *mActionSync3DNavTo2D = nullptr;
    QAction *mShowFrustumPolyogon = nullptr;
    QAction *mActionAnim = nullptr;
    QToolButton *mBtnOptions = nullptr;
    QgsDockableWidgetHelper *mDockableWidgetHelper = nullptr;
    QObjectUniquePtr< QgsRubberBand > mViewFrustumHighlight;
    QObjectUniquePtr< QgsRubberBand > mViewExtentHighlight;
    QPointer<QDialog> mConfigureDialog;
    QgsMessageBar *mMessageBar = nullptr;
    bool mGpuMemoryLimitReachedReported = false;
};

#endif // KADAS3DMAPCANVASWIDGET_H
