/***************************************************************************
    kadasguidegridlayer.cpp
    -----------------------
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

#include <qgis/qgsapplication.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgsmaplayerrenderer.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgssymbollayerutils.h>

#include <guidegrid/kadasguidegridlayer.h>

static QString gridLabel( QChar firstChar, int offset )
{
  if ( firstChar >= '0' && firstChar <= '9' )
  {
    return QString::number( firstChar.digitValue() + offset );
  }
  else
  {
    QString label;
    offset += firstChar.toLatin1() - 'A' + 1;
    do
    {
      offset -= 1;
      int res = offset % 26;
      label.prepend( QChar( 'A' + res ) );
      offset /= 26;
    } while ( offset > 0 );
    return label;
  }
}

class KadasGuideGridLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( KadasGuideGridLayer *layer, QgsRenderContext &rendererContext )
      : QgsMapLayerRenderer( layer->id(), &rendererContext )
      , mRenderGridConfig( layer->mGridConfig )
      , mRenderOpacity( layer->opacity() )
    {}

    bool render() override
    {
      if ( mRenderGridConfig.rows == 0 || mRenderGridConfig.cols == 0 )
      {
        return true;
      }
      bool previewJob = renderContext()->flags() & Qgis::RenderContextFlag::RenderPreviewJob;

      renderContext()->painter()->save();
      renderContext()->painter()->setOpacity( mRenderOpacity );
      renderContext()->painter()->setCompositionMode( QPainter::CompositionMode_Source );
      renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, mRenderGridConfig.lineWidth ) );
      renderContext()->painter()->setBrush( mRenderGridConfig.color );

      const QVariantMap &flags = renderContext()->customProperties();
      bool adaptLabelsToScreen = !( flags["globe"].toBool() || flags["kml"].toBool() );

      QColor bufferColor = ( 0.2126 * mRenderGridConfig.color.red() + 0.7152 * mRenderGridConfig.color.green() + 0.0722 * mRenderGridConfig.color.blue() ) > 128 ? Qt::black : Qt::white;
      double dpiScale = double( renderContext()->painter()->device()->logicalDpiX() ) / qApp->desktop()->logicalDpiX();

      QFont smallFont;
      smallFont.setPixelSize( 0.5 * mRenderGridConfig.fontSize * dpiScale );
      QFontMetrics smallFontMetrics( smallFont );

      QFont font;
      font.setPixelSize( mRenderGridConfig.fontSize * dpiScale );
      QFontMetrics fontMetrics( font );

      const int labelBoxSize = fontMetrics.height();
      const int smallLabelBoxSize = smallFontMetrics.height();

      QgsCoordinateTransform crst = renderContext()->coordinateTransform();
      const QgsMapToPixel &mapToPixel = renderContext()->mapToPixel();
      const QgsRectangle &gridRect = mRenderGridConfig.gridRect;
      QgsPoint pTL = QgsPoint( renderContext()->mapExtent().xMinimum(), renderContext()->mapExtent().yMaximum() );
      QgsPoint pBR = QgsPoint( renderContext()->mapExtent().xMaximum(), renderContext()->mapExtent().yMinimum() );
      QPointF screenTL = mapToPixel.transform( crst.transform( pTL ) ).toQPointF();
      QPointF screenBR = mapToPixel.transform( crst.transform( pBR ) ).toQPointF();
      QRectF screenRect( screenTL, screenBR );

      double ix = gridRect.width() / mRenderGridConfig.cols;
      double iy = gridRect.height() / mRenderGridConfig.rows;

      // Draw vertical lines
      QPolygonF vLine1 = vScreenLine( gridRect.xMinimum(), iy );
      {
        QPainterPath path;
        path.addPolygon( vLine1 );
        renderContext()->painter()->drawPath( path );
      }
      double sy1 = adaptLabelsToScreen ? std::max( vLine1.first().y(), screenRect.top() ) : vLine1.first().y();
      double sy2 = adaptLabelsToScreen ? std::min( vLine1.last().y(), screenRect.bottom() ) : vLine1.last().y();
      QuadrantLabeling quadrantLabeling = mRenderGridConfig.quadrantLabeling;
      for ( int col = 1; col <= mRenderGridConfig.cols; ++col )
      {
        double x2 = gridRect.xMinimum() + col * ix;
        QPolygonF vLine2 = vScreenLine( x2, iy );
        QPainterPath path;
        path.addPolygon( vLine2 );
        renderContext()->painter()->drawPath( path );

        if ( !previewJob )
        {
          double sx1 = vLine1.first().x();
          double sx2 = vLine2.first().x();
          QString label = gridLabel( mRenderGridConfig.colChar, col - 1 );
          if ( mRenderGridConfig.labelingPos == LabelsOutside && vLine1.first().y() - labelBoxSize > screenRect.top() )
          {
            drawGridLabel( 0.5 * ( sx1 + sx2 ), sy1 - 0.5 * labelBoxSize, label, font, fontMetrics, bufferColor );
          }
          else if ( sy1 < vLine1.last().y() - 2 * labelBoxSize )
          {
            drawGridLabel( 0.5 * ( sx1 + sx2 ), sy1 + 0.5 * labelBoxSize, label, font, fontMetrics, bufferColor );
          }
          if ( mRenderGridConfig.labelingPos == LabelsOutside && vLine1.last().y() + labelBoxSize < screenRect.bottom() )
          {
            drawGridLabel( 0.5 * ( sx1 + sx2 ), sy2 + 0.5 * labelBoxSize, label, font, fontMetrics, bufferColor );
          }
          else if ( sy2 > vLine1.first().y() + 2 * labelBoxSize )
          {
            drawGridLabel( 0.5 * ( sx1 + sx2 ), sy2 - 0.5 * labelBoxSize, label, font, fontMetrics, bufferColor );
          }
        }

        if ( quadrantLabeling != DontLabelQuadrants )
        {
          renderContext()->painter()->save();
          renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, mRenderGridConfig.lineWidth, Qt::DashLine ) );
          QSizeF smallLabelBox( smallLabelBoxSize, smallLabelBoxSize );
          QPolygonF vLineMid;
          for ( int i = 0, n = vLine1.size(); i < n; ++i )
          {
            vLineMid.append( 0.5 * ( vLine1.at( i ) + vLine2.at( i ) ) );
            if ( i < n - 1 && 0.4 * qAbs( vLine1.at( i ).x() - vLine2.at( i ).x() ) > smallLabelBoxSize )
            {
              drawGridLabel( vLine1.at( i ).x() + 0.5 * smallLabelBoxSize, vLine1.at( i ).y() + 0.5 * smallLabelBoxSize, "A", smallFont, smallFontMetrics, bufferColor );
              drawGridLabel( vLine2.at( i ).x() - 0.5 * smallLabelBoxSize, vLine2.at( i ).y() + 0.5 * smallLabelBoxSize, "B", smallFont, smallFontMetrics, bufferColor );
              drawGridLabel( vLine1.at( i + 1 ).x() + 0.5 * smallLabelBoxSize, vLine1.at( i + 1 ).y() - 0.5 * smallLabelBoxSize, "D", smallFont, smallFontMetrics, bufferColor );
              drawGridLabel( vLine2.at( i + 1 ).x() - 0.5 * smallLabelBoxSize, vLine2.at( i + 1 ).y() - 0.5 * smallLabelBoxSize, "C", smallFont, smallFontMetrics, bufferColor );
            }
            if ( quadrantLabeling == LabelOneQuadrant )
            {
              vLineMid.append( 0.5 * ( vLine1.at( i + 1 ) + vLine2.at( i + 1 ) ) );
              quadrantLabeling = DontLabelQuadrants;
              break;
            }
          }
          QPainterPath path;
          path.addPolygon( vLineMid );
          renderContext()->painter()->drawPath( path );
          renderContext()->painter()->restore();
        }

        vLine1 = vLine2;
      }

      // Draw horizontal lines
      QPolygonF hLine1 = hScreenLine( gridRect.yMaximum(), ix );
      {
        QPainterPath path;
        path.addPolygon( hLine1 );
        renderContext()->painter()->drawPath( path );
      }
      double sx1 = adaptLabelsToScreen ? std::max( hLine1.first().x(), screenRect.left() ) : hLine1.first().x();
      double sx2 = adaptLabelsToScreen ? std::min( hLine1.last().x(), screenRect.right() ) : hLine1.last().x();
      quadrantLabeling = mRenderGridConfig.quadrantLabeling;
      for ( int row = 1; row <= mRenderGridConfig.rows; ++row )
      {
        double y = gridRect.yMaximum() - row * iy;
        QPolygonF hLine2 = hScreenLine( y, ix );
        QPainterPath path;
        path.addPolygon( hLine2 );
        renderContext()->painter()->drawPath( path );

        if ( !previewJob )
        {
          double sy1 = hLine1.first().y();
          double sy2 = hLine2.first().y();
          QString label = gridLabel( mRenderGridConfig.rowChar, row - 1 );
          if ( mRenderGridConfig.labelingPos == LabelsOutside && hLine1.first().x() - labelBoxSize > screenRect.left() )
          {
            drawGridLabel( sx1 - 0.5 * labelBoxSize, 0.5 * ( sy1 + sy2 ), label, font, fontMetrics, bufferColor );
          }
          else if ( sx1 < vLine1.last().x() - 2 * labelBoxSize )
          {
            drawGridLabel( sx1 + 0.5 * labelBoxSize, 0.5 * ( sy1 + sy2 ), label, font, fontMetrics, bufferColor );
          }
          if ( mRenderGridConfig.labelingPos == LabelsOutside && hLine1.last().x() + labelBoxSize < screenRect.right() )
          {
            drawGridLabel( sx2 + 0.5 * labelBoxSize, 0.5 * ( sy1 + sy2 ), label, font, fontMetrics, bufferColor );
          }
          else if ( sx2 > hLine1.first().x() + 2 * labelBoxSize )
          {
            drawGridLabel( sx2 - 0.5 * labelBoxSize, 0.5 * ( sy1 + sy2 ), label, font, fontMetrics, bufferColor );
          }
        }

        if ( quadrantLabeling != DontLabelQuadrants )
        {
          renderContext()->painter()->save();
          renderContext()->painter()->setPen( QPen( mRenderGridConfig.color, mRenderGridConfig.lineWidth, Qt::DashLine ) );
          QPolygonF hLineMid;
          if ( quadrantLabeling == LabelOneQuadrant )
          {
            hLineMid.append( 0.5 * ( hLine1.at( 0 ) + hLine2.at( 0 ) ) );
            hLineMid.append( 0.5 * ( hLine1.at( 1 ) + hLine2.at( 1 ) ) );
            quadrantLabeling = DontLabelQuadrants;
          }
          else
          {
            for ( int i = 0, n = hLine1.size(); i < n; ++i )
            {
              hLineMid.append( 0.5 * ( hLine1.at( i ) + hLine2.at( i ) ) );
            }
          }
          QPainterPath path;
          path.addPolygon( hLineMid );
          renderContext()->painter()->drawPath( path );
          renderContext()->painter()->restore();
        }

        hLine1 = hLine2;
      }
      renderContext()->painter()->restore();
      return true;
    }
    void drawGridLabel( double x, double y, const QString &text, const QFont &font, const QFontMetrics &metrics, const QColor &bufferColor )
    {
      QPainterPath path;
      x -= 0.5 * metrics.horizontalAdvance( text );
      y = y - metrics.descent() + 0.5 * metrics.height();
      path.addText( x, y, font, text );
      renderContext()->painter()->save();
      renderContext()->painter()->setPen( QPen( bufferColor, qRound( mRenderGridConfig.fontSize / 8. ) ) );
      renderContext()->painter()->drawPath( path );
      renderContext()->painter()->setPen( Qt::NoPen );
      renderContext()->painter()->drawPath( path );
      renderContext()->painter()->restore();
    }

  private:
    KadasGuideGridLayer::GridConfig mRenderGridConfig;
    double mRenderOpacity = 1.0;

    QPolygonF vScreenLine( double x, double iy ) const
    {
      QgsCoordinateTransform crst = renderContext()->coordinateTransform();
      const QgsMapToPixel &mapToPixel = renderContext()->mapToPixel();
      const QgsRectangle &gridRect = mRenderGridConfig.gridRect;
      QPolygonF screenPoints;
      for ( int row = 0; row <= mRenderGridConfig.rows; ++row )
      {
        QgsPoint p( x, gridRect.yMaximum() - row * iy );
        QPointF screenPoint = mapToPixel.transform( crst.transform( p ) ).toQPointF();
        screenPoints.append( screenPoint );
      }
      return screenPoints;
    }
    QPolygonF hScreenLine( double y, double ix ) const
    {
      QgsCoordinateTransform crst = renderContext()->coordinateTransform();
      const QgsMapToPixel &mapToPixel = renderContext()->mapToPixel();
      const QgsRectangle &gridRect = mRenderGridConfig.gridRect;
      QPolygonF screenPoints;
      for ( int col = 0; col <= mRenderGridConfig.cols; ++col )
      {
        QgsPoint p( gridRect.xMinimum() + col * ix, y );
        QPointF screenPoint = mapToPixel.transform( crst.transform( p ) ).toQPointF();
        screenPoints.append( screenPoint );
      }
      return screenPoints;
    }
};

KadasGuideGridLayer::KadasGuideGridLayer( const QString &name )
  : KadasPluginLayer( layerType(), name )
{
  mValid = true;
}

void KadasGuideGridLayer::setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs, bool colSizeLocked, bool rowSizeLocked )
{
  mGridConfig.gridRect = gridRect;
  mGridConfig.cols = cols;
  mGridConfig.rows = rows;
  mGridConfig.colSizeLocked = colSizeLocked;
  mGridConfig.rowSizeLocked = rowSizeLocked;
  setCrs( crs, false );
}

KadasGuideGridLayer *KadasGuideGridLayer::clone() const
{
  KadasGuideGridLayer *layer = new KadasGuideGridLayer( name() );
  layer->mTransformContext = mTransformContext;
  layer->mOpacity = mOpacity;
  layer->mGridConfig = mGridConfig;
  return layer;
}

QgsMapLayerRenderer *KadasGuideGridLayer::createMapRenderer( QgsRenderContext &rendererContext )
{
  return new Renderer( this, rendererContext );
}

bool KadasGuideGridLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  QDomElement layerEl = layer_node.toElement();
  mLayerName = layerEl.attribute( "title" );
  mOpacity = ( 100. - layerEl.attribute( "transparency" ).toInt() ) / 100.;
  mGridConfig.gridRect.setXMinimum( layerEl.attribute( "xmin" ).toDouble() );
  mGridConfig.gridRect.setYMinimum( layerEl.attribute( "ymin" ).toDouble() );
  mGridConfig.gridRect.setXMaximum( layerEl.attribute( "xmax" ).toDouble() );
  mGridConfig.gridRect.setYMaximum( layerEl.attribute( "ymax" ).toDouble() );
  mGridConfig.cols = layerEl.attribute( "cols" ).toInt();
  mGridConfig.rows = layerEl.attribute( "rows" ).toInt();
  mGridConfig.colSizeLocked = layerEl.attribute( "colSizeLocked", "0" ).toInt();
  mGridConfig.rowSizeLocked = layerEl.attribute( "rowSizeLocked", "0" ).toInt();
  mGridConfig.fontSize = layerEl.attribute( "fontSize" ).toInt();
  mGridConfig.color = QgsSymbolLayerUtils::decodeColor( layerEl.attribute( "color" ) );
  mGridConfig.lineWidth = layerEl.attribute( "lineWidth", "1" ).toInt();
  mGridConfig.rowChar = layerEl.attribute( "rowChar" ).size() > 0 ? layerEl.attribute( "rowChar" ).at( 0 ) : 'A';
  mGridConfig.colChar = layerEl.attribute( "colChar" ).size() > 0 ? layerEl.attribute( "colChar" ).at( 0 ) : '1';
  mGridConfig.labelingPos = static_cast<LabelingPos>( layerEl.attribute( "labelingPos" ).toInt() );
  if ( layerEl.hasAttribute( "quadrantLabeling" ) )
  {
    mGridConfig.quadrantLabeling = static_cast<QuadrantLabeling>( layerEl.attribute( "quadrantLabeling" ).toInt() );
  }
  else
  {
    mGridConfig.quadrantLabeling = layerEl.attribute( "labelQuadrans" ).toInt() == 1 ? LabelAllQuadrants : DontLabelQuadrants;
  }
  if ( !layerEl.attribute( "labellingMode" ).isEmpty() )
  {
    // Compatibility
    enum LabellingMode
    {
      LABEL_A_1,
      LABEL_1_A
    };
    int labellingMode = static_cast<LabellingMode>( layerEl.attribute( "labellingMode" ).toInt() );
    if ( labellingMode == LABEL_A_1 )
    {
      mGridConfig.rowChar = 'A';
      mGridConfig.colChar = '1';
    }
    else
    {
      mGridConfig.rowChar = '1';
      mGridConfig.colChar = 'A';
    }
  }

  setCrs( QgsCoordinateReferenceSystem( layerEl.attribute( "crs" ) ) );
  return true;
}

QList<KadasGuideGridLayer::IdentifyResult> KadasGuideGridLayer::identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings )
{
  QgsCoordinateTransform crst( mapSettings.destinationCrs(), crs(), mTransformContext );
  QgsPointXY pos = crst.transform( mapPos );

  double colWidth = ( mGridConfig.gridRect.xMaximum() - mGridConfig.gridRect.xMinimum() ) / mGridConfig.cols;
  double rowHeight = ( mGridConfig.gridRect.yMaximum() - mGridConfig.gridRect.yMinimum() ) / mGridConfig.rows;

  int i = std::floor( ( pos.x() - mGridConfig.gridRect.xMinimum() ) / colWidth );
  int j = std::floor( ( mGridConfig.gridRect.yMaximum() - pos.y() ) / rowHeight );

  QgsPolygon *bbox = new QgsPolygon();
  QgsLineString *ring = new QgsLineString();
  ring->setPoints(
    QgsPointSequence()
    << QgsPoint( mGridConfig.gridRect.xMinimum() + i * colWidth, mGridConfig.gridRect.yMaximum() - j * rowHeight )
    << QgsPoint( mGridConfig.gridRect.xMinimum() + ( i + 1 ) * colWidth, mGridConfig.gridRect.yMaximum() - j * rowHeight )
    << QgsPoint( mGridConfig.gridRect.xMinimum() + ( i + 1 ) * colWidth, mGridConfig.gridRect.yMaximum() - ( j + 1 ) * rowHeight )
    << QgsPoint( mGridConfig.gridRect.xMinimum() + i * colWidth, mGridConfig.gridRect.yMaximum() - ( j + 1 ) * rowHeight )
    << QgsPoint( mGridConfig.gridRect.xMinimum() + i * colWidth, mGridConfig.gridRect.yMaximum() - j * rowHeight ) );
  bbox->setExteriorRing( ring );
  QMap<QString, QVariant> attrs;

  QString text = tr( "Cell %1, %2" ).arg( gridLabel( mGridConfig.rowChar, j ) ).arg( gridLabel( mGridConfig.colChar, i ) );
  if ( mGridConfig.quadrantLabeling != DontLabelQuadrants )
  {
    bool left = pos.x() <= mGridConfig.gridRect.xMinimum() + ( i + 0.5 ) * colWidth;
    bool top = pos.y() >= mGridConfig.gridRect.yMaximum() - ( j + 0.5 ) * rowHeight;
    QString quadrantLabel;
    if ( left )
    {
      quadrantLabel = top ? "A" : "D";
    }
    else
    {
      quadrantLabel = top ? "B" : "C";
    }
    text += tr( " (Quadrant %1)" ).arg( quadrantLabel );
  }

  return QList<IdentifyResult>() << IdentifyResult( text, attrs, QgsGeometry( bbox ) );
}

bool KadasGuideGridLayer::writeXml( QDomNode &layer_node, QDomDocument & /*document*/, const QgsReadWriteContext &context ) const
{
  QDomElement layerEl = layer_node.toElement();
  layerEl.setAttribute( "type", "plugin" );
  layerEl.setAttribute( "name", layerTypeKey() );
  layerEl.setAttribute( "title", name() );
  layerEl.setAttribute( "transparency", 100. - mOpacity * 100. );
  layerEl.setAttribute( "xmin", mGridConfig.gridRect.xMinimum() );
  layerEl.setAttribute( "ymin", mGridConfig.gridRect.yMinimum() );
  layerEl.setAttribute( "xmax", mGridConfig.gridRect.xMaximum() );
  layerEl.setAttribute( "ymax", mGridConfig.gridRect.yMaximum() );
  layerEl.setAttribute( "cols", mGridConfig.cols );
  layerEl.setAttribute( "rows", mGridConfig.rows );
  layerEl.setAttribute( "colSizeLocked", mGridConfig.colSizeLocked ? 1 : 0 );
  layerEl.setAttribute( "rowSizeLocked", mGridConfig.rowSizeLocked ? 1 : 0 );
  layerEl.setAttribute( "crs", crs().authid() );
  layerEl.setAttribute( "fontSize", mGridConfig.fontSize );
  layerEl.setAttribute( "color", QgsSymbolLayerUtils::encodeColor( mGridConfig.color ) );
  layerEl.setAttribute( "lineWidth", mGridConfig.lineWidth );
  layerEl.setAttribute( "colChar", QString( mGridConfig.colChar ) );
  layerEl.setAttribute( "rowChar", QString( mGridConfig.rowChar ) );
  layerEl.setAttribute( "labelingPos", mGridConfig.labelingPos );
  layerEl.setAttribute( "quadrantLabeling", mGridConfig.quadrantLabeling );
  return true;
}

void KadasGuideGridLayerType::addLayerTreeMenuActions( QMenu *menu, QgsPluginLayer *layer ) const
{
  menu->addAction( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ), tr( "Edit" ), this, [this, layer] {
    mActionGuideGridTool->trigger();
  } );
}
