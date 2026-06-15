/***************************************************************************
    kadasannotationlayer.h
    ----------------------
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

#ifndef KADASANNOTATIONLAYER_H
#define KADASANNOTATIONLAYER_H

#include <functional>

#include <QHash>
#include <QString>

#include <qgis/qgsannotationlayer.h>

class QgsProject;

/**
 * \brief Abstract base for Kadas-specific QgsAnnotationLayer subclasses
 *        (bullseye, guide grid, ...).
 *
 * Adds the \c regenerate() contract and a promotion mechanism that swaps
 * plain \c QgsAnnotationLayer instances back to their Kadas subclass on load.
 */
class KadasAnnotationLayer : public QgsAnnotationLayer
{
    Q_OBJECT
  public:
    using Factory = std::function<KadasAnnotationLayer *( const QString &name )>;

    //! Register a subclass factory under its \c kadas/annotation-type customProperty key.
    static void registerType( const QString &kadasType, Factory factory );

    //! Promote plain annotation layers in \a project to their registered Kadas subclass, in place.
    static void promoteAll( QgsProject *project );

    /// Rebuild the layer's annotation items from the current subclass-owned configuration.
    virtual void regenerate() = 0;

  protected:
    //! \a kadasType sets the kadas/annotation-type marker immediately so parametric layers are detectable gui-side.
    KadasAnnotationLayer( const QString &name, const QString &kadasType );

  private:
    static QHash<QString, Factory> &registry();
};

#endif // KADASANNOTATIONLAYER_H
