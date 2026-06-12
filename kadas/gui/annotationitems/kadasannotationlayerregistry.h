/***************************************************************************
    kadasannotationlayerregistry.h
    ------------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASANNOTATIONLAYERREGISTRY_H
#define KADASANNOTATIONLAYERREGISTRY_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

#include "qgis/qgis_sip.h"

#include "kadas/gui/kadas_gui.h"

class QDomDocument;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Process-wide registry of the well-known singleton Kadas
 *        \c QgsAnnotationLayer instances inside the current \c QgsProject.
 *
 * Each \c StandardLayer enum value names a *role* (Redlining, Symbols,
 * Pictures, Pins, Routes, MSS) that the application auto-creates the
 * first time a tool needs a destination layer for that role.  There is
 * at most one layer per role per project; subsequent lookups return the
 * existing instance.  The mapping from role to project layer id is
 * persisted under a dedicated \c StandardAnnotationLayers element in the
 * project XML.
 *
 * Configurable / parametric annotation layers (bullseye, guidegrid)
 * are deliberately out of scope: each instance carries its own
 * configuration and a project can hold many of them, so they cannot be
 * keyed by a fixed role.  Those are handled by \c KadasAnnotationLayer
 * + its promotion registry instead.
 */
class KADAS_GUI_EXPORT KadasAnnotationLayerRegistry : public QObject
{
    Q_OBJECT
  public:
    enum class StandardLayer
    {
      RedliningLayer,
      SymbolsLayer,
      PicturesLayer,
      PinsLayer,
      RoutesLayer,
      MssLayer
    };

    //! Returns the well-known annotation layer for \a layer, creating and
    //! adding it to the current project on first use.
    static QgsAnnotationLayer *getOrCreateAnnotationLayer( StandardLayer layer );

    //! Returns the human-readable layer names keyed by \c StandardLayer.
    static const QMap<StandardLayer, QString> &standardLayerNames() SIP_SKIP;

    //! Connects to project signals; must be called once at app startup.
    static void init();

  protected:
    KadasAnnotationLayerRegistry();

  private:
    static KadasAnnotationLayerRegistry *instance();
    QMap<StandardLayer, QString> mLayerIdMap;

  private slots:
    void clear();
    void readFromProject( const QDomDocument &doc );
    void writeToProject( QDomDocument &doc );
    //! Slot bound to \c QgsProject::crsChanged. For each tracked standard
    //! annotation layer that is still empty (no items), retargets its CRS
    //! to the new project CRS so freshly drawn axis-aligned items
    //! (rectangles, circles) stay un-skewed. MSS is excluded
    //! (libmss IPC requires WGS84) and non-empty layers are left
    //! untouched (mutating their CRS would re-interpret existing
    //! coordinates and shift / skew already-drawn items).
    void onProjectCrsChanged();
};

#endif // KADASANNOTATIONLAYERREGISTRY_H
