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
    static void migrateKadas1xTo2x( QDomDocument &doc, QDomElement &root, const QString &basedir, QStringList &filesToAttach );
    static QDomElement replaceAnnotationLayer( QDomDocument &doc, QDomElement &root, const QString &layerId );
    static QMap<QString, QString> deserializeLegacyRedliningFlags( const QString &flagsStr );
    static bool shouldAttach( const QString &baseDir, const QString &filePath );
};

#endif // KADASPROJECTMIGRATION_H
