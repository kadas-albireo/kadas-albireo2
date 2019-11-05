/***************************************************************************
    kadasbullseyelayer.cpp
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

#include <QAction>
#include <QMenu>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>

#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgslayertreeview.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/app/bullseye/kadasbullseyelayer.h>
#include <kadas/app/bullseye/kadasmaptoolbullseye.h>


class KadasBullseyeLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasBullseyeLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id() )
      , mLayer( layer )
      , mRendererContext( rendererContext )
      , mGeod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() )
    {
      mDa.setEllipsoid( "WGS84" );
      mDa.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), mRendererContext.transformContext() );
    }

    bool render() override
    {
      if ( mLayer->mRings <= 0 || mLayer->mInterval <= 0 )
      {
        return true;
      }

      const QgsMapToPixel &mapToPixel = mRendererContext.mapToPixel();
      bool labelAxes = mLayer->mLabellingMode == LABEL_AXES || mLayer->mLabellingMode == LABEL_AXES_RINGS;
      bool labelRings = mLayer->mLabellingMode == LABEL_RINGS || mLayer->mLabellingMode == LABEL_AXES_RINGS;

      mRendererContext.painter()->save();
      mRendererContext.painter()->setOpacity( mLayer->mOpacity / 100. );
      mRendererContext.painter()->setCompositionMode( QPainter::CompositionMode_Source );
      mRendererContext.painter()->setPen( QPen( mLayer->mColor, mLayer->mLineWidth ) );
      QFont font = mRendererContext.painter()->font();
      font.setPixelSize( mLayer->mFontSize );
      mRendererContext.painter()->setFont( font );
      QFontMetrics metrics( mRendererContext.painter()->font() );

      QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
      QgsCoordinateTransform ct( mLayer->crs(), crsWgs84, mRendererContext.transformContext() );
      QgsCoordinateTransform rct( crsWgs84, mRendererContext.coordinateTransform().destinationCrs(), mRendererContext.transformContext() );

      // Draw rings
      QgsPointXY wgsCenter = ct.transform( mLayer->mCenter );
      double nm2meters = QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::DistanceNauticalMiles, QgsUnitTypes::DistanceMeters );
      for ( int iRing = 0; iRing < mLayer->mRings; ++iRing )
      {
        double radMeters = mLayer->mInterval * ( 1 + iRing ) * nm2meters;

        QPolygonF poly;
        for ( int a = 0; a <= 360; ++a )
        {
          QgsPointXY wgsPoint = mDa.computeSpheroidProject( wgsCenter, radMeters, a / 180. * M_PI );
          QgsPointXY mapPoint = rct.transform( wgsPoint );
          poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        }
        QPainterPath path;
        path.addPolygon( poly );
        mRendererContext.painter()->drawPath( path );
        if ( labelRings )
        {
          QString label = QString( "%1 nm" ).arg( ( iRing + 1 ) * mLayer->mInterval, 0, 'f', 2 );
          double x = poly.last().x() - 0.5 * metrics.width( label );
          mRendererContext.painter()->drawText( x, poly.last().y() - 0.25 * metrics.height(), label );
        }
      }

      // Draw axes
      double axisRadiusMeters = mLayer->mInterval * ( mLayer->mRings + 1 ) * nm2meters;
      for ( int bearing = 0; bearing < 360; bearing += mLayer->mAxesInterval )
      {
        QgsPointXY wgsPoint = mDa.computeSpheroidProject( wgsCenter, axisRadiusMeters, bearing / 180. * M_PI );
        GeographicLib::GeodesicLine line = mGeod.InverseLine( wgsCenter.y(), wgsCenter.x(), wgsPoint.y(), wgsPoint.x() );
        double dist = line.Distance();
        double sdist = 100000; // ~100km segments
        int nSegments = qMax( 1, int( std::ceil( dist / sdist ) ) );
        QPolygonF poly;
        for ( int iSeg = 0; iSeg < nSegments; ++iSeg )
        {
          double lat, lon;
          line.Position( iSeg * sdist, lat, lon );
          QgsPointXY mapPoint = rct.transform( QgsPoint( lon, lat ) );
          poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        }
        double lat, lon;
        line.Position( dist, lat, lon );
        QgsPointXY mapPoint = rct.transform( QgsPoint( lon, lat ) );
        poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        QPainterPath path;
        path.addPolygon( poly );
        mRendererContext.painter()->drawPath( path );
        if ( labelAxes )
        {
          QString label = QString( "%1°" ).arg( bearing );
          int n = poly.size();
          double dx = n > 1 ? poly[n - 1].x() - poly[n - 2].x() : 0;
          double dy = n > 1 ? poly[n - 1].y() - poly[n - 2].y() : 0;
          double l = std::sqrt( dx * dx + dy * dy );
          double d = mLayer->mFontSize;
          double w = metrics.width( label );
          double x = n < 2 ? poly.last().x() : poly.last().x() + d * dx / l;
          double y = n < 2 ? poly.last().y() : poly.last().y() + d * dy / l;
          mRendererContext.painter()->drawText( x - 0.5 * w, y - d, w, 2 * d, Qt::AlignCenter | Qt::AlignHCenter, label );
        }
      }

      mRendererContext.painter()->restore();
      return true;
    }

  private:
    KadasBullseyeLayer *mLayer;
    QgsRenderContext &mRendererContext;
    QgsDistanceArea mDa;
    GeographicLib::Geodesic mGeod;

    QPair<QPointF, QPointF> screenLine( const QgsPoint &p1, const QgsPoint &p2 ) const
    {
      const QgsMapToPixel &mapToPixel = mRendererContext.mapToPixel();
      QPointF sp1 = mapToPixel.transform( mRendererContext.coordinateTransform().transform( p1 ) ).toQPointF();
      QPointF sp2 = mapToPixel.transform( mRendererContext.coordinateTransform().transform( p2 ) ).toQPointF();
      return qMakePair( sp1, sp2 );
    }
};

KadasBullseyeLayer::KadasBullseyeLayer( const QString &name )
  : KadasPluginLayer( layerTypeKey(), name )
{
  mValid = true;
}

void KadasBullseyeLayer::setup( const QgsPointXY &center, const QgsCoordinateReferenceSystem &crs, int rings, double interval, double axesInterval )
{
  mCenter = center;
  mRings = rings;
  mInterval = interval;
  mAxesInterval = axesInterval;
  setCrs( crs, false );
}

KadasBullseyeLayer *KadasBullseyeLayer::clone() const
{
  KadasBullseyeLayer *layer = new KadasBullseyeLayer( name() );
  layer->mTransformContext = mTransformContext;
  layer->mOpacity = mOpacity;
  layer->mCenter = mCenter;
  layer->mRings = mRings;
  layer->mInterval = mInterval;
  layer->mAxesInterval = mAxesInterval;
  layer->mColor = mColor;
  layer->mFontSize = mFontSize;
  layer->mLabellingMode = mLabellingMode;
  layer->mLineWidth = mLineWidth;
  return layer;
}

QgsMapLayerRenderer *KadasBullseyeLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle KadasBullseyeLayer::extent() const
{
  QgsDistanceArea da;
  double radius = mRings * mInterval * QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::DistanceNauticalMiles, QgsUnitTypes::DistanceMeters );
  radius *= QgsUnitTypes::fromUnitToUnitFactor( QgsUnitTypes::DistanceMeters, crs().mapUnits() );
  return QgsRectangle( mCenter.x() - radius, mCenter.y() - radius, mCenter.x() + radius, mCenter.y() + radius );
}

bool KadasBullseyeLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  mOpacity = 100. - layerEl.attribute( "transparency" ).toInt();
  mCenter.setX( layerEl.attribute( "x" ).toDouble() );
  mCenter.setY( layerEl.attribute( "y" ).toDouble() );
  mRings = layerEl.attribute( "rings" ).toInt();
  mAxesInterval = layerEl.attribute( "axes" ).toDouble();
  mInterval = layerEl.attribute( "interval" ).toDouble();
  mFontSize = layerEl.attribute( "fontSize" ).toInt();
  mLineWidth = layerEl.attribute( "lineWidth" ).toInt();
  mColor = QgsSymbolLayerUtils::decodeColor( layerEl.attribute( "color" ) );
  mLabellingMode = static_cast<LabellingMode>( layerEl.attribute( "labellingMode" ).toInt() );

  setCrs( QgsCoordinateReferenceSystem( layerEl.attribute( "crs" ) ) );
  return true;
}

bool KadasBullseyeLayer::writeXml( QDomNode &layer_node, QDomDocument &document, const QgsReadWriteContext &context ) const
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  layerEl.setAttribute( "transparency", 100. - mOpacity );
  layerEl.setAttribute( "x", mCenter.x() );
  layerEl.setAttribute( "y", mCenter.y() );
  layerEl.setAttribute( "rings", mRings );
  layerEl.setAttribute( "axes", mAxesInterval );
  layerEl.setAttribute( "interval", mInterval );
  layerEl.setAttribute( "crs", crs().authid() );
  layerEl.setAttribute( "fontSize", mFontSize );
  layerEl.setAttribute( "lineWidth", mLineWidth );
  layerEl.setAttribute( "color", QgsSymbolLayerUtils::encodeColor( mColor ) );
  layerEl.setAttribute( "labellingMode", static_cast<int>( mLabellingMode ) );
  return true;
}

void KadasBullseyeLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  menu->addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, [this, layer]
  {
    mActionBullseyeTool->trigger();
  } );
}
