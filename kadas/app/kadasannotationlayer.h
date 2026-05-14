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
 * Two responsibilities on top of \c QgsAnnotationLayer:
 *
 * - Defines the \c regenerate() contract: rebuild the layer's annotation
 *   items from the subclass-owned configuration. Called by the subclass
 *   itself after every config change and after readXml().
 *
 * - Provides a project-wide promotion mechanism. QGIS serializes every
 *   annotation layer as plain \c type="annotation"; on load (or after the
 *   project was round-tripped through vanilla QGIS), the plain instances
 *   need to be swapped for the matching Kadas subclass so that custom
 *   renderers and edit hooks come back. Subclasses register themselves
 *   with \ref registerType() under a \c kadas/annotation-type key, and
 *   \ref promoteAll() walks the project, performs an XML round-trip into
 *   a fresh subclass instance, and swaps it into the layer tree at the
 *   same position.
 */
class KadasAnnotationLayer : public QgsAnnotationLayer
{
    Q_OBJECT
  public:
    using Factory = std::function<KadasAnnotationLayer *( const QString &name )>;

    /**
     * Register a subclass factory under its \c kadas/annotation-type
     * customProperty key. The factory is invoked with the plain layer's
     * name; the returned instance is then filled via \c readLayerXml().
     */
    static void registerType( const QString &kadasType, Factory factory );

    /**
     * Walk \a project, find plain \c QgsAnnotationLayer instances whose
     * \c kadas/annotation-type customProperty matches a registered type,
     * promote each to the matching Kadas subclass via XML round-trip,
     * and swap them into the layer tree at the same position. Subclass
     * instances already present in the project are skipped.
     */
    static void promoteAll( QgsProject *project );

    /// Rebuild the layer's annotation items from the current subclass-owned configuration.
    virtual void regenerate() = 0;

  protected:
    explicit KadasAnnotationLayer( const QString &name );

  private:
    static QHash<QString, Factory> &registry();
};

#endif // KADASANNOTATIONLAYER_H
