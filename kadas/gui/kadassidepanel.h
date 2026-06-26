/***************************************************************************
    kadassidepanel.h
    ----------------
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

#ifndef KADASSIDEPANEL_H
#define KADASSIDEPANEL_H

#include <QFrame>
#include <QPointer>

#include <qgis/qgis_sip.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/kadassidepanelhost.h"

class QgsMapCanvas;
class QFormLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QVBoxLayout;

/**
 * A neutral-styled container that docks into a KadasSidePanelHost beside the
 * map canvas. It is a drop-in replacement for KadasBottomBar for map tool
 * bars: create one, populate it with a layout, then show() it. The canvas
 * reflows to make room (the panel does not overlay the canvas).
 *
 * The panel docks itself into the matching host (located on the map canvas
 * window) on construction and undocks on destruction, mirroring the
 * create/populate/show/delete lifecycle used by KadasBottomBar consumers.
 */
class KADAS_GUI_EXPORT KadasSidePanel : public QFrame
{
    Q_OBJECT
  public:
    KadasSidePanel( QgsMapCanvas *canvas, KadasSidePanelHost::Edge edge = KadasSidePanelHost::Edge::Right );
    ~KadasSidePanel() override;

    //! Adds the first row: a bold title and a close button (emits closeRequested()).
    void setTitle( const QString &title );

    //! Adds an undo/redo button row (emits undoRequested()/redoRequested()).
    void addUndoRedoRow();

    //! Adds a label + widget row, the default vertical layout style.
    void addRow( const QString &label, QWidget *widget SIP_TRANSFER );
    //! Adds a single label followed by several widgets sharing that row.
    void addRow( const QString &label, const QList<QWidget *> &widgets SIP_TRANSFER );
    //! Adds a full-width widget row (no label).
    void addRow( QWidget *widget SIP_TRANSFER );
    //! Adds a full-width layout row (e.g. a button row).
    void addRow( QLayout *layout SIP_TRANSFER );

    //! Creates a titled group box spanning the panel width and returns its form layout to populate.
    QFormLayout *addGroup( const QString &title );

  public slots:
    //! Enables/disables the undo button created by addUndoRedoRow().
    void setCanUndo( bool enabled );
    //! Enables/disables the redo button created by addUndoRedoRow().
    void setCanRedo( bool enabled );

  signals:
    void closeRequested();
    void undoRequested();
    void redoRequested();

  protected:
    QgsMapCanvas *mCanvas = nullptr;

  private:
    KadasSidePanelHost *findHost() const;
    QVBoxLayout *ensureOuterLayout();
    QFormLayout *ensureForm();

    KadasSidePanelHost::Edge mEdge;
    QPointer<KadasSidePanelHost> mHost;

    QVBoxLayout *mOuterLayout = nullptr;
    QHBoxLayout *mHeaderRow = nullptr;
    QLabel *mTitleLabel = nullptr;
    QHBoxLayout *mUndoRedoRow = nullptr;
    QPushButton *mUndoButton = nullptr;
    QPushButton *mRedoButton = nullptr;
    QFormLayout *mFormLayout = nullptr;
};

#endif // KADASSIDEPANEL_H
