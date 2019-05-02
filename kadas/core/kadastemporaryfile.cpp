/***************************************************************************
    kadastemporaryfile.cpp
    ----------------------
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

#include <QDir>
#include <QTemporaryFile>

#include <kadas/core/kadastemporaryfile.h>


KadasTemporaryFile::~KadasTemporaryFile()
{
  clear();
}

void KadasTemporaryFile::clear()
{
  for ( QTemporaryFile* file : instance()->mFiles )
  {
    QFile( file->fileName() + ".aux.xml" ).remove();
    delete file;
  }
  instance()->mFiles.clear();
}

QString KadasTemporaryFile::createNewFile( const QString& templateName )
{
  return instance()->_createNewFile( templateName );
}

KadasTemporaryFile* KadasTemporaryFile::instance()
{
  static KadasTemporaryFile i;
  return &i;
}

QString KadasTemporaryFile::_createNewFile( const QString& templateName )
{
  QTemporaryFile* tmpFile = new QTemporaryFile( QDir::temp().absoluteFilePath( "XXXXXX_" + templateName ), this );
  mFiles.append( tmpFile );
  return tmpFile->open() ? tmpFile->fileName() : QString();
}
