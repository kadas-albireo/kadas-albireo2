/***************************************************************************
    kadasalgorithms.h
    -----------------
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

#ifndef KADASALGORITHMS_H
#define KADASALGORITHMS_H

#ifndef SIP_RUN

#include <QList>

#include <kadas/core/kadas_core.h>

class KADAS_CORE_EXPORT KadasAlgorithms
{

  public:
    struct Rect
    {
      int x1, y1, x2, y2;
      void *data;
    };
    struct Cluster
    {
      int x1, y1, x2, y2;
      QList<Rect> rects;
    };

    static QList<Cluster> overlappingRects( const QList<Rect> &rects );

};

#endif // SIP_RUN

#endif // KADASALGORITHMS_H
