/***************************************************************************
    kadascircularsectoritem.cpp
    ---------------------------
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

#include <qmath.h>

#include <qgis/qgsgeometry.h>
#include <qgis/qgscircularstring.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmultisurface.h>
#include <qgis/qgspoint.h>
#include <qgis/qgspolygon.h>

#include <kadas/core/mapitems/kadascircularsectoritem.h>


KadasCircularSectorItem::KadasCircularSectorItem(const QgsCoordinateReferenceSystem &crs, QObject* parent)
  : KadasGeometryItem(crs, parent)
{
  clear();
}

bool KadasCircularSectorItem::startPart(const QgsPointXY& firstPoint)
{
  state()->drawStatus = State::Drawing;
  state()->sectorStatus = State::HaveCenter;
  state()->centers.append(firstPoint);
  state()->radii.append(0);
  state()->startAngles.append(0);
  state()->stopAngles.append(0);
  recomputeDerived();
  return true;
}

bool KadasCircularSectorItem::startPart(const QList<double>& attributeValues)
{
  state()->drawStatus = State::Drawing;
  // todo
  return false;
}

void KadasCircularSectorItem::setCurrentPoint(const QgsPointXY& p, const QgsMapSettings &mapSettings)
{
  if ( state()->sectorStatus == State::HaveCenter )
  {
    state()->radii.back() = qSqrt( p.sqrDist( state()->centers.back() ) );
    state()->startAngles.back() = state()->stopAngles.back() = qAtan2( p.y() - state()->centers.back().y(), p.x() - state()->centers.back().x() );
  }
  else if ( state()->sectorStatus == State::HaveRadius )
  {
    state()->stopAngles.back() = qAtan2( p.y() - state()->centers.back().y(), p.x() - state()->centers.back().x() );
    if ( state()->stopAngles.back() <= state()->startAngles.back() )
    {
      state()->stopAngles.back() += 2 * M_PI;
    }

    // Snap to full circle if within 5px
    QgsCoordinateTransform crst(mCrs, mapSettings.destinationCrs(), mapSettings.transformContext());
    const QgsPointXY& center = state()->centers.back();
    const double& radius = state()->radii.back();
    const double& startAngle = state()->startAngles.back();
    const double& stopAngle = state()->stopAngles.back();
    QgsPointXY pStart( center.x() + radius * qCos( startAngle ),
                       center.y() + radius * qSin( startAngle ) );
    QgsPointXY pEnd( center.x() + radius * qCos( stopAngle ),
                     center.y() + radius * qSin( stopAngle ) );
    pStart = mapSettings.mapToPixel().transform(crst.transform(pStart));
    pEnd = mapSettings.mapToPixel().transform(crst.transform(pEnd));
    if ( pStart.sqrDist(pEnd) < 25 )
    {
      state()->stopAngles.back() = state()->startAngles.back() + 2 * M_PI;
    }
  }
  recomputeDerived();
}

void KadasCircularSectorItem::setCurrentAttributes(const QList<double>& values)
{
  // todo
}

bool KadasCircularSectorItem::continuePart()
{
  if(state()->sectorStatus == State::HaveCenter) {
    state()->sectorStatus = State::HaveRadius;
    return true;
  }
  return false;
}

void KadasCircularSectorItem::endPart()
{
  state()->drawStatus = State::Finished;
}

const QgsMultiSurface* KadasCircularSectorItem::geometry() const
{
  return static_cast<QgsMultiSurface*>(mGeometry);
}

QgsMultiSurface* KadasCircularSectorItem::geometry()
{
  return static_cast<QgsMultiSurface*>(mGeometry);
}

void KadasCircularSectorItem::measureGeometry()
{
  // Not implemented
}

void KadasCircularSectorItem::recomputeDerived()
{
  QgsMultiSurface* multiGeom = new QgsMultiSurface();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i )
  {
    const QgsPointXY& center = state()->centers[i];
    const double& radius = state()->radii[i];
    const double& startAngle = state()->startAngles[i];
    const double& stopAngle = state()->stopAngles[i];
    QgsPoint pStart, pMid, pEnd;
    if ( stopAngle == startAngle + 2 * M_PI )
    {
      pStart = pEnd = QgsPoint( center.x() + radius * qCos( stopAngle ),
                                center.y() + radius * qSin( stopAngle ) );
      pMid = QgsPoint( center );
    }
    else
    {
      double alphaMid = 0.5 * ( startAngle + stopAngle - 2 * M_PI );
      pStart = QgsPoint( center.x() + radius * qCos( startAngle ),
                         center.y() + radius * qSin( startAngle ) );
      pMid = QgsPoint( center.x() + radius * qCos( alphaMid ),
                       center.y() + radius * qSin( alphaMid ) );
      pEnd = QgsPoint( center.x() + radius * qCos( stopAngle - 2 * M_PI ),
                       center.y() + radius * qSin( stopAngle - 2 * M_PI ) );
    }
    QgsCompoundCurve* exterior = new QgsCompoundCurve();
    if ( startAngle != stopAngle )
    {
      QgsCircularString* arc = new QgsCircularString();
      arc->setPoints( QgsPointSequence() << pStart << pMid << pEnd );
      exterior->addCurve( arc );
    }
    if ( startAngle != stopAngle + 2 * M_PI )
    {
      QgsLineString* line = new QgsLineString();
      line->setPoints( QgsPointSequence() << pStart << pMid << pEnd );
      exterior->addCurve( line );
    }
    QgsPolygon* poly = new QgsPolygon;
    poly->setExteriorRing( exterior );
    multiGeom->addGeometry( poly );
  }
  setGeometry(multiGeom);
}

QList<KadasMapItem::NumericAttribute> KadasCircularSectorItem::attributes() const
{
  double dMin = std::numeric_limits<double>::min();
  double dMax = std::numeric_limits<double>::max();
  QList<KadasMapItem::NumericAttribute> attributes;
  attributes.insert(AttrX, NumericAttribute{"x", dMin, dMax, 0});
  attributes.insert(AttrY, NumericAttribute{"y", dMin, dMax, 0});
  attributes.insert(AttrR, NumericAttribute{"r", 0, dMax, 0});
  attributes.insert(AttrA1, NumericAttribute{QString( QChar( 0x03B1 ) ) + "1", 0, dMax, 0});
  attributes.insert(AttrA2, NumericAttribute{QString( QChar( 0x03B1 ) ) + "2", 0, dMax, 0});
  return attributes;
}

QList<double> KadasCircularSectorItem::attributesFromPosition(const QgsPointXY& pos) const
{
  QList<double> attributes;
  if(state()->drawStatus == State::Empty) {
    attributes.insert(AttrX, pos.x());
    attributes.insert(AttrY, pos.y());
    attributes.insert(AttrR, state()->radii.last());
    attributes.insert(AttrA1, state()->stopAngles.last());
    attributes.insert(AttrA2, state()->stopAngles.last());
  }
  else if ( state()->sectorStatus == State::HaveCenter)
  {
    attributes.insert(AttrX, state()->centers.last().x());
    attributes.insert(AttrY, state()->centers.last().y());
    attributes.insert(AttrR, qSqrt(state()->centers.last().sqrDist(pos)));
    attributes.insert(AttrA1, state()->stopAngles.last());
    attributes.insert(AttrA2, state()->stopAngles.last());
  }
  else if ( state()->sectorStatus == State::HaveRadius )
  {
    attributes.insert(AttrX, state()->centers.last().x());
    attributes.insert(AttrY, state()->centers.last().y());
    attributes.insert(AttrR, state()->radii.last());
    double startAngle = 2.5 * M_PI - state()->startAngles.last();
    if ( startAngle > 2 * M_PI )
      startAngle -= 2 * M_PI;
    else if ( startAngle < 0 )
      startAngle += 2 * M_PI;
    attributes.insert(AttrA1, startAngle / M_PI * 180.);
    double stopAngle = 2.5 * M_PI - state()->stopAngles.last();
    if ( stopAngle > 2 * M_PI )
      stopAngle -= 2 * M_PI;
    else if ( stopAngle < 0 )
      stopAngle += 2 * M_PI;
    attributes.insert(AttrA2, stopAngle / M_PI * 180.);
  }
  return attributes;
}

QgsPointXY KadasCircularSectorItem::positionFromAttributes(const QList<double>& values) const
{
  return QgsPointXY(values[AttrX], values[AttrY]);
}

KadasMapItem::EditContext KadasCircularSectorItem::getEditContext(const QgsPointXY& pos, const QgsMapSettings& mapSettings) const
{
  return EditContext();
}

void KadasCircularSectorItem::edit(const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings& mapSettings)
{

}
