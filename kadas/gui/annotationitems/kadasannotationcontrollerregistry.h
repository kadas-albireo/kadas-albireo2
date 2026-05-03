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
 * \brief Process-wide registry mapping QgsAnnotationItem type ids to their
 *        Kadas controller.
 *
 * Map tools, editors and KML export look up the controller for a given
 * \c QgsAnnotationItem via \c controllerFor(item->type()) and delegate
 * interactive behavior (draw / edit / snap / tooltip / KML) to it.
 *
 * Registration happens at application startup, typically from
 * \c KadasApplication::init().
 *
 * The registry owns the registered controllers; they are deleted when
 * the registry is destroyed (or when overwritten).
 */
class KADAS_GUI_EXPORT KadasAnnotationControllerRegistry
{
  public:
    //! Returns the process-wide registry instance.
    static KadasAnnotationControllerRegistry *instance();

    ~KadasAnnotationControllerRegistry();

    //! Registers \a controller for the type id it reports via
    //! \c KadasAnnotationItemController::itemType(). Takes ownership of \a controller.
    //! Replacing an existing registration deletes the previous one.
    void addController( KadasAnnotationItemController *controller );

    //! Returns the controller registered for QgsAnnotationItem type id \a typeId,
    //! or nullptr if none.
    KadasAnnotationItemController *controllerFor( const QString &typeId ) const;

    //! Returns the registered annotation item type ids, in registration order.
    QStringList registeredTypes() const;

  private:
    KadasAnnotationControllerRegistry() = default;
    KadasAnnotationControllerRegistry( const KadasAnnotationControllerRegistry & ) = delete;
    KadasAnnotationControllerRegistry &operator=( const KadasAnnotationControllerRegistry & ) = delete;

    QHash<QString, KadasAnnotationItemController *> mControllers;
    QStringList mOrder;
};

#endif // KADASANNOTATIONCONTROLLERREGISTRY_H
