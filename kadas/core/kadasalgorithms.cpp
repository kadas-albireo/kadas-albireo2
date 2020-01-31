/***************************************************************************
    kadasalgorithms.cpp
    -------------------
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

#include <algorithm>
#include <list>
#include <vector>

#include <kadas/core/kadasalgorithms.h>


struct Cluster1D
{
  int min, max;
  std::vector<const KadasAlgorithms::Rect *> rects;
};

struct Cluster2D
{
  int x1, y1, x2, y2;
  std::vector<const KadasAlgorithms::Rect *> rects;
};

static int linescan( const std::vector<const KadasAlgorithms::Rect *> &rects, int axis, std::list<Cluster1D> &clusters )
{
  if ( axis == 0 )
  {
    for ( const KadasAlgorithms::Rect *rect : rects )
    {
      clusters.push_back( {rect->x1, rect->x2, {rect}} );
    }
  }
  else
  {
    for ( const KadasAlgorithms::Rect *rect : rects )
    {
      clusters.push_back( {rect->y1, rect->y2, {rect}} );
    }
  }

  clusters.sort( []( const Cluster1D & c1, const Cluster1D & c2 ) { return c1.min < c2.min; } );

  // Merge neighboring clusters
  auto it = clusters.begin();
  int count = it != clusters.end() ? 1 : 0;
  ++it;
  while ( it != clusters.end() )
  {
    auto prev = it; --prev;
    Cluster1D &c1 = *prev;
    Cluster1D &c2 = *it;
    if ( c2.min < c1.max )
    {
      c1.max = std::max( c1.max, c2.max );
      c1.rects.insert( c1.rects.end(), c2.rects.begin(), c2.rects.end() );
      it = clusters.erase( it );
    }
    else
    {
      ++it;
      ++count;
    }
  }
  return count;
}

static void clusters( const std::vector<const KadasAlgorithms::Rect *> &rects, std::vector<Cluster2D> &output )
{
  // Abort condition: y linescan returns a single cluster
  std::list<Cluster1D> clustersX;
  linescan( rects, 0, clustersX );
  for ( const Cluster1D &cx : clustersX )
  {
    std::list<Cluster1D> clustersY;
    int count = linescan( cx.rects, 1, clustersY );
    if ( count == 1 )
    {
      const Cluster1D &cy = clustersY.front();
      output.push_back( {cx.min, cy.min, cx.max, cy.max, cy.rects} );
    }
    else
    {
      for ( const Cluster1D &cy : clustersY )
      {
        clusters( cy.rects, output );
      }
    }
  }
}


QList<KadasAlgorithms::Cluster> KadasAlgorithms::overlappingRects( const QList<Rect> &rects )
{
  std::vector<const Rect *> rectp;
  for ( const Rect &rect : rects )
  {
    rectp.push_back( &rect );
  }
  std::vector<Cluster2D> output;
  clusters( rectp, output );

  QList<Cluster> result;
  for ( const Cluster2D &c : output )
  {
    result.append( Cluster() );
    result.back().x1 = c.x1;
    result.back().y1 = c.y1;
    result.back().x2 = c.x2;
    result.back().y2 = c.y2;
    for ( const Rect *rect : c.rects )
    {
      result.back().rects.append( *rect );
    }
  }
  return result;
}
