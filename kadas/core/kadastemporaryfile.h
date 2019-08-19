/***************************************************************************
    kadastemporaryfile.h
    --------------------
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

#ifndef KADASTEMPORARYFILE_H
#define KADASTEMPORARYFILE_H

#include <QObject>
#include <QList>

#include <kadas/core/kadas_core.h>

class QTemporaryFile;

class KADAS_CORE_EXPORT KadasTemporaryFile : public QObject
{
public:
  static QString createNewFile ( const QString& templateName );
  static void clear();

private:
  QList<QTemporaryFile*> mFiles;
  KadasTemporaryFile() {}
  ~KadasTemporaryFile();
  static KadasTemporaryFile* instance();
  QString _createNewFile ( const QString& templateName );
};

#endif // KADASTEMPORARYFILE_H
