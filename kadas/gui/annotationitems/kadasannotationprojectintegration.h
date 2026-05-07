/***************************************************************************
    kadasannotationprojectintegration.h
    -----------------------------------
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

#ifndef KADASANNOTATIONPROJECTINTEGRATION_H
#define KADASANNOTATIONPROJECTINTEGRATION_H

#include <QHash>
#include <QObject>
#include <QString>

#include "kadas/gui/kadas_gui.h"

class QDomDocument;
class QgsAnnotationPictureItem;

/**
 * \ingroup gui
 * \brief Wires the save-time shadow mechanism into the active QgsProject.
 *
 * On construction this object connects to \c QgsProject::readProject so any
 * Kadas-shadow items left over in a project file (e.g. one saved by an
 * older Kadas session that wasn't cleaned up) are stripped after load,
 * leaving the in-memory layer pristine.
 *
 * The save half of the lifecycle is driven explicitly by callers:
 * \c KadasApplication::projectSave() is expected to call
 * \c prepareForSave() immediately before \c QgsProject::write() and
 * \c stripAfterSave() immediately after, so the on-disk project contains
 * the QGIS-compat shadows but the running session does not.
 *
 * Single instance owned by \c KadasApplication.
 */
class KADAS_GUI_EXPORT KadasAnnotationProjectIntegration : public QObject
{
    Q_OBJECT
  public:
    explicit KadasAnnotationProjectIntegration( QObject *parent = nullptr );

    //! Iterates all \c QgsAnnotationLayer instances in the active project and
    //! calls \c KadasAnnotationLayerHelpers::prepareLayerForSave on each.
    void prepareForSave();

    //! Iterates all \c QgsAnnotationLayer instances in the active project and
    //! calls \c KadasAnnotationLayerHelpers::stripShadowsFromLayer on each.
    void stripAfterSave();

  private slots:
    void onProjectRead( const QDomDocument &doc );

  private:
    // Live attachment paths captured by prepareForSave() so they can be
    // restored by stripAfterSave() after QgsProject::write() has run with
    // the "attachment:..." identifiers in place.
    QHash<QgsAnnotationPictureItem *, QString> mPicturePathsBeforeSave;
};

#endif // KADASANNOTATIONPROJECTINTEGRATION_H
