/***************************************************************************
    kadasitemlayermigration.h
    -------------------------
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

#ifndef KADASITEMLAYERMIGRATION_H
#define KADASITEMLAYERMIGRATION_H

#include "kadas/gui/kadas_gui.h"

class QgsProject;

/**
 * \ingroup gui
 * \brief Post-load migration from legacy \c KadasItemLayer (a plugin layer
 *        holding \c KadasMapItem subclasses) to the current
 *        \c QgsAnnotationLayer + \c QgsAnnotationItem stack.
 *
 * The XML-level \c KadasProjectMigration handles only the Kadas 1.x → 2.x
 * upgrade and still emits \c KadasItemLayer plugin layers. This class runs
 * after \c QgsProject::read() and walks every loaded \c KadasItemLayer:
 * for each layer where every item exposes an \c annotationItem() override,
 * the items are translated into a fresh \c QgsAnnotationLayer (same name,
 * CRS, and visibility), and the legacy layer is removed from the project.
 *
 * Layers whose items cannot all be translated are left untouched and a
 * warning is emitted — they will continue to use the legacy renderer until
 * the missing item types acquire an \c annotationItem() override.
 *
 * The project is marked dirty after a successful migration so the user is
 * prompted to save in the new format.
 */
class KADAS_GUI_EXPORT KadasItemLayerMigration
{
  public:
    /**
     * Walks \a project and migrates every fully-translatable
     * \c KadasItemLayer to a \c QgsAnnotationLayer. Returns the number of
     * layers that were migrated.
     */
    static int migrateProject( QgsProject *project );

  private:
    KadasItemLayerMigration() = delete;
};

#endif // KADASITEMLAYERMIGRATION_H
