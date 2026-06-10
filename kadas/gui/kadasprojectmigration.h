/***************************************************************************
    kadasprojectmigration.h
    -----------------------
    copyright            : (C) 2019 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASPROJECTMIGRATION_H
#define KADASPROJECTMIGRATION_H

#include "kadas/gui/kadas_gui.h"
#include <QString>

class QDomDocument;
class QDomElement;

class KADAS_GUI_EXPORT KadasProjectMigration
{
  public:
    static QString migrateProject( const QString &fileName, QStringList &filesToAttach );
    static bool migrateProjectXml( const QString &basedir, QDomDocument &doc, QStringList &filesToAttach );

  private:
    //! Rewrites legacy `<maplayer name="KadasMilxLayer" type="plugin">` blocks
    //! and their `<MapItem name="KadasMilxItem">` children into a
    //! `QgsAnnotationLayer` populated with `KadasMilxAnnotationItem` entries.
    //! Returns true if at least one such layer was rewritten.
    static bool migrateLegacyMilxLayers( QDomDocument &doc, QDomElement &root );
    //! Rewrites legacy `<maplayer name="bullseye"|"guide_grid" type="plugin">`
    //! blocks into plain `QgsAnnotationLayer` blocks carrying the
    //! `kadas/annotation-type` customProperty marker plus the per-type
    //! config keys; the post-load promotion pass
    //! (KadasAnnotationLayer::promoteAll) then swaps in the matching
    //! KadasBullseyeLayer / KadasGuideGridLayer subclass instance.
    //! Returns true if at least one such layer was rewritten.
    static bool migrateLegacyPluginGridLayers( QDomDocument &doc, QDomElement &root );
    //! Rewrites legacy `<maplayer name="KadasItemLayer" type="plugin">`
    //! blocks into `QgsAnnotationLayer` blocks by translating each
    //! `<MapItem name="Kadas...Item">` child into the corresponding
    //! `QgsAnnotationItem` subclass. Only the v2 (XML-attribute) MapItem
    //! payload is handled here; v1 (JSON-in-CDATA) MapItems and any layer
    //! containing item types not yet covered by a translator are left
    //! untouched; such layers will fail to load.
    //! Returns true if at least one such layer was rewritten.
    static bool migrateLegacyKadasItemLayers( QDomDocument &doc, QDomElement &root );
    static QDomElement replaceAnnotationLayer( QDomDocument &doc, QDomElement &root, const QString &layerId );
    static QMap<QString, QString> deserializeLegacyRedliningFlags( const QString &flagsStr );
    static bool shouldAttach( const QString &baseDir, const QString &filePath );
};

#endif // KADASPROJECTMIGRATION_H
