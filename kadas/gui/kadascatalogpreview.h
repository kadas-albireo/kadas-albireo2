/***************************************************************************
    kadascatalogpreview.h
    ---------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASCATALOGPREVIEW_H
#define KADASCATALOGPREVIEW_H

#include <memory>

#include <QImage>
#include <QObject>
#include <QPointer>

#include <qgis/qgis.h>
#include <qgis/qgscoordinatetransformcontext.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadascataloglayersource.h"

class QLabel;
class QThread;
class QgsMapCanvas;
class QgsMapLayer;
class QgsMapRendererParallelJob;
class QgsMessageBar;
class QgsMessageBarItem;
class KadasCatalogPreviewCanvasItem;

/**
 * A built preview layer, shared so that it is released together with the
 * queued signal carrying it whenever nobody takes it over.
 */
using KadasCatalogPreviewLayer = std::shared_ptr<QgsMapLayer>;

/**
 * Instantiates preview layers, living on the worker thread of a
 * KadasCatalogPreview.
 */
class KADAS_GUI_EXPORT KadasCatalogPreviewBuilder : public QObject
{
    Q_OBJECT
  public:
    /**
     * Builds the layer for \a source and announces it through built(), tagged
     * with \a generation so a superseded result can be recognized.
     */
    void build( int generation, const KadasCatalogLayerSource &source, const QgsCoordinateTransformContext &transformContext );

  signals:
    //! Emitted once \a layer is built; \a layer is null if the source made no layer.
    void built( int generation, KadasCatalogPreviewLayer layer );
};

/**
 * Renders a catalog entry over the map canvas without adding it to the project.
 *
 * The layer is built on a worker thread, because instantiating a remote
 * provider issues a capabilities request which would otherwise freeze the
 * window for as long as the server takes to answer. Only the newest request
 * counts: results of superseded ones are discarded, so browsing the catalog
 * quickly never queues up previews.
 *
 * The preview layer never enters QgsProject, so it leaves neither a layer tree
 * entry nor a dirty project behind.
 */
class KADAS_GUI_EXPORT KadasCatalogPreview : public QObject
{
    Q_OBJECT
  public:
    /**
     * Constructor. Previews are drawn over \a canvas, and messages about them
     * (a layer outside the current view, a layer which cannot be previewed)
     * go to \a messageBar.
     */
    KadasCatalogPreview( QgsMapCanvas *canvas, QgsMessageBar *messageBar, QObject *parent = nullptr );
    ~KadasCatalogPreview() override;

  public slots:
    /**
     * Previews \a source, replacing whatever is currently previewed. An
     * invalid source clears the preview.
     */
    void setSource( const KadasCatalogLayerSource &source );

    //! Removes the preview from the canvas.
    void clear();

  protected:
    //! Keeps the badge in the canvas corner as the canvas is resized.
    bool eventFilter( QObject *watched, QEvent *event ) override;

  private slots:
    //! Re-renders the preview for the canvas' current extent.
    void refresh();

    /**
     * Takes over \a layer, built for \a generation, and starts previewing it.
     * Discards it if a newer request has superseded it.
     */
    void adoptLayer( int generation, KadasCatalogPreviewLayer layer );

  private:
    //! Puts \a image on the canvas and, the first time, ends the pending state.
    void showImage( const QImage &image, const QgsRectangle &extent );
    /**
     * Shows which entry is being previewed, and whether its first image is
     * still on its way. Hidden while nothing is previewed.
     */
    void updateBadge();
    void positionBadge();
    //! Stops the running render job, if any, without blocking on it.
    void cancelRenderJob();
    //! Warns if \a layer does not overlap what the canvas currently shows.
    void checkLayerInView( QgsMapLayer *layer );
    /**
     * Replaces the preview message with \a text, offering to zoom to
     * \a zoomTo when that extent is not empty.
     */
    void showMessage( const QString &text, Qgis::MessageLevel level, const QgsRectangle &zoomTo = QgsRectangle() );
    void removeMessage();

    QgsMapCanvas *mCanvas = nullptr;
    QPointer<QgsMessageBar> mMessageBar;
    KadasCatalogPreviewCanvasItem *mItem = nullptr;

    QThread *mWorkerThread = nullptr;
    KadasCatalogPreviewBuilder *mBuilder = nullptr;

    /**
     * Incremented on every request; a built layer is only adopted when it
     * still carries the current value.
     */
    int mGeneration = 0;

    KadasCatalogPreviewLayer mLayer;
    QgsMapRendererParallelJob *mRenderJob = nullptr;
    QPointer<QgsMessageBarItem> mMessageItem;

    //! Canvas corner label naming the previewed entry, see updateBadge().
    QPointer<QLabel> mBadge;
    //! Name of the entry being previewed; empty when there is none.
    QString mSourceName;
    //! TRUE between the request and the first image of the current preview.
    bool mPending = false;
};

Q_DECLARE_METATYPE( KadasCatalogPreviewLayer )

#endif // KADASCATALOGPREVIEW_H
