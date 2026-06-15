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
 */
class KADAS_GUI_EXPORT KadasAnnotationProjectIntegration : public QObject
{
    Q_OBJECT
  public:
    explicit KadasAnnotationProjectIntegration( QObject *parent = nullptr );

    //! Calls prepareLayerForSave() on every annotation layer in the project.
    void prepareForSave();

    //! Calls stripShadowsFromLayer() on every annotation layer in the project.
    void stripAfterSave();

  private slots:
    void onProjectRead( const QDomDocument &doc );

  private:
    // Live paths captured by prepareForSave(), restored by stripAfterSave().
    QHash<QgsAnnotationPictureItem *, QString> mPicturePathsBeforeSave;
};

#endif // KADASANNOTATIONPROJECTINTEGRATION_H
