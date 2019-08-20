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
#include <QList>
#include <QObject>
#include <QTemporaryFile>

#include <kadas/core/kadastemporaryfile.h>

class QTemporaryFile;

class KadasTemporaryFileImpl : public QObject
{
public:
  static KadasTemporaryFileImpl* instance();
  QString createNewFile ( const QString& templateName );
  void clear();

private:
  ~KadasTemporaryFileImpl();
  QList<QTemporaryFile*> mFiles;
};

KadasTemporaryFileImpl::~KadasTemporaryFileImpl()
{
  clear();
}

KadasTemporaryFileImpl* KadasTemporaryFileImpl::instance()
{
  static KadasTemporaryFileImpl i;
  return &i;
}

void KadasTemporaryFileImpl::clear()
{
  for ( QTemporaryFile* file : instance()->mFiles ) {
    QFile ( file->fileName() + ".aux.xml" ).remove();
    delete file;
  }
  instance()->mFiles.clear();
}

QString KadasTemporaryFileImpl::createNewFile ( const QString& templateName )
{
  QTemporaryFile* tmpFile = new QTemporaryFile ( QDir::temp().absoluteFilePath ( "XXXXXX_" + templateName ), this );
  mFiles.append ( tmpFile );
  return tmpFile->open() ? tmpFile->fileName() : QString();
}

void KadasTemporaryFile::clear()
{
  KadasTemporaryFileImpl::instance()->clear();
}

QString KadasTemporaryFile::createNewFile ( const QString& templateName )
{
  return KadasTemporaryFileImpl::instance()->createNewFile ( templateName );
}
