/***************************************************************************
    kadassidepanelhost.h
    --------------------
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

#ifndef KADASSIDEPANELHOST_H
#define KADASSIDEPANELHOST_H

#include <QWidget>

#include "kadas/gui/kadas_gui.h"

class QEvent;
class QTimer;
class QVBoxLayout;
class QgsMapCanvas;

/**
 * Reflow container for side panels docked beside the map canvas.
 *
 * A host lives in the central horizontal layout, next to the map canvas. When
 * a panel is added, the host becomes visible and pushes the canvas aside
 * (reflow) instead of overlaying it. When the last panel is removed, the host
 * collapses to zero width so the canvas occupies the full width again.
 *
 * The host is edge-aware so the same infrastructure can be reused for a left
 * panel (e.g. layer tree / catalog) and a right panel (e.g. map tool bars).
 */
class KADAS_GUI_EXPORT KadasSidePanelHost : public QWidget
{
    Q_OBJECT
  public:
    enum class Edge
    {
      Left,
      Right
    };

    //! Returns the canonical object name used to locate the host for a given edge.
    static QString objectNameForEdge( Edge edge );

    explicit KadasSidePanelHost( Edge edge, QWidget *parent = nullptr );

    Edge edge() const { return mEdge; }

    /**
     * Sets the map canvas the host reflows. When set, showing or hiding a
     * panel keeps the canvas content visually anchored: instead of the whole
     * map shifting, only the strip now covered by the panel is cropped.
     */
    void setMapCanvas( QgsMapCanvas *canvas );

    //! Docks a panel into the host and shows the host.
    void addPanel( QWidget *panel );
    //! Removes a panel from the host, collapsing the host if it becomes empty.
    void removePanel( QWidget *panel );

  protected:
    //! Watches the canvas for the reflow-driven resizes that follow a toggle.
    bool eventFilter( QObject *watched, QEvent *event ) override;

  private:
    //! Captured canvas state used to keep the map anchored across a reflow.
    struct CanvasAnchor
    {
        bool valid = false;
        double mapUnitsPerPixel = 0.0;
        double top = 0.0;
        double bottom = 0.0;
        double anchorX = 0.0;
        bool anchorLeft = true;
    };

    void updateVisibility();
    //! Records the canvas edge/scale to preserve before a reflow.
    CanvasAnchor captureCanvasAnchor() const;
    //! Freezes the canvas and arms re-anchoring for the reflows a toggle triggers.
    void armCanvasAnchor( const CanvasAnchor &anchor );
    //! Restores the armed scale/edge at the canvas' current width.
    void applyArmedAnchor();
    //! Thaws the canvas and disarms once the reflow has settled.
    void finishCanvasAnchor();

    Edge mEdge;
    QVBoxLayout *mLayout = nullptr;
    QgsMapCanvas *mCanvas = nullptr;
    //! Scale/edge to preserve while the canvas reflows; invalid when disarmed.
    CanvasAnchor mArmedAnchor;
    //! Fires once the reflow-driven resizes stop arriving, to thaw and disarm.
    QTimer *mSettleTimer = nullptr;
    //! Re-entrancy guard so re-anchoring does not recurse through resizes.
    bool mApplying = false;
};

#endif // KADASSIDEPANELHOST_H
