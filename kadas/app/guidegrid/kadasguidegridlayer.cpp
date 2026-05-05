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

#include <QDomDocument>
#include <QDomElement>
#include <QFont>

#include <qgis/qgsannotationlineitem.h>
#include <qgis/qgsannotationpointtextitem.h>
#include <qgis/qgscoordinatetransformcontext.h>
#include <qgis/qgslinestring.h>
#include <qgis/qgslinesymbol.h>
#include <qgis/qgspolygon.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>
#include <qgis/qgstextformat.h>

#include <guidegrid/kadasguidegridlayer.h>

namespace
{
  QString gridLabel( QChar firstChar, int offset )
  {
    if ( firstChar >= '0' && firstChar <= '9' )
    {
      return QString::number( firstChar.digitValue() + offset );
    }
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

  QgsCoordinateTransformContext transformContextForLayer()
  {
    if ( QgsProject::instance() )
      return QgsProject::instance()->transformContext();
    return QgsCoordinateTransformContext();
  }
} // namespace


KadasGuideGridLayer::KadasGuideGridLayer( const QString &name )
  : QgsAnnotationLayer( name, QgsAnnotationLayer::LayerOptions( transformContextForLayer() ) )
{
  // QgsAnnotationLayer is always valid by construction.
}

void KadasGuideGridLayer::setup( const QgsRectangle &gridRect, int cols, int rows, const QgsCoordinateReferenceSystem &crs, bool colSizeLocked, bool rowSizeLocked )
{
  mGridConfig.gridRect = gridRect;
  mGridConfig.cols = cols;
  mGridConfig.rows = rows;
  mGridConfig.colSizeLocked = colSizeLocked;
  mGridConfig.rowSizeLocked = rowSizeLocked;
  setCrs( crs, false );
  regenerate();
}

KadasGuideGridLayer *KadasGuideGridLayer::clone() const
{
  KadasGuideGridLayer *layer = new KadasGuideGridLayer( name() );
  layer->setOpacity( opacity() );
  layer->setCrs( crs(), false );
  layer->mGridConfig = mGridConfig;
  layer->regenerate();
  return layer;
}

QList<KadasPluginLayer::IdentifyResult> KadasGuideGridLayer::identify( const QgsPointXY &mapPos, const QgsMapSettings &mapSettings )
{
  QgsCoordinateTransform crst( mapSettings.destinationCrs(), crs(), QgsProject::instance() ? QgsProject::instance()->transformContext() : QgsCoordinateTransformContext() );
  QgsPointXY pos = crst.transform( mapPos );

  if ( mGridConfig.cols <= 0 || mGridConfig.rows <= 0 )
    return {};

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
    << QgsPoint( mGridConfig.gridRect.xMinimum() + i * colWidth, mGridConfig.gridRect.yMaximum() - j * rowHeight )
  );
  bbox->setExteriorRing( ring );
  QMap<QString, QVariant> attrs;

  QString text = tr( "Cell %1, %2" ).arg( gridLabel( mGridConfig.rowChar, j ) ).arg( gridLabel( mGridConfig.colChar, i ) );
  if ( mGridConfig.quadrantLabeling != DontLabelQuadrants )
  {
    bool left = pos.x() <= mGridConfig.gridRect.xMinimum() + ( i + 0.5 ) * colWidth;
    bool top = pos.y() >= mGridConfig.gridRect.yMaximum() - ( j + 0.5 ) * rowHeight;
    QString quadrantLabel;
    if ( left )
      quadrantLabel = top ? "A" : "D";
    else
      quadrantLabel = top ? "B" : "C";
    text += tr( " (Quadrant %1)" ).arg( quadrantLabel );
  }

  return QList<KadasPluginLayer::IdentifyResult>() << KadasPluginLayer::IdentifyResult( text, attrs, QgsGeometry( bbox ) );
}

bool KadasGuideGridLayer::readXml( const QDomNode &layer_node, QgsReadWriteContext &context )
{
  // Let the stock annotation layer load its own state (CRS, opacity, items).
  QgsAnnotationLayer::readXml( layer_node, context );

  // Then overlay our config from the dedicated child element. Items are
  // regenerated from the config to keep the on-disk and in-memory views
  // consistent (and to be robust against partially-written files).
  const QDomElement layerEl = layer_node.toElement();
  const QDomElement cfgEl = layerEl.firstChildElement( QStringLiteral( "KadasGuideGrid" ) );
  if ( cfgEl.isNull() )
    return true;

  mGridConfig.gridRect.setXMinimum( cfgEl.attribute( "xmin" ).toDouble() );
  mGridConfig.gridRect.setYMinimum( cfgEl.attribute( "ymin" ).toDouble() );
  mGridConfig.gridRect.setXMaximum( cfgEl.attribute( "xmax" ).toDouble() );
  mGridConfig.gridRect.setYMaximum( cfgEl.attribute( "ymax" ).toDouble() );
  mGridConfig.cols = cfgEl.attribute( "cols" ).toInt();
  mGridConfig.rows = cfgEl.attribute( "rows" ).toInt();
  mGridConfig.colSizeLocked = cfgEl.attribute( "colSizeLocked", "0" ).toInt();
  mGridConfig.rowSizeLocked = cfgEl.attribute( "rowSizeLocked", "0" ).toInt();
  mGridConfig.fontSize = cfgEl.attribute( "fontSize" ).toInt();
  mGridConfig.color = QgsSymbolLayerUtils::decodeColor( cfgEl.attribute( "color" ) );
  mGridConfig.lineWidth = cfgEl.attribute( "lineWidth", "1" ).toInt();
  mGridConfig.rowChar = !cfgEl.attribute( "rowChar" ).isEmpty() ? cfgEl.attribute( "rowChar" ).at( 0 ) : 'A';
  mGridConfig.colChar = !cfgEl.attribute( "colChar" ).isEmpty() ? cfgEl.attribute( "colChar" ).at( 0 ) : '1';
  mGridConfig.labelingPos = static_cast<LabelingPos>( cfgEl.attribute( "labelingPos" ).toInt() );
  mGridConfig.quadrantLabeling = static_cast<QuadrantLabeling>( cfgEl.attribute( "quadrantLabeling" ).toInt() );

  // Items in the project file are authoritative for what QGIS draws; but we
  // still rebuild from config so the in-memory state matches what a re-save
  // would emit. (Cheap, and keeps Kadas/QGIS visually identical.)
  regenerate();
  return true;
}

bool KadasGuideGridLayer::writeXml( QDomNode &layer_node, QDomDocument &doc, const QgsReadWriteContext &context ) const
{
  // Let QgsAnnotationLayer serialize its standard fields (and our items).
  if ( !QgsAnnotationLayer::writeXml( layer_node, doc, context ) )
    return false;

  QDomElement layerEl = layer_node.toElement();
  QDomElement cfgEl = doc.createElement( QStringLiteral( "KadasGuideGrid" ) );
  cfgEl.setAttribute( "xmin", mGridConfig.gridRect.xMinimum() );
  cfgEl.setAttribute( "ymin", mGridConfig.gridRect.yMinimum() );
  cfgEl.setAttribute( "xmax", mGridConfig.gridRect.xMaximum() );
  cfgEl.setAttribute( "ymax", mGridConfig.gridRect.yMaximum() );
  cfgEl.setAttribute( "cols", mGridConfig.cols );
  cfgEl.setAttribute( "rows", mGridConfig.rows );
  cfgEl.setAttribute( "colSizeLocked", mGridConfig.colSizeLocked ? 1 : 0 );
  cfgEl.setAttribute( "rowSizeLocked", mGridConfig.rowSizeLocked ? 1 : 0 );
  cfgEl.setAttribute( "fontSize", mGridConfig.fontSize );
  cfgEl.setAttribute( "color", QgsSymbolLayerUtils::encodeColor( mGridConfig.color ) );
  cfgEl.setAttribute( "lineWidth", mGridConfig.lineWidth );
  cfgEl.setAttribute( "colChar", QString( mGridConfig.colChar ) );
  cfgEl.setAttribute( "rowChar", QString( mGridConfig.rowChar ) );
  cfgEl.setAttribute( "labelingPos", static_cast<int>( mGridConfig.labelingPos ) );
  cfgEl.setAttribute( "quadrantLabeling", static_cast<int>( mGridConfig.quadrantLabeling ) );
  layerEl.appendChild( cfgEl );
  return true;
}

void KadasGuideGridLayer::regenerate()
{
  clear();

  if ( mGridConfig.cols <= 0 || mGridConfig.rows <= 0 || mGridConfig.gridRect.isEmpty() )
  {
    triggerRepaint();
    return;
  }

  const QgsRectangle &r = mGridConfig.gridRect;
  const double dx = r.width() / mGridConfig.cols;
  const double dy = r.height() / mGridConfig.rows;

  // Helpers for symbol/text styling.
  auto makeLineSymbol = [&]( bool dashed ) -> QgsLineSymbol * {
    auto sym = QgsLineSymbol::createSimple( {} );
    sym->setColor( mGridConfig.color );
    sym->setWidth( mGridConfig.lineWidth );
    sym->setWidthUnit( Qgis::RenderUnit::Pixels );
    if ( dashed )
    {
      QVariantMap props;
      props.insert( QStringLiteral( "line_color" ), QgsSymbolLayerUtils::encodeColor( mGridConfig.color ) );
      props.insert( QStringLiteral( "line_width" ), QString::number( mGridConfig.lineWidth ) );
      props.insert( QStringLiteral( "line_width_unit" ), QStringLiteral( "Pixel" ) );
      props.insert( QStringLiteral( "line_style" ), QStringLiteral( "dash" ) );
      auto dashedSym = QgsLineSymbol::createSimple( props );
      return dashedSym.release();
    }
    return sym.release();
  };

  auto makeTextFormat = [&]( double fontPx ) {
    QgsTextFormat fmt;
    QFont f = fmt.font();
    f.setPixelSize( static_cast<int>( fontPx ) );
    fmt.setFont( f );
    fmt.setSize( fontPx );
    fmt.setSizeUnit( Qgis::RenderUnit::Pixels );
    fmt.setColor( mGridConfig.color );
    QgsTextBufferSettings buf = fmt.buffer();
    const bool darkText = ( 0.2126 * mGridConfig.color.red() + 0.7152 * mGridConfig.color.green() + 0.0722 * mGridConfig.color.blue() ) > 128;
    buf.setColor( darkText ? Qt::black : Qt::white );
    buf.setSize( std::max( 1.0, mGridConfig.fontSize / 8.0 ) );
    buf.setSizeUnit( Qgis::RenderUnit::Pixels );
    buf.setEnabled( true );
    fmt.setBuffer( buf );
    return fmt;
  };

  const QgsTextFormat textFmt = makeTextFormat( mGridConfig.fontSize );
  const QgsTextFormat smallTextFmt = makeTextFormat( 0.5 * mGridConfig.fontSize );
  const double labelInsetPx = mGridConfig.fontSize; // visual margin from the grid edge

  // Approximate map-units-per-pixel for label insets; precise screen-edge
  // placement is intentionally not done here — items are static.
  const double mupX = dx / std::max( 1, mGridConfig.cols * 50 ); // tiny inset relative to cell
  const double mupY = dy / std::max( 1, mGridConfig.rows * 50 );

  // --- Vertical lines ---
  for ( int col = 0; col <= mGridConfig.cols; ++col )
  {
    const double x = r.xMinimum() + col * dx;
    auto *line = new QgsLineString( QgsPointSequence() << QgsPoint( x, r.yMinimum() ) << QgsPoint( x, r.yMaximum() ) );
    auto *item = new QgsAnnotationLineItem( line );
    item->setSymbol( makeLineSymbol( false ) );
    addItem( item );
  }

  // --- Horizontal lines ---
  for ( int row = 0; row <= mGridConfig.rows; ++row )
  {
    const double y = r.yMaximum() - row * dy;
    auto *line = new QgsLineString( QgsPointSequence() << QgsPoint( r.xMinimum(), y ) << QgsPoint( r.xMaximum(), y ) );
    auto *item = new QgsAnnotationLineItem( line );
    item->setSymbol( makeLineSymbol( false ) );
    addItem( item );
  }

  // --- Column labels (top + bottom of the grid rect) ---
  for ( int col = 0; col < mGridConfig.cols; ++col )
  {
    const QString label = gridLabel( mGridConfig.colChar, col );
    const double cx = r.xMinimum() + ( col + 0.5 ) * dx;
    // Top
    {
      const double cy = mGridConfig.labelingPos == LabelsOutside ? r.yMaximum() + 0.5 * dy : r.yMaximum() - 0.5 * dy;
      auto *txt = new QgsAnnotationPointTextItem( label, QgsPointXY( cx, cy ) );
      txt->setFormat( textFmt );
      txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
      addItem( txt );
    }
    // Bottom
    {
      const double cy = mGridConfig.labelingPos == LabelsOutside ? r.yMinimum() - 0.5 * dy : r.yMinimum() + 0.5 * dy;
      auto *txt = new QgsAnnotationPointTextItem( label, QgsPointXY( cx, cy ) );
      txt->setFormat( textFmt );
      txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
      addItem( txt );
    }
  }

  // --- Row labels (left + right of the grid rect) ---
  for ( int row = 0; row < mGridConfig.rows; ++row )
  {
    const QString label = gridLabel( mGridConfig.rowChar, row );
    const double cy = r.yMaximum() - ( row + 0.5 ) * dy;
    // Left
    {
      const double cx = mGridConfig.labelingPos == LabelsOutside ? r.xMinimum() - 0.5 * dx : r.xMinimum() + 0.5 * dx;
      auto *txt = new QgsAnnotationPointTextItem( label, QgsPointXY( cx, cy ) );
      txt->setFormat( textFmt );
      txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
      addItem( txt );
    }
    // Right
    {
      const double cx = mGridConfig.labelingPos == LabelsOutside ? r.xMaximum() + 0.5 * dx : r.xMaximum() - 0.5 * dx;
      auto *txt = new QgsAnnotationPointTextItem( label, QgsPointXY( cx, cy ) );
      txt->setFormat( textFmt );
      txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
      addItem( txt );
    }
  }

  // --- Quadrant subgrid (mid-lines + ABCD micro-labels) ---
  if ( mGridConfig.quadrantLabeling != DontLabelQuadrants )
  {
    const int qCols = mGridConfig.quadrantLabeling == LabelOneQuadrant ? 1 : mGridConfig.cols;
    const int qRows = mGridConfig.quadrantLabeling == LabelOneQuadrant ? 1 : mGridConfig.rows;

    // Vertical mid-lines (one per quadrant column, splitting it horizontally in half)
    for ( int col = 0; col < qCols; ++col )
    {
      const double x = r.xMinimum() + ( col + 0.5 ) * dx;
      const double yMin = r.yMaximum() - qRows * dy;
      auto *line = new QgsLineString( QgsPointSequence() << QgsPoint( x, yMin ) << QgsPoint( x, r.yMaximum() ) );
      auto *item = new QgsAnnotationLineItem( line );
      item->setSymbol( makeLineSymbol( true ) );
      addItem( item );
    }

    // Horizontal mid-lines
    for ( int row = 0; row < qRows; ++row )
    {
      const double y = r.yMaximum() - ( row + 0.5 ) * dy;
      const double xMax = r.xMinimum() + qCols * dx;
      auto *line = new QgsLineString( QgsPointSequence() << QgsPoint( r.xMinimum(), y ) << QgsPoint( xMax, y ) );
      auto *item = new QgsAnnotationLineItem( line );
      item->setSymbol( makeLineSymbol( true ) );
      addItem( item );
    }

    // ABCD micro-labels at the four corners of each quadrant cell.
    const double insetX = 0.15 * dx;
    const double insetY = 0.15 * dy;
    for ( int col = 0; col < qCols; ++col )
    {
      const double xL = r.xMinimum() + col * dx;
      const double xR = r.xMinimum() + ( col + 1 ) * dx;
      for ( int row = 0; row < qRows; ++row )
      {
        const double yT = r.yMaximum() - row * dy;
        const double yB = r.yMaximum() - ( row + 1 ) * dy;
        struct
        {
            const char *label;
            double x;
            double y;
        } corners[4] = {
          { "A", xL + insetX, yT - insetY },
          { "B", xR - insetX, yT - insetY },
          { "C", xR - insetX, yB + insetY },
          { "D", xL + insetX, yB + insetY },
        };
        for ( const auto &c : corners )
        {
          auto *txt = new QgsAnnotationPointTextItem( QString::fromLatin1( c.label ), QgsPointXY( c.x, c.y ) );
          txt->setFormat( smallTextFmt );
          txt->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
          addItem( txt );
        }
      }
    }
  }

  Q_UNUSED( mupX );
  Q_UNUSED( mupY );
  Q_UNUSED( labelInsetPx );

  triggerRepaint();
}
