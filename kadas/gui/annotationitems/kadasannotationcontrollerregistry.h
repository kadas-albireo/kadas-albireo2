/***************************************************************************
    kadasannotationcontrollerregistry.h
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

#ifndef KADASANNOTATIONCONTROLLERREGISTRY_H
#define KADASANNOTATIONCONTROLLERREGISTRY_H

#include <QHash>
#include <QString>

#include "kadas/gui/kadas_gui.h"

class KadasAnnotationItemController;

/**
 * \ingroup gui
 * \brief Process-wide registry mapping QgsAnnotationItem type ids to their Kadas controller.
 */
class KADAS_GUI_EXPORT KadasAnnotationControllerRegistry
{
  public:
    //! Returns the process-wide registry instance.
    static KadasAnnotationControllerRegistry *instance();

    ~KadasAnnotationControllerRegistry();

    //! Registers \a controller under its itemType(); takes ownership and replaces any existing.
    void addController( KadasAnnotationItemController *controller );

    //! Returns the controller for type id \a typeId, or nullptr.
    KadasAnnotationItemController *controllerFor( const QString &typeId ) const;

    //! Registered type ids, in registration order.
    QStringList registeredTypes() const;

  private:
    KadasAnnotationControllerRegistry() = default;
    KadasAnnotationControllerRegistry( const KadasAnnotationControllerRegistry & ) = delete;
    KadasAnnotationControllerRegistry &operator=( const KadasAnnotationControllerRegistry & ) = delete;
#ifdef SIP_RUN
    KadasAnnotationControllerRegistry();
    KadasAnnotationControllerRegistry( const KadasAnnotationControllerRegistry & );
#endif

    QHash<QString, KadasAnnotationItemController *> mControllers;
    QStringList mOrder;
};

#endif // KADASANNOTATIONCONTROLLERREGISTRY_H
