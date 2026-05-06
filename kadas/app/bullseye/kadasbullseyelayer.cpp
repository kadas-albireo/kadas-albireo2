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
#include <QApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QScreen>
#include <QMenu>

#include <GeographicLib/Geodesic.hpp>
#include <GeographicLib/GeodesicLine.hpp>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinatereferencesystem.h>
#include <qgis/qgslayertreeview.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgsmapcanvas.h>
#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgstextformat.h>
#include <qgis/qgsunittypes.h>

#include <bullseye/kadasbullseyelayer.h>
#include <bullseye/kadasmaptoolbullseye.h>


namespace
{
  QgsCoordinateTransformContext transformContextForLayer()
  {
    if ( QgsProject::instance() )
      return QgsProject::instance()->transformContext();
    return QgsCoordinateTransformContext();
  }
} // namespace


class KadasBullseyeLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasBullseyeLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id(), &rendererContext )
      , mRenderBullseyeConfig( layer->mBullseyeConfig )
      , mRenderOpacity( layer->opacity() )
      , mLayerCrs( layer->crs() )
      , mGeod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() )
    {
      mDa.setEllipsoid( "WGS84" );
      mDa.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), renderContext()->transformContext() );
    }

    bool render() override
    {
      if ( mRenderBullseyeConfig.rings <= 0 || mRenderBullseyeConfig.interval <= 0 )
      {
        return true;
      }

      const QgsMapToPixel &mapToPixel = renderContext()->mapToPixel();
      double dpiScale = double( renderContext()->painter()->device()->logicalDpiX() ) / qApp->primaryScreen()->logicalDotsPerInchX();

      renderContext()->painter()->save();
      renderContext()->painter()->setOpacity( mRenderOpacity );
      renderContext()->painter()->setCompositionMode( QPainter::CompositionMode_Source );
      renderContext()->painter()->setPen( QPen( mRenderBullseyeConfig.color, mRenderBullseyeConfig.lineWidth ) );
      QFont font = renderContext()->painter()->font();
      font.setPixelSize( mRenderBullseyeConfig.fontSize * dpiScale );
      QFontMetrics metrics( renderContext()->painter()->font() );
      QColor bufferColor = ( 0.2126 * mRenderBullseyeConfig.color.red() + 0.7152 * mRenderBullseyeConfig.color.green() + 0.0722 * mRenderBullseyeConfig.color.blue() ) > 128 ? Qt::black : Qt::white;

      QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
      QgsCoordinateTransform ct( mLayerCrs, crsWgs84, renderContext()->transformContext() );
      QgsCoordinateTransform rct( crsWgs84, renderContext()->coordinateTransform().destinationCrs(), renderContext()->transformContext() );

      // Draw rings
      QgsPointXY wgsCenter = ct.transform( mRenderBullseyeConfig.center );
      double intervalUnit2meters = QgsUnitTypes::fromUnitToUnitFactor( mRenderBullseyeConfig.intervalUnit, Qgis::DistanceUnit::Meters );
      for ( int iRing = 0; iRing < mRenderBullseyeConfig.rings; ++iRing )
      {
        double radMeters = mRenderBullseyeConfig.interval * ( 1 + iRing ) * intervalUnit2meters;

        QPolygonF poly;
        for ( int a = 0; a <= 360; ++a )
        {
          QgsPointXY wgsPoint = mDa.computeSpheroidProject( wgsCenter, radMeters, a / 180. * M_PI );
          QgsPointXY mapPoint = rct.transform( wgsPoint );
          poly.append( mapToPixel.transform( mapPoint ).toQPointF() );
        }
        QPainterPath path;
        path.addPolygon( poly );
        renderContext()->painter()->drawPath( path );
        if ( mRenderBullseyeConfig.labelRings )
        {
          QString label = QString( "%1 %2" ).arg( ( iRing + 1 ) * mRenderBullseyeConfig.interval, 0, 'f', 2 ).arg( QgsUnitTypes::toAbbreviatedString( mRenderBullseyeConfig.intervalUnit ) );
          double x = poly.last().x() - 0.5 * metrics.horizontalAdvance( label );
          drawGridLabel( x, poly.last().y() - 0.25 * metrics.height(), label, font, bufferColor );
        }
      }

      // Draw axes
      double axisRadiusMeters = mRenderBullseyeConfig.interval * ( mRenderBullseyeConfig.rings + 1 ) * intervalUnit2meters;
      for ( int bearing = 0; bearing < 360; bearing += mRenderBullseyeConfig.axesInterval )
      {
        QgsPointXY wgsPoint = mDa.computeSpheroidProject( wgsCenter, axisRadiusMeters, bearing / 180. * M_PI );
        GeographicLib::GeodesicLine line = mGeod.InverseLine( wgsCenter.y(), wgsCenter.x(), wgsPoint.y(), wgsPoint.x() );
        double dist = line.Distance();
        double sdist = 100000; // ~100km segments
        int nSegments = std::max( 1, int( std::ceil( dist / sdist ) ) );
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
        renderContext()->painter()->drawPath( path );
        if ( mRenderBullseyeConfig.labelAxes )
        {
          QString label = QString( "%1°" ).arg( bearing );
          int n = poly.size();
          double dx = n > 1 ? poly[n - 1].x() - poly[n - 2].x() : 0;
          double dy = n > 1 ? poly[n - 1].y() - poly[n - 2].y() : 0;
          double l = std::sqrt( dx * dx + dy * dy );
          double d = mRenderBullseyeConfig.fontSize;
          double w = metrics.horizontalAdvance( label );
          double x = n < 2 ? poly.last().x() : poly.last().x() + d * dx / l;
          double y = n < 2 ? poly.last().y() : poly.last().y() + d * dy / l;
          drawGridLabel( x - w, y + 0.5 * metrics.ascent(), label, font, bufferColor );
        }
      }
      if ( mRenderBullseyeConfig.labelQuadrants )
      {
        const char firstLetter = 'F';
        QList<char> labelChars = { firstLetter };
        for ( int iRing = 0; iRing < mRenderBullseyeConfig.rings; ++iRing )
        {
          double r = mRenderBullseyeConfig.interval * ( 0.5 + iRing ) * intervalUnit2meters;
          for ( int bearing = 0; bearing < 360; bearing += mRenderBullseyeConfig.axesInterval )
          {
            double a = bearing + 0.5 * mRenderBullseyeConfig.axesInterval;
            QgsPointXY wgsPoint = mDa.computeSpheroidProject( wgsCenter, r, a / 180. * M_PI );
            QgsPointXY mapPoint = rct.transform( wgsPoint );
            QPointF screenPoint = mapToPixel.transform( mapPoint ).toQPointF();
            QString label;
            for ( char c : labelChars )
            {
              label += c;
            }
            drawGridLabel( screenPoint.x(), screenPoint.y(), label, font, bufferColor );
            if ( labelChars.last() == 'Z' )
            {
              labelChars.last() = firstLetter;
              labelChars.append( firstLetter );
            }
            else
            {
              ++labelChars.last();
              if ( labelChars.last() == 'I' || labelChars.last() == 'O' )
              {
                ++labelChars.last();
              }
            }
          }
        }
      }

      renderContext()->painter()->restore();
      return true;
    }

  private:
    KadasBullseyeLayer::BullseyeConfig mRenderBullseyeConfig;
    double mRenderOpacity = 1.;
    QgsCoordinateReferenceSystem mLayerCrs;
    QgsDistanceArea mDa;
    GeographicLib::Geodesic mGeod;

    QPair<QPointF, QPointF> screenLine( const QgsPoint &p1, const QgsPoint &p2 ) const
    {
      const QgsMapToPixel &mapToPixel = renderContext()->mapToPixel();
      QPointF sp1 = mapToPixel.transform( renderContext()->coordinateTransform().transform( p1 ) ).toQPointF();
      QPointF sp2 = mapToPixel.transform( renderContext()->coordinateTransform().transform( p2 ) ).toQPointF();
      return qMakePair( sp1, sp2 );
    }
    void drawGridLabel( double x, double y, const QString &text, const QFont &font, const QColor &bufferColor )
    {
      QPainterPath path;
      path.addText( x, y, font, text );
      renderContext()->painter()->save();
      renderContext()->painter()->setBrush( mRenderBullseyeConfig.color );
      renderContext()->painter()->setPen( QPen( bufferColor, qRound( mRenderBullseyeConfig.fontSize / 8. ) ) );
      renderContext()->painter()->drawPath( path );
      renderContext()->painter()->setPen( Qt::NoPen );
      renderContext()->painter()->drawPath( path );
      renderContext()->painter()->restore();
    }
};

KadasBullseyeLayer::KadasBullseyeLayer( const QString &name )
  : QgsAnnotationLayer( name, QgsAnnotationLayer::LayerOptions( transformContextForLayer() ) )
{}

void KadasBullseyeLayer::setup( const QgsPointXY &center, const QgsCoordinateReferenceSystem &crs, int rings, double interval, Qgis::DistanceUnit intervalUnit, double axesInterval )
{
  mBullseyeConfig.center = center;
  mBullseyeConfig.rings = rings;
  mBullseyeConfig.interval = interval;
  mBullseyeConfig.intervalUnit = intervalUnit;
  mBullseyeConfig.axesInterval = axesInterval;
  setCrs( crs, false );
  regenerate();
}

KadasBullseyeLayer *KadasBullseyeLayer::clone() const
{
  KadasBullseyeLayer *layer = new KadasBullseyeLayer( name() );
  layer->setOpacity( opacity() );
  layer->setCrs( crs(), false );
  layer->mBullseyeConfig = mBullseyeConfig;
  layer->regenerate();
  return layer;
}

QgsMapLayerRenderer *KadasBullseyeLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle KadasBullseyeLayer::extent() const
{
  double radius = mBullseyeConfig.rings * mBullseyeConfig.interval * QgsUnitTypes::fromUnitToUnitFactor( mBullseyeConfig.intervalUnit, Qgis::DistanceUnit::Meters );
  radius *= QgsUnitTypes::fromUnitToUnitFactor( Qgis::DistanceUnit::Meters, crs().mapUnits() );
  return QgsRectangle( mBullseyeConfig.center.x() - radius, mBullseyeConfig.center.y() - radius, mBullseyeConfig.center.x() + radius, mBullseyeConfig.center.y() + radius );
}

bool KadasBullseyeLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  // Let the stock annotation layer load its own state (CRS, opacity, items,
  // *and* customProperties).
  QgsAnnotationLayer::readXml( layer_node, context );

  // Preferred path: read config from layer customProperties. These survive a
  // round-trip through vanilla QGIS (which has no knowledge of our subclass
  // and would silently strip any custom child element written by writeXml).
  if ( customProperty( QStringLiteral( "kadas/annotation-type" ) ).toString() == QLatin1String( "bullseye" ) )
  {
    mBullseyeConfig.center.setX( customProperty( QStringLiteral( "kadas/bullseye/x" ) ).toDouble() );
    mBullseyeConfig.center.setY( customProperty( QStringLiteral( "kadas/bullseye/y" ) ).toDouble() );
    mBullseyeConfig.rings = customProperty( QStringLiteral( "kadas/bullseye/rings" ) ).toInt();
    mBullseyeConfig.axesInterval = customProperty( QStringLiteral( "kadas/bullseye/axes" ) ).toDouble();
    mBullseyeConfig.interval = customProperty( QStringLiteral( "kadas/bullseye/interval" ) ).toDouble();
    const QString unitStr = customProperty( QStringLiteral( "kadas/bullseye/intervalUnit" ) ).toString();
    mBullseyeConfig.intervalUnit = unitStr.isEmpty() ? Qgis::DistanceUnit::NauticalMiles : QgsUnitTypes::decodeDistanceUnit( unitStr );
    mBullseyeConfig.fontSize = customProperty( QStringLiteral( "kadas/bullseye/fontSize" ) ).toInt();
    mBullseyeConfig.lineWidth = customProperty( QStringLiteral( "kadas/bullseye/lineWidth" ) ).toInt();
    mBullseyeConfig.color = QgsSymbolLayerUtils::decodeColor( customProperty( QStringLiteral( "kadas/bullseye/color" ) ).toString() );
    mBullseyeConfig.labelAxes = customProperty( QStringLiteral( "kadas/bullseye/labelAxes" ) ).toBool();
    mBullseyeConfig.labelQuadrants = customProperty( QStringLiteral( "kadas/bullseye/labelQuadrants" ) ).toBool();
    mBullseyeConfig.labelRings = customProperty( QStringLiteral( "kadas/bullseye/labelRings" ) ).toBool();
    regenerate();
    return true;
  }

  // Legacy fallback: pre-customProperty projects stored config as a child
  // element on the layer node. Drop after one release of burn-in.
  const QDomElement layerEl = layer_node.toElement();
  const QDomElement cfgEl = layerEl.firstChildElement( QStringLiteral( "KadasBullseye" ) );
  if ( cfgEl.isNull() )
    return true;

  mBullseyeConfig.center.setX( cfgEl.attribute( "x" ).toDouble() );
  mBullseyeConfig.center.setY( cfgEl.attribute( "y" ).toDouble() );
  mBullseyeConfig.rings = cfgEl.attribute( "rings" ).toInt();
  mBullseyeConfig.axesInterval = cfgEl.attribute( "axes" ).toDouble();
  mBullseyeConfig.interval = cfgEl.attribute( "interval" ).toDouble();
  if ( cfgEl.hasAttribute( "intervalUnit" ) )
    mBullseyeConfig.intervalUnit = QgsUnitTypes::decodeDistanceUnit( cfgEl.attribute( "intervalUnit" ) );
  else
    mBullseyeConfig.intervalUnit = Qgis::DistanceUnit::NauticalMiles;
  mBullseyeConfig.fontSize = cfgEl.attribute( "fontSize" ).toInt();
  mBullseyeConfig.lineWidth = cfgEl.attribute( "lineWidth" ).toInt();
  mBullseyeConfig.color = QgsSymbolLayerUtils::decodeColor( cfgEl.attribute( "color" ) );
  mBullseyeConfig.labelAxes = cfgEl.attribute( "labelAxes" ) == "1";
  mBullseyeConfig.labelQuadrants = cfgEl.attribute( "labelQuadrants" ) == "1";
  mBullseyeConfig.labelRings = cfgEl.attribute( "labelRings" ) == "1";

  regenerate();
  return true;
}

bool KadasBullseyeLayer::writeXml( QDomNode &layer_node, QDomDocument &doc, const QgsReadWriteContext &context ) const
{
  // Stash config on the layer's customProperties *before* the base writes
  // them out. This is the only persistence mechanism that survives a vanilla
  // QGIS save (which would otherwise strip any subclass-specific child
  // element since QGIS doesn't know about KadasBullseyeLayer).
  const_cast<KadasBullseyeLayer *>( this )->writeConfigToCustomProperties();
  return QgsAnnotationLayer::writeXml( layer_node, doc, context );
}

void KadasBullseyeLayer::writeConfigToCustomProperties()
{
  setCustomProperty( QStringLiteral( "kadas/annotation-type" ), QStringLiteral( "bullseye" ) );
  setCustomProperty( QStringLiteral( "kadas/bullseye/x" ), mBullseyeConfig.center.x() );
  setCustomProperty( QStringLiteral( "kadas/bullseye/y" ), mBullseyeConfig.center.y() );
  setCustomProperty( QStringLiteral( "kadas/bullseye/rings" ), mBullseyeConfig.rings );
  setCustomProperty( QStringLiteral( "kadas/bullseye/axes" ), mBullseyeConfig.axesInterval );
  setCustomProperty( QStringLiteral( "kadas/bullseye/interval" ), mBullseyeConfig.interval );
  setCustomProperty( QStringLiteral( "kadas/bullseye/intervalUnit" ), QgsUnitTypes::encodeUnit( mBullseyeConfig.intervalUnit ) );
  setCustomProperty( QStringLiteral( "kadas/bullseye/fontSize" ), mBullseyeConfig.fontSize );
  setCustomProperty( QStringLiteral( "kadas/bullseye/lineWidth" ), mBullseyeConfig.lineWidth );
  setCustomProperty( QStringLiteral( "kadas/bullseye/color" ), QgsSymbolLayerUtils::encodeColor( mBullseyeConfig.color ) );
  setCustomProperty( QStringLiteral( "kadas/bullseye/labelAxes" ), mBullseyeConfig.labelAxes );
  setCustomProperty( QStringLiteral( "kadas/bullseye/labelQuadrants" ), mBullseyeConfig.labelQuadrants );
  setCustomProperty( QStringLiteral( "kadas/bullseye/labelRings" ), mBullseyeConfig.labelRings );
}

void KadasBullseyeLayer::regenerate()
{
  clear();

  if ( mBullseyeConfig.rings <= 0 || mBullseyeConfig.interval <= 0 )
  {
    triggerRepaint();
    return;
  }

  // Geodesic ring + axis sampling, mirroring the live renderer. Items are
  // emitted in this layer's CRS (which is the same CRS the user picked at
  // creation time); QgsAnnotationLayer reprojects on the fly when a project
  // uses a different destination CRS in QGIS.
  QgsDistanceArea da;
  da.setEllipsoid( "WGS84" );
  da.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), transformContext() );

  const QgsCoordinateReferenceSystem crsWgs84( "EPSG:4326" );
  const QgsCoordinateTransform layerToWgs( crs(), crsWgs84, transformContext() );
  const QgsCoordinateTransform wgsToLayer( crsWgs84, crs(), transformContext() );
  QgsPointXY wgsCenter;
  try
  {
    wgsCenter = layerToWgs.transform( mBullseyeConfig.center );
  }
  catch ( ... )
  {
    triggerRepaint();
    return;
  }

  const double intervalUnit2meters = QgsUnitTypes::fromUnitToUnitFactor( mBullseyeConfig.intervalUnit, Qgis::DistanceUnit::Meters );

  auto makeLineSymbol = [&]() -> QgsLineSymbol * {
    auto sym = QgsLineSymbol::createSimple( {} );
    sym->setColor( mBullseyeConfig.color );
    sym->setWidth( mBullseyeConfig.lineWidth );
    sym->setWidthUnit( Qgis::RenderUnit::Pixels );
    return sym.release();
  };

  auto makeTextFormat = [&]( double fontPx ) {
    QgsTextFormat fmt;
    QFont f = fmt.font();
    f.setPixelSize( static_cast<int>( fontPx ) );
    fmt.setFont( f );
    fmt.setSize( fontPx );
    fmt.setSizeUnit( Qgis::RenderUnit::Pixels );
    fmt.setColor( mBullseyeConfig.color );
    QgsTextBufferSettings buf = fmt.buffer();
    const bool darkText = ( 0.2126 * mBullseyeConfig.color.red() + 0.7152 * mBullseyeConfig.color.green() + 0.0722 * mBullseyeConfig.color.blue() ) > 128;
    buf.setColor( darkText ? Qt::black : Qt::white );
    buf.setSize( std::max( 1.0, mBullseyeConfig.fontSize / 8.0 ) );
    buf.setSizeUnit( Qgis::RenderUnit::Pixels );
    buf.setEnabled( true );
    fmt.setBuffer( buf );
    return fmt;
  };

  const QgsTextFormat textFmt = makeTextFormat( mBullseyeConfig.fontSize );

  // --- Rings ---
  for ( int iRing = 0; iRing < mBullseyeConfig.rings; ++iRing )
  {
    const double radMeters = mBullseyeConfig.interval * ( 1 + iRing ) * intervalUnit2meters;
    QgsPointSequence pts;
    QgsPointXY topPoint;
    for ( int a = 0; a <= 360; ++a )
    {
      const QgsPointXY wgsPoint = da.computeSpheroidProject( wgsCenter, radMeters, a / 180. * M_PI );
      const QgsPointXY layerPoint = wgsToLayer.transform( wgsPoint );
      pts << QgsPoint( layerPoint.x(), layerPoint.y() );
      if ( a == 360 )
        topPoint = layerPoint;
    }
    auto *line = new QgsLineString( pts );
    auto *item = new QgsAnnotationLineItem( line );
    item->setSymbol( makeLineSymbol() );
    addItem( item );

    if ( mBullseyeConfig.labelRings )
    {
      const QString label = QString( "%1 %2" ).arg( ( iRing + 1 ) * mBullseyeConfig.interval, 0, 'f', 2 ).arg( QgsUnitTypes::toAbbreviatedString( mBullseyeConfig.intervalUnit ) );
      auto *txt = new QgsAnnotationPointTextItem( label, topPoint );
      txt->setFormat( textFmt );
      txt->setAlignment( Qt::AlignHCenter | Qt::AlignBottom );
      addItem( txt );
    }
  }

  // --- Axes ---
  const double axisRadiusMeters = mBullseyeConfig.interval * ( mBullseyeConfig.rings + 1 ) * intervalUnit2meters;
  GeographicLib::Geodesic geod( GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f() );
  for ( int bearing = 0; bearing < 360; bearing += static_cast<int>( mBullseyeConfig.axesInterval ) )
  {
    const QgsPointXY wgsPoint = da.computeSpheroidProject( wgsCenter, axisRadiusMeters, bearing / 180. * M_PI );
    GeographicLib::GeodesicLine line = geod.InverseLine( wgsCenter.y(), wgsCenter.x(), wgsPoint.y(), wgsPoint.x() );
    const double dist = line.Distance();
    const double sdist = 100000; // ~100 km segments, matches live renderer
    const int nSegments = std::max( 1, int( std::ceil( dist / sdist ) ) );
    QgsPointSequence pts;
    QgsPointXY tipPoint;
    for ( int iSeg = 0; iSeg <= nSegments; ++iSeg )
    {
      const double s = std::min( iSeg * sdist, dist );
      double lat, lon;
      line.Position( s, lat, lon );
      const QgsPointXY layerPoint = wgsToLayer.transform( QgsPointXY( lon, lat ) );
      pts << QgsPoint( layerPoint.x(), layerPoint.y() );
      if ( iSeg == nSegments )
        tipPoint = layerPoint;
    }
    auto *axis = new QgsLineString( pts );
    auto *item = new QgsAnnotationLineItem( axis );
    item->setSymbol( makeLineSymbol() );
    addItem( item );

    if ( mBullseyeConfig.labelAxes )
    {
      const QString label = QString( "%1°" ).arg( bearing );
      auto *txt = new QgsAnnotationPointTextItem( label, tipPoint );
      txt->setFormat( textFmt );
      txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
      addItem( txt );
    }
  }

  // --- Quadrant alpha labels (one per ring × axis sector cell) ---
  if ( mBullseyeConfig.labelQuadrants )
  {
    const char firstLetter = 'F';
    QList<char> labelChars = { firstLetter };
    for ( int iRing = 0; iRing < mBullseyeConfig.rings; ++iRing )
    {
      const double r = mBullseyeConfig.interval * ( 0.5 + iRing ) * intervalUnit2meters;
      for ( int bearing = 0; bearing < 360; bearing += static_cast<int>( mBullseyeConfig.axesInterval ) )
      {
        const double a = bearing + 0.5 * mBullseyeConfig.axesInterval;
        const QgsPointXY wgsPoint = da.computeSpheroidProject( wgsCenter, r, a / 180. * M_PI );
        const QgsPointXY layerPoint = wgsToLayer.transform( wgsPoint );

        QString label;
        for ( char c : labelChars )
          label += c;

        auto *txt = new QgsAnnotationPointTextItem( label, layerPoint );
        txt->setFormat( textFmt );
        txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
        addItem( txt );

        if ( labelChars.last() == 'Z' )
        {
          labelChars.last() = firstLetter;
          labelChars.append( firstLetter );
        }
        else
        {
          ++labelChars.last();
          if ( labelChars.last() == 'I' || labelChars.last() == 'O' )
            ++labelChars.last();
        }
      }
    }
  }

  triggerRepaint();
}
