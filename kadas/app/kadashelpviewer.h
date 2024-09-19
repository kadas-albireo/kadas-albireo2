/***************************************************************************
    kadashelpviewer.h
    ----------------
    copyright            : (C) 2024 by Damiano Lombardi
    email                : damiano at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASHELPVIEWER_H
#define KADASHELPVIEWER_H

#include <QObject>

#include "kadas/core/kadasfileserver.h"

class KadasHelpViewer : public QObject
{
    Q_OBJECT

  public:
    KadasHelpViewer( QObject *parent = nullptr );

  public slots:
    void showHelp() const;

  private:

    KadasFileServer mHelpFileServer;

};

#endif // KADASHELPVIEWER_H
