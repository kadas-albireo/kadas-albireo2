/***************************************************************************
    kadasmapgridlayer.cpp
    ---------------------
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

#include <QApplication>
#include <QDesktopWidget>
#include <QMenu>
#include <QVector2D>

#include <qgis/qgsapplication.h>
#include <qgis/qgscoordinateformatter.h>
#include <qgis/qgsgeometryutils.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/core/kadaslatlontoutm.h>
#include <kadas/app/mapgrid/kadasmapgridlayer.h>


class KadasMapGridLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasMapGridLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id(), &rendererContext )
      , mRenderGridConfig( layer->mGridConfig )
      , mRenderOpacity( layer->opacity() )
    {}

    bool render() override
    {
      renderContext()->painter()->save();
      renderContext()->painter()->setOpacity( mRenderOpacity );
      renderContext()->painter()->setCompositionMode( QPainter::CompositionMode_Source );
      renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, 1. ) );

      switch ( mRenderGridConfig.gridType )
      {
        case GridLV03:
          drawCrsGrid( "EPSG:21781", 100000, QgsCoordinateFormatter::FormatPair, 0, QgsCoordinateFormatter::FormatFlags() );
          break;
        case GridLV95:
          drawCrsGrid( "EPSG:2056", 100000, QgsCoordinateFormatter::FormatPair, 0, QgsCoordinateFormatter::FormatFlags() );
          break;
        case GridDD:
          drawCrsGrid( "EPSG:4326", 1, QgsCoordinateFormatter::FormatDecimalDegrees, 3, QgsCoordinateFormatter::FormatFlags() );
          break;
        case GridDM:
          drawCrsGrid( "EPSG:4326", 1, QgsCoordinateFormatter::FormatDegreesMinutes, 1, QgsCoordinateFormatter::FlagDegreesUseStringSuffix | QgsCoordinateFormatter::FlagDegreesPadMinutesSeconds );
          break;
        case GridDMS:
          drawCrsGrid( "EPSG:4326", 1, QgsCoordinateFormatter::FormatDegreesMinutesSeconds, 0, QgsCoordinateFormatter::FlagDegreesUseStringSuffix | QgsCoordinateFormatter::FlagDegreesPadMinutesSeconds );
          break;
        case GridUTM:
        case GridMGRS:
          drawMgrsGrid();
          break;
      }

      renderContext()->painter()->restore();
      return true;
    }

  private:
    KadasMapGridLayer::GridConfig mRenderGridConfig;
    double mRenderOpacity = 1.;

    struct GridLabel
    {
      QString text;
      QPointF screenPos;
    };

    void drawCrsGrid( const QString &crs, double segmentLength, QgsCoordinateFormatter::Format format, int precision, QgsCoordinateFormatter::FormatFlags flags )
    {
      QgsCoordinateTransform crst( QgsCoordinateReferenceSystem( crs ), renderContext()->coordinateTransform().destinationCrs(), renderContext()->transformContext() );
      QgsRectangle area = crst.transformBoundingBox( renderContext()->mapExtent(), Qgis::TransformDirection::Reverse );
      QRectF screenRect = computeScreenExtent( renderContext()->mapExtent(), renderContext()->mapToPixel() );

      QList<GridLabel> leftLabels;
      QList<GridLabel> rightLabels;
      QList<GridLabel> topLabels;
      QList<GridLabel> bottomLabels;

      double xStart = std::floor( area.xMinimum() / mRenderGridConfig.intervalX ) * mRenderGridConfig.intervalX;
      double xEnd = std::ceil( area.xMaximum() / mRenderGridConfig.intervalX ) * mRenderGridConfig.intervalX;
      double yStart = std::floor( area.yMinimum() / mRenderGridConfig.intervalY ) * mRenderGridConfig.intervalY;
      double yEnd = std::ceil( area.yMaximum() / mRenderGridConfig.intervalY ) * mRenderGridConfig.intervalY;

      const QVariantMap &renderFlags = renderContext()->customRenderingFlags();
      bool drawLabels = !( renderFlags["globe"].toBool() || renderFlags["kml"].toBool() || renderContext()->flags() & Qgis::RenderContextFlag::RenderPreviewJob );

      // If chosen intervals would result in over 100 grid lines, reduce interval
      double intervalX = mRenderGridConfig.intervalX;
      int numX = qRound( ( xEnd - xStart ) / intervalX ) + 1;
      while ( numX > 100 )
      {
        intervalX *= 2;
        numX = qRound( ( xEnd - xStart ) / intervalX ) + 1;
      }
      double intervalY = mRenderGridConfig.intervalY;
      int numY = qRound( ( yEnd - yStart ) / intervalY ) + 1;
      while ( numY > 100 )
      {
        intervalY *= 2;
        numY = qRound( ( yEnd - yStart ) / intervalY ) + 1;
      }

      // Vertical lines
      double ySegmentLength = intervalY / std::ceil( intervalY / segmentLength );
      for ( int ix = 0; ix <= numX; ++ix )
      {
        QPolygonF poly;
        double x = xStart + ix * intervalX;
        double y = yStart;
        while ( y - ySegmentLength <= area.yMaximum() )
        {
          QgsPointXY p = crst.transform( x, y );
          poly.append( renderContext()->mapToPixel().transform( p ).toQPointF() );
          y += ySegmentLength;
        }
        renderContext()->painter()->drawPolyline( poly );

        if ( drawLabels && mRenderGridConfig.labelingMode == LabelingEnabled )
        {
          int iSegment = 0, nSegments = poly.size() - 1;
          // Bottom edge label pos
          for ( ; iSegment < nSegments && poly[1 + iSegment].y() >= screenRect.bottom(); ++iSegment );
          if ( iSegment < nSegments )
          {
            QgsPoint inter;
            bool isInter = false;
            QgsGeometryUtils::segmentIntersection( QgsPoint( poly[iSegment] ), QgsPoint( poly[iSegment + 1] ), QgsPoint( screenRect.bottomLeft() ), QgsPoint( screenRect.bottomRight() ), inter, isInter );
            if ( isInter )
            {
              bottomLabels.append( {QgsCoordinateFormatter::formatX( x, format, precision, flags ), inter.toQPointF()} );
            }
          }

          // Top edge label pos
          iSegment = nSegments - 2;
          for ( ; iSegment >= 0 && poly[1 + iSegment].y() <= screenRect.top(); --iSegment );
          if ( iSegment >= 0 )
          {
            QgsPoint inter;
            bool isInter = false;
            QgsGeometryUtils::segmentIntersection( QgsPoint( poly[iSegment] ), QgsPoint( poly[iSegment + 1] ), QgsPoint( screenRect.topLeft() ), QgsPoint( screenRect.topRight() ), inter, isInter );
            if ( isInter )
            {
              topLabels.append( {QgsCoordinateFormatter::formatX( x, format, precision, flags ), inter.toQPointF()} );
            }
          }
        }
      }

      // Horizontal lines
      double xSegmentLength = intervalX / std::ceil( intervalX / segmentLength );
      for ( int iy = numY; iy >= 0; --iy )
      {
        QPolygonF poly;
        double x = xStart;
        double y = yStart + iy * intervalY;
        while ( x - xSegmentLength <= area.xMaximum() )
        {
          QgsPointXY p = crst.transform( x, y );
          poly.append( renderContext()->mapToPixel().transform( p ).toQPointF() );
          x += xSegmentLength;
        }
        renderContext()->painter()->drawPolyline( poly );

        if ( drawLabels && mRenderGridConfig.labelingMode == LabelingEnabled )
        {
          int iSegment = 0, nSegments = poly.size() - 1;
          // Left edge label pos
          for ( ; iSegment < nSegments && poly[1 + iSegment].y() <= screenRect.left(); ++iSegment );
          if ( iSegment < nSegments )
          {
            QgsPoint inter;
            bool isInter = false;
            QgsGeometryUtils::segmentIntersection( QgsPoint( poly[iSegment] ), QgsPoint( poly[iSegment + 1] ), QgsPoint( screenRect.bottomLeft() ), QgsPoint( screenRect.topLeft() ), inter, isInter );
            if ( isInter )
            {
              leftLabels.append( {QgsCoordinateFormatter::formatY( y, format, precision, flags ), inter.toQPointF()} );
            }
          }

          // Right edge label pos
          iSegment = nSegments - 2;
          for ( ; iSegment >= 0 && poly[1 + iSegment].y() >= screenRect.right(); --iSegment );
          if ( iSegment >= 0 )
          {
            QgsPoint inter;
            bool isInter = false;
            QgsGeometryUtils::segmentIntersection( QgsPoint( poly[iSegment] ), QgsPoint( poly[iSegment + 1] ), QgsPoint( screenRect.bottomRight() ), QgsPoint( screenRect.topRight() ), inter, isInter );
            if ( isInter )
            {
              rightLabels.append( {QgsCoordinateFormatter::formatY( y, format, precision, flags ), inter.toQPointF()} );
            }
          }
        }
      }

      if ( drawLabels && mRenderGridConfig.labelingMode == LabelingEnabled )
      {
        double dpiScale = double( renderContext()->painter()->device()->logicalDpiX() ) / qApp->desktop()->logicalDpiX();
        QFont font = renderContext()->painter()->font();
        font.setBold( true );
        font.setPointSizeF( mRenderGridConfig.fontSize * dpiScale );
        QFontMetrics fm( font );
        renderContext()->painter()->setBrush( mRenderGridConfig.color );

        QColor bufferColor = ( 0.2126 * mRenderGridConfig.color.red() + 0.7152 * mRenderGridConfig.color.green() + 0.0722 * mRenderGridConfig.color.blue() ) > 128 ? Qt::black : Qt::white;
        double screenMargin = 5;
        double labelMargin = 10;
        double lastDrawY = std::numeric_limits<double>::lowest();
        for ( const GridLabel &label : leftLabels )
        {
          QPointF pos = label.screenPos;
          pos.rx() += screenMargin;
          pos.ry() += fm.descent();
          if ( pos.y() > lastDrawY )
          {
            drawGridLabel( pos, label.text, font, bufferColor );
            lastDrawY = pos.y() + fm.height() + labelMargin;
          }
        }
        lastDrawY = std::numeric_limits<double>::lowest();
        for ( const GridLabel &label : rightLabels )
        {
          QPointF pos = label.screenPos;
          pos.rx() -= fm.horizontalAdvance( label.text ) + screenMargin;
          pos.ry() += fm.descent();
          if ( pos.y() > lastDrawY )
          {
            drawGridLabel( pos, label.text, font, bufferColor );
            lastDrawY = pos.y() + fm.height() + labelMargin;
          }
        }
        double lastDrawX = std::numeric_limits<double>::lowest();
        for ( const GridLabel &label : bottomLabels )
        {
          QPointF pos = label.screenPos;
          pos.rx() -= 0.5 * fm.horizontalAdvance( label.text );
          pos.ry() -= fm.descent() + screenMargin;
          if ( pos.x() > lastDrawX )
          {
            drawGridLabel( pos, label.text, font, bufferColor );
            lastDrawX = pos.x() + fm.horizontalAdvance( label.text ) + labelMargin;
          }
        }
        lastDrawX = std::numeric_limits<double>::lowest();
        for ( const GridLabel &label : topLabels )
        {
          QPointF pos = label.screenPos;
          pos.rx() -= 0.5 * fm.horizontalAdvance( label.text );
          pos.ry() += fm.ascent() + screenMargin;
          if ( pos.x() > lastDrawX )
          {
            drawGridLabel( pos, label.text, font, bufferColor );
            lastDrawX = pos.x() + fm.horizontalAdvance( label.text ) + labelMargin;
          }
        }
      }

    }

    void adjustZoneLabelPos( QPointF &labelPos, const QPointF &maxLabelPos, const QRectF &visibleExtent )
    {
      if ( !visibleExtent.contains( labelPos ) )
      {
        double x1 = visibleExtent.x();
        double x2 = visibleExtent.x() + visibleExtent.width();
        double y1 = visibleExtent.y();
        double y2 = visibleExtent.y() + visibleExtent.height();
        // Adjust left/right
        if ( labelPos.x() < maxLabelPos.x() && labelPos.x() < x1 && maxLabelPos.x() > x1 )
        {
          labelPos.setX( x1 );
        }
        else if ( labelPos.x() > maxLabelPos.x() && labelPos.x() > x2 && maxLabelPos.x() < x2 )
        {
          labelPos.setX( x2 );
        }
        // Adjust top/bottom
        if ( labelPos.y() < maxLabelPos.y() && labelPos.y() < y1 && maxLabelPos.y() > y1 )
        {
          labelPos.setY( y1 );
        }
        else if ( labelPos.y() > maxLabelPos.y() && labelPos.y() > y2 && maxLabelPos.y() < y2 )
        {
          labelPos.setY( y2 );
        }
      }
    }

    QRect computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel )
    {
      QPoint topLeft = mapToPixel.transform( mapExtent.xMinimum(), mapExtent.yMinimum() ).toQPointF().toPoint();
      QPoint topRight = mapToPixel.transform( mapExtent.xMaximum(), mapExtent.yMinimum() ).toQPointF().toPoint();
      QPoint bottomLeft = mapToPixel.transform( mapExtent.xMinimum(), mapExtent.yMaximum() ).toQPointF().toPoint();
      QPoint bottomRight = mapToPixel.transform( mapExtent.xMaximum(), mapExtent.yMaximum() ).toQPointF().toPoint();
      int xMin = std::min( std::min( topLeft.x(), topRight.x() ), std::min( bottomLeft.x(), bottomRight.x() ) );
      int xMax = std::max( std::max( topLeft.x(), topRight.x() ), std::max( bottomLeft.x(), bottomRight.x() ) );
      int yMin = std::min( std::min( topLeft.y(), topRight.y() ), std::min( bottomLeft.y(), bottomRight.y() ) );
      int yMax = std::max( std::max( topLeft.y(), topRight.y() ), std::max( bottomLeft.y(), bottomRight.y() ) );
      return QRect( xMin, yMin, xMax - xMin, yMax - yMin ).normalized();
    }

    void drawMgrsGrid()
    {
      const QVariantMap &renderFlags = renderContext()->customRenderingFlags();
      bool adaptToScreen = !( renderFlags["globe"].toBool() || renderFlags["kml"].toBool() );

      QgsCoordinateTransform crst( QgsCoordinateReferenceSystem( "EPSG:4326" ), renderContext()->coordinateTransform().destinationCrs(), renderContext()->transformContext() );
      QgsRectangle area = crst.transformBoundingBox( renderContext()->mapExtent(), Qgis::TransformDirection::Reverse );
      QRectF screenExtent = computeScreenExtent( renderContext()->mapExtent(), renderContext()->mapToPixel() );
      double mapScale = renderContext()->rendererScale();
      if ( !adaptToScreen )
      {
        area = area.buffered( area.width() );
      }

      QList<QPolygonF> zoneLines;
      QList<QPolygonF> subZoneLines;
      QList<QPolygonF> gridLines;
      QList<KadasLatLonToUTM::ZoneLabel> zoneLabels;
      QList<KadasLatLonToUTM::ZoneLabel> zoneSubLabels;
      QList<KadasLatLonToUTM::GridLabel> gridLabels;
      KadasLatLonToUTM::computeGrid( area, mapScale, zoneLines, subZoneLines, gridLines, zoneLabels, zoneSubLabels, gridLabels, mRenderGridConfig.gridType == GridMGRS ? KadasLatLonToUTM::GridMGRS : KadasLatLonToUTM::GridUTM, mRenderGridConfig.cellSize );

      // Draw grid lines
      renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, 3 ) );
      for ( const QPolygonF &zoneLine : zoneLines )
      {
        QPolygonF itemLine;
        for ( const QPointF &point : zoneLine )
        {
          itemLine.append( renderContext()->mapToPixel().transform( crst.transform( point.x(), point.y() ) ).toQPointF() );
        }
        renderContext()->painter()->drawPolyline( itemLine );
      }

      renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, 1.5 ) );
      for ( const QPolygonF &subZoneLine : subZoneLines )
      {
        QPolygonF itemLine;
        for ( const QPointF &point : subZoneLine )
        {
          itemLine.append( renderContext()->mapToPixel().transform( crst.transform( point.x(), point.y() ) ).toQPointF() );
        }
        renderContext()->painter()->drawPolyline( itemLine );
      }

      renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, 1 ) );
      for ( const QPolygonF &gridLine : gridLines )
      {
        QPolygonF itemLine;
        for ( const QPointF &point : gridLine )
        {
          itemLine.append( renderContext()->mapToPixel().transform( crst.transform( point.x(), point.y() ) ).toQPointF() );
        }
        renderContext()->painter()->drawPolyline( itemLine );
      }

      // Draw labels
      bool previewJob = renderContext()->flags() & Qgis::RenderContextFlag::RenderPreviewJob;
      if ( previewJob || mRenderGridConfig.labelingMode != LabelingEnabled )
      {
        return;
      }

      double zoneFontSize = 0;
      double subZoneFontSize = 0;
      double gridLabelSize = mRenderGridConfig.fontSize;
      if ( mapScale > 20000000 )
      {
        zoneFontSize = 0.66 * mRenderGridConfig.fontSize;
      }
      else if ( mapScale > 10000000 )
      {
        zoneFontSize = mRenderGridConfig.fontSize;
      }
      else if ( mapScale > 5000000 )   // Zones only, see KadasLatLonToUTM::computeGrid
      {
        zoneFontSize = 1.33 * mRenderGridConfig.fontSize;
      }
      else if ( mapScale > 500000 )   // Zones and subzones only, see KadasLatLonToUTM::computeGrid
      {
        zoneFontSize = 1.8 * mRenderGridConfig.fontSize;
        subZoneFontSize = mRenderGridConfig.fontSize;
      }
      else
      {
        zoneFontSize = 2 * mRenderGridConfig.fontSize;
        subZoneFontSize = 1.33 * mRenderGridConfig.fontSize;
      }

      QColor bufferColor = ( 0.2126 * mRenderGridConfig.color.red() + 0.7152 * mRenderGridConfig.color.green() + 0.0722 * mRenderGridConfig.color.blue() ) > 128 ? Qt::black : Qt::white;
      double dpiScale = double( renderContext()->painter()->device()->logicalDpiX() ) / qApp->desktop()->logicalDpiX();
      renderContext()->painter()->setBrush( mRenderGridConfig.color );

      QFont font = renderContext()->painter()->font();
      font.setBold( true );

      if ( adaptToScreen )
      {
        font.setPointSizeF( gridLabelSize );
        for ( const KadasLatLonToUTM::GridLabel &gridLabel : gridLabels )
        {
          const QPolygonF &gridLine = gridLines[gridLabel.lineIdx];
          QPointF labelPos = renderContext()->mapToPixel().transform( crst.transform( gridLine.front().x(), gridLine.front().y() ) ).toQPointF();
          const QRectF &visibleRect = screenExtent;
          int i = 1, n = gridLine.size();
          QPointF pp = labelPos;
          if ( gridLabel.horiz && labelPos.x() < visibleRect.x() )
          {
            for ( ; i < n; ++i )
            {
              QPointF pn = renderContext()->mapToPixel().transform( crst.transform( gridLine[i].x(), gridLine[i].y() ) ).toQPointF();
              if ( pn.x() > visibleRect.x() )
              {
                double lambda = ( visibleRect.x() - pp.x() ) / ( pn.x() - pp.x() );
                labelPos = QPointF( pp.x() + lambda * ( pn.x() - pp.x() ), pp.y() + lambda * ( pn.y() - pp.y() ) );
                break;
              }
              pp = pn;
            }
          }
          else if ( !gridLabel.horiz && labelPos.y() > visibleRect.y() + visibleRect.height() )
          {
            for ( ; i < n; ++i )
            {
              QPointF pn = renderContext()->mapToPixel().transform( crst.transform( gridLine[i].x(), gridLine[i].y() ) ).toQPointF();
              if ( pn.y() < visibleRect.y() + visibleRect.height() )
              {
                double lambda = ( visibleRect.y() + visibleRect.height() - pp.y() ) / ( pn.y() - pp.y() );
                labelPos = QPointF( pp.x() + lambda * ( pn.x() - pp.x() ), pp.y() + lambda * ( pn.y() - pp.y() ) );
                break;
              }
              pp = pn;
            }
          }
          if ( i < n )
          {
            drawGridLabel( labelPos, gridLabel.label, font, bufferColor );
          }
        }
      }

      font.setPointSizeF( zoneFontSize * dpiScale );
      QFontMetrics fm( font );
      for ( const KadasLatLonToUTM::ZoneLabel &zoneLabel : zoneLabels )
      {
        const QPointF &pos = zoneLabel.pos;
        const QPointF &maxPos = zoneLabel.maxPos;
        QPointF labelPos = renderContext()->mapToPixel().transform( crst.transform( pos.x(), pos.y() ) ).toQPointF();
        QPointF maxLabelPos = renderContext()->mapToPixel().transform( crst.transform( maxPos.x(), maxPos.y() ) ).toQPointF();
        if ( adaptToScreen )
        {
          adjustZoneLabelPos( labelPos, maxLabelPos, screenExtent );
        }
        labelPos.rx() -= fm.horizontalAdvance( zoneLabel.label );
        labelPos.ry() += fm.height();
        if ( labelPos.x() > maxLabelPos.x() && labelPos.y() < maxLabelPos.y() )
        {
          drawGridLabel( labelPos, zoneLabel.label, font, bufferColor );
        }
      }

      font.setPointSizeF( subZoneFontSize );
      fm = QFontMetrics( font );
      for ( const KadasLatLonToUTM::ZoneLabel &subZoneLabel : zoneSubLabels )
      {
        const QPointF &pos = subZoneLabel.pos;
        const QPointF &maxPos = subZoneLabel.maxPos;
        QPointF labelPos = renderContext()->mapToPixel().transform( crst.transform( pos.x(), pos.y() ) ).toQPointF();
        QPointF maxLabelPos = renderContext()->mapToPixel().transform( crst.transform( maxPos.x(), maxPos.y() ) ).toQPointF();
        if ( adaptToScreen )
        {
          adjustZoneLabelPos( labelPos, maxLabelPos, screenExtent );
        }
        if ( labelPos.x() + fm.horizontalAdvance( subZoneLabel.label ) < maxLabelPos.x() && labelPos.y() - fm.height() > maxLabelPos.y() )
        {
          drawGridLabel( labelPos, subZoneLabel.label, font, bufferColor );
        }
      }
    }
    void drawGridLabel( const QPointF &pos, const QString &text, const QFont &font, const QColor &bufferColor )
    {
      QPainterPath path;
      path.addText( pos, font, text );
      renderContext()->painter()->setPen( QPen( bufferColor, qRound( mRenderGridConfig.fontSize / 8. ) ) );
      renderContext()->painter()->drawPath( path );
      renderContext()->painter()->setPen( Qt::NoPen );
      renderContext()->painter()->drawPath( path );
    }
};

KadasMapGridLayer::KadasMapGridLayer( const QString &name )
  : KadasPluginLayer( layerType(), name )
{
  mValid = true;
}

void KadasMapGridLayer::setup( GridType type, double intervalX, double intervalY, int cellSize )
{
  mGridConfig.gridType = type;
  mGridConfig.intervalX = intervalX;
  mGridConfig.intervalY = intervalY;
  mGridConfig.cellSize = cellSize;
}

KadasMapGridLayer *KadasMapGridLayer::clone() const
{
  KadasMapGridLayer *layer = new KadasMapGridLayer( name() );
  layer->mTransformContext = mTransformContext;
  layer->mOpacity = mOpacity;
  layer->mGridConfig = mGridConfig;
  return layer;
}

QgsMapLayerRenderer *KadasMapGridLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

QList<KadasMapGridLayer::IdentifyResult> KadasMapGridLayer::identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings )
{
  // TODO?
  return QList<KadasMapGridLayer::IdentifyResult>();
}

bool KadasMapGridLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  mOpacity = ( 100. - layerEl.attribute( "transparency" ).toInt() ) / 100.;
  mGridConfig.gridType = static_cast<GridType>( layerEl.attribute( "gridtype" ).toInt() );
  mGridConfig.intervalX = layerEl.attribute( "intervalX" ).toDouble();
  mGridConfig.intervalY = layerEl.attribute( "intervalY" ).toDouble();
  mGridConfig.cellSize = layerEl.attribute( "cellSize" ).toInt();
  mGridConfig.fontSize = layerEl.attribute( "fontSize" ).toInt();
  mGridConfig.color = QgsSymbolLayerUtils::decodeColor( layerEl.attribute( "color" ) );
  mGridConfig.labelingMode = static_cast<LabelingMode>( layerEl.attribute( "labelingMode" ).toInt() );
  return true;
}

bool KadasMapGridLayer::writeXml( QDomNode &layer_node, QDomDocument & /*document*/, const QgsReadWriteContext &context ) const
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  layerEl.setAttribute( "transparency", 100. - mOpacity * 100. );
  layerEl.setAttribute( "gridtype", mGridConfig.gridType );
  layerEl.setAttribute( "intervalX", mGridConfig.intervalX );
  layerEl.setAttribute( "intervalY", mGridConfig.intervalY );
  layerEl.setAttribute( "cellSize", mGridConfig.cellSize );
  layerEl.setAttribute( "fontSize", mGridConfig.fontSize );
  layerEl.setAttribute( "color", QgsSymbolLayerUtils::encodeColor( mGridConfig.color ) );
  layerEl.setAttribute( "labelingMode", static_cast<int>( mGridConfig.labelingMode ) );
  return true;
}

void KadasMapGridLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  menu->addAction( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ), tr( "Edit" ), this, [this, layer]
  {
    mActionMapGridTool->trigger();
  } );
}
