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

#include "kadas/gui/kadas_gui.h"

class QDomDocument;
class QgsAnnotationLayer;

/**
 * \ingroup gui
 * \brief Process-wide registry of the well-known Kadas \c QgsAnnotationLayer
 *        instances (Redlining, Symbols, Pictures, Pins, Routes) and their
 *        ids inside the current \c QgsProject.
 *
 * Mirrors the legacy \c KadasItemLayerRegistry but produces vanilla
 * \c QgsAnnotationLayer instances.  The mapping of \c StandardLayer to
 * project layer id is persisted in the project XML under the dedicated
 * \c StandardAnnotationLayers element, so it does not collide with the
 * legacy \c StandardItemLayers element while both registries coexist
 * during the migration.
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
    static const QMap<StandardLayer, QString> &standardLayerNames();

    //! Returns all \c QgsAnnotationLayer instances in the current project.
    static QList<QgsAnnotationLayer *> getAnnotationLayers();

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
};

#endif // KADASANNOTATIONLAYERREGISTRY_H
