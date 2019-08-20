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

#include <kadas/gui/mapitems/kadascircularsectoritem.h>


KadasCircularSectorItem::KadasCircularSectorItem ( const QgsCoordinateReferenceSystem& crs, QObject* parent )
  : KadasGeometryItem ( crs, parent )
{
  clear();
}

bool KadasCircularSectorItem::startPart ( const QgsPointXY& firstPoint )
{
  state()->drawStatus = State::Drawing;
  state()->sectorStatus = State::HaveCenter;
  state()->centers.append ( firstPoint );
  state()->radii.append ( 0 );
  state()->startAngles.append ( 0 );
  state()->stopAngles.append ( 2 * M_PI );
  recomputeDerived();
  return true;
}

bool KadasCircularSectorItem::startPart ( const AttribValues& values )
{
  state()->drawStatus = State::Drawing;
  state()->sectorStatus = values[AttrR] > 0 ? State::HaveRadius : State::HaveCenter;
  state()->centers.append ( QgsPointXY ( values[AttrX], values[AttrY] ) );
  state()->radii.append ( values[AttrR] );
  state()->startAngles.append ( values[AttrA1] / 180 * M_PI );
  state()->stopAngles.append ( values[AttrA2] / 180. * M_PI );
  if ( state()->stopAngles.last() <= state()->startAngles.last() ) {
    state()->stopAngles.last() += 2 * M_PI;
  }
  recomputeDerived();
  return true;
}

void KadasCircularSectorItem::setCurrentPoint ( const QgsPointXY& p, const QgsMapSettings* mapSettings )
{
  if ( state()->sectorStatus == State::HaveCenter ) {
    state()->radii.back() = qSqrt ( p.sqrDist ( state()->centers.back() ) );
    state()->startAngles.back() = qAtan2 ( p.y() - state()->centers.back().y(), p.x() - state()->centers.back().x() );
    if ( state()->startAngles.back() < 0 ) {
      state()->startAngles.back() += 2 * M_PI;
    }
    state()->stopAngles.back() = state()->startAngles.back() + 2 * M_PI;
  } else if ( state()->sectorStatus == State::HaveRadius ) {
    state()->stopAngles.back() = qAtan2 ( p.y() - state()->centers.back().y(), p.x() - state()->centers.back().x() );
    while ( state()->stopAngles.back() <= state()->startAngles.back() ) {
      state()->stopAngles.back() += 2 * M_PI;
    }

    // Snap to full circle if within 5px
    if ( mapSettings ) {
      QgsCoordinateTransform crst ( mCrs, mapSettings->destinationCrs(), mapSettings->transformContext() );
      const QgsPointXY& center = state()->centers.back();
      const double& radius = state()->radii.back();
      const double& startAngle = state()->startAngles.back();
      const double& stopAngle = state()->stopAngles.back();
      QgsPointXY pStart ( center.x() + radius * qCos ( startAngle ),
                          center.y() + radius * qSin ( startAngle ) );
      QgsPointXY pEnd ( center.x() + radius * qCos ( stopAngle ),
                        center.y() + radius * qSin ( stopAngle ) );
      pStart = mapSettings->mapToPixel().transform ( crst.transform ( pStart ) );
      pEnd = mapSettings->mapToPixel().transform ( crst.transform ( pEnd ) );
      if ( pStart.sqrDist ( pEnd ) < 25 ) {
        state()->stopAngles.back() = state()->startAngles.back() + 2 * M_PI;
      }
    }
  }
  recomputeDerived();
}

void KadasCircularSectorItem::setCurrentAttributes ( const AttribValues& values )
{
  state()->sectorStatus = values[AttrR] > 0 ? State::HaveRadius : State::HaveCenter;
  state()->centers.last() = QgsPointXY ( values[AttrX], values[AttrY] );
  state()->radii.last() = values[AttrR];
  state()->startAngles.last() = values[AttrA1] / 180 * M_PI;
  state()->stopAngles.last() = values[AttrA2] / 180. * M_PI;
  if ( state()->stopAngles.last() <= state()->startAngles.last() ) {
    state()->stopAngles.last() += 2 * M_PI;
  }
  recomputeDerived();
}

bool KadasCircularSectorItem::continuePart()
{
  if ( state()->sectorStatus == State::HaveCenter ) {
    state()->sectorStatus = State::HaveRadius;
    return true;
  }
  return false;
}

void KadasCircularSectorItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasCircularSectorItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert ( AttrX, NumericAttribute{"x"} );
  attributes.insert ( AttrY, NumericAttribute{"y"} );
  attributes.insert ( AttrR, NumericAttribute{"r", 0} );
  attributes.insert ( AttrA1, NumericAttribute{QString ( QChar ( 0x03B1 ) ) + "1", 0} );
  attributes.insert ( AttrA2, NumericAttribute{QString ( QChar ( 0x03B1 ) ) + "2", 0} );
  return attributes;
}

KadasMapItem::AttribValues KadasCircularSectorItem::drawAttribsFromPosition ( const QgsPointXY& pos ) const
{
  AttribValues attributes;
  if ( constState()->drawStatus == State::Empty ) {
    attributes.insert ( AttrX, pos.x() );
    attributes.insert ( AttrY, pos.y() );
    attributes.insert ( AttrR, 0 );
    attributes.insert ( AttrA1, 0 );
    attributes.insert ( AttrA2, 0 );
  } else {
    attributes.insert ( AttrX, constState()->centers.last().x() );
    attributes.insert ( AttrY, constState()->centers.last().y() );
    attributes.insert ( AttrR, constState()->radii.last() );
    attributes.insert ( AttrA1, constState()->startAngles.last() / M_PI * 180. );
    attributes.insert ( AttrA2, constState()->stopAngles.last() / M_PI * 180. );
  }
  return attributes;
}

QgsPointXY KadasCircularSectorItem::positionFromDrawAttribs ( const AttribValues& values ) const
{
  return QgsPointXY ( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasCircularSectorItem::getEditContext ( const QgsPointXY& pos, const QgsMapSettings& mapSettings ) const
{
  // Not yet implemented
  return EditContext();
}

void KadasCircularSectorItem::edit ( const EditContext& context, const QgsPointXY& newPoint, const QgsMapSettings* mapSettings )
{
  // Not yet implemented
}

void KadasCircularSectorItem::edit ( const EditContext& context, const AttribValues& values )
{
  // Not yet implemented
}

KadasMapItem::AttribValues KadasCircularSectorItem::editAttribsFromPosition ( const EditContext& context, const QgsPointXY& pos ) const
{
  AttribValues values;
  // Not yet implemented
  return values;
}

QgsPointXY KadasCircularSectorItem::positionFromEditAttribs ( const EditContext& context, const AttribValues& values, const QgsMapSettings& mapSettings ) const
{
  // Not yet implemented
  return QgsPointXY();
}

void KadasCircularSectorItem::addPartFromGeometry ( const QgsAbstractGeometry* geom )
{
  // Not yet implemented
}

const QgsMultiSurface* KadasCircularSectorItem::geometry() const
{
  return static_cast<QgsMultiSurface*> ( mGeometry );
}

QgsMultiSurface* KadasCircularSectorItem::geometry()
{
  return static_cast<QgsMultiSurface*> ( mGeometry );
}

void KadasCircularSectorItem::measureGeometry()
{
  // Not yet implemented
}

void KadasCircularSectorItem::recomputeDerived()
{
  QgsMultiSurface* multiGeom = new QgsMultiSurface();
  for ( int i = 0, n = state()->centers.size(); i < n; ++i ) {
    const QgsPointXY& center = state()->centers[i];
    double radius = state()->radii[i];
    double startAngle = state()->startAngles[i];
    double stopAngle = state()->stopAngles[i];
    QgsCompoundCurve* exterior = new QgsCompoundCurve();
    if ( stopAngle - startAngle < 2 * M_PI - std::numeric_limits<float>::epsilon() ) {
      double alphaMid = 0.5 * ( startAngle + stopAngle );
      QgsPoint pStart = QgsPoint ( center.x() + radius * qCos ( startAngle ),
                                   center.y() + radius * qSin ( startAngle ) );
      QgsPoint pMid = QgsPoint ( center.x() + radius * qCos ( alphaMid ),
                                 center.y() + radius * qSin ( alphaMid ) );
      QgsPoint pEnd = QgsPoint ( center.x() + radius * qCos ( stopAngle ),
                                 center.y() + radius * qSin ( stopAngle ) );
      exterior->addCurve ( new QgsCircularString ( pStart, pMid, pEnd ) );

      exterior->addCurve ( new QgsLineString ( QgsPointSequence() << pEnd << QgsPoint ( center ) << pStart ) );
    } else {
      QgsCircularString* arc = new QgsCircularString();
      arc->setPoints ( QgsPointSequence()
                       << QgsPoint ( center.x(), center.y() + radius )
                       << QgsPoint ( center.x() + radius, center.y() )
                       << QgsPoint ( center.x(), center.y() - radius )
                       << QgsPoint ( center.x() - radius, center.y() )
                       << QgsPoint ( center.x(), center.y() + radius )
                     );
      exterior->addCurve ( arc );
    }
    QgsPolygon* poly = new QgsPolygon;
    poly->setExteriorRing ( exterior );
    multiGeom->addGeometry ( poly );
  }
  setInternalGeometry ( multiGeom );
}
