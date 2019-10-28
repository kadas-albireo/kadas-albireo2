/***************************************************************************
    kdasmilxitem.cpp
    ----------------
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

#include <QDesktopWidget>
#include <QMenu>
#include <QVector2D>

#include <qgis/qgsdistancearea.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmaptopixel.h>
#include <qgis/qgsproject.h>

#include <quazip5/quazipfile.h>

#include <kadas/app/kadasapplication.h>
#include <kadas/app/milx/kadasmilxitem.h>


KADAS_REGISTER_MAP_ITEM( KadasMilxItem, []( const QgsCoordinateReferenceSystem &crs )  { return new KadasMilxItem(); } );

QJsonObject KadasMilxItem::State::serialize() const
{
  QJsonArray pts;
  for ( const KadasItemPos &pos : points )
  {
    QJsonArray p;
    p.append( pos.x() );
    p.append( pos.y() );
    pts.append( p );
  }
  QJsonArray attrs;
  for ( auto it = attributes.begin(), itEnd = attributes.end(); it != itEnd; ++it )
  {
    QJsonArray attr;
    attr.append( it.key() );
    attr.append( it.value() );
    attrs.append( attr );
  }
  QJsonArray attrPts;
  for ( auto it = attributePoints.begin(), itEnd = attributePoints.end(); it != itEnd; ++it )
  {
    QJsonArray attrPt;
    attrPt.append( it.key() );
    QJsonArray pt;
    pt.append( it.value().x() );
    pt.append( it.value().y() );
    attrPt.append( pt );
    attrPts.append( attrPt );
  }
  QJsonArray ctrlPts;
  for ( int ctrlPt : controlPoints )
  {
    ctrlPts.append( ctrlPt );
  }
  QJsonArray offset;
  offset.append( userOffset.x() );
  offset.append( userOffset.y() );

  QJsonObject json;
  json["status"] = drawStatus;
  json["points"] = pts;
  json["attributes"] = attrs;
  json["attributePoints"] = attrPts;
  json["controlPoints"] = ctrlPts;
  json["userOffset"] = offset;
  json["pressedPoints"] = pressedPoints;
  return json;;
}

bool KadasMilxItem::State::deserialize( const QJsonObject &json )
{
  points.clear();
  attributes.clear();
  attributePoints.clear();
  controlPoints.clear();

  drawStatus = static_cast<DrawStatus>( json["status"].toInt() );
  for ( QJsonValue val : json["points"].toArray() )
  {
    QJsonArray pos = val.toArray();
    points.append( KadasItemPos( pos.at( 0 ).toDouble(), pos.at( 1 ).toDouble() ) );
  }
  for ( QJsonValue attrVal : json["attributes"].toArray() )
  {
    QJsonArray attr = attrVal.toArray();
    attributes.insert( static_cast<KadasMilxClient::AttributeType>( attr.at( 0 ).toInt() ), attr.at( 1 ).toDouble() );
  }
  for ( QJsonValue attrPtVal : json["attributePoints"].toArray() )
  {
    QJsonArray attrPt = attrPtVal.toArray();
    QJsonArray pt = attrPt.at( 1 ).toArray();
    attributePoints.insert( static_cast<KadasMilxClient::AttributeType>( attrPt.at( 0 ).toInt() ), KadasItemPos( pt.at( 0 ).toDouble(), pt.at( 1 ).toDouble() ) );
  }
  for ( QJsonValue val : json["controlPoints"].toArray() )
  {
    controlPoints.append( val.toInt() );
  }
  QJsonArray offset = json["userOffset"].toArray();
  userOffset.setX( offset.at( 0 ).toDouble() );
  userOffset.setY( offset.at( 1 ).toDouble() );
  pressedPoints = json["pressedPoints"].toInt();
  return attributes.size() == attributePoints.size();
}


KadasMilxItem::KadasMilxItem( QObject *parent )
  : KadasMapItem( QgsCoordinateReferenceSystem( "EPSG:4326" ), parent )
{
  clear();
}

void KadasMilxItem::setSymbol( const KadasMilxClient::SymbolDesc &symbolDesc )
{
  mMssString = symbolDesc.symbolXml;
  mMilitaryName = symbolDesc.militaryName;
  mHasVariablePoints = symbolDesc.hasVariablePoints;
  mMinNPoints = symbolDesc.minNumPoints;
}

void KadasMilxItem::setMssString( const QString &mssString )
{
  mMssString = mssString;
  update();
}

void KadasMilxItem::setMilitaryName( const QString &militaryName )
{
  mMilitaryName = militaryName;
  update();
}

void KadasMilxItem::setMinNPoints( int minNPoints )
{
  mMinNPoints = minNPoints;
  update();
}

void KadasMilxItem::setHasVariablePoints( bool hasVariablePoints )
{
  mHasVariablePoints = hasVariablePoints;
  update();
}

KadasItemPos KadasMilxItem::position() const
{
  double x = 0., y = 0.;
  for ( const KadasItemPos &point : constState()->points )
  {
    x += point.x();
    y += point.y();
  }
  int n = std::max( 1, constState()->points.size() );
  return KadasItemPos( x / n, y / n );
}

void KadasMilxItem::setPosition( const KadasItemPos &pos )
{
  QgsPointXY prevPos = position();
  double dx = pos.x() - prevPos.x();
  double dy = pos.y() - prevPos.y();
  for ( KadasItemPos &point : state()->points )
  {
    point.setX( point.x() + dx );
    point.setY( point.y() + dy );
  }
  for ( auto it = state()->attributePoints.begin(), itEnd = state()->attributePoints.end(); it != itEnd; ++it )
  {
    it.value().setX( it.value().x() + dx );
    it.value().setY( it.value().y() + dy );
  }

  update();
}

KadasItemRect KadasMilxItem::boundingBox() const
{
  QgsRectangle r;

  for ( const KadasItemPos &p : constState()->points )
  {
    if ( r.isNull() )
    {
      r.set( p.x(), p.y(), p.x(), p.y() );
    }
    else
    {
      r.include( p );
    }
  }
  return KadasItemRect( r.xMinimum(), r.yMinimum(), r.xMaximum(), r.yMaximum() );
}

KadasMapItem::Margin KadasMilxItem::margin() const
{
  Margin m = mMargin;
  m.left = qMax( 0, m.left - constState()->userOffset.x() );
  m.right = qMax( 0, m.right + constState()->userOffset.x() );
  m.top = qMax( 0, m.top - constState()->userOffset.y() );
  m.bottom = qMax( 0, m.bottom + constState()->userOffset.y() );
  return m;
}

QList<KadasMapItem::Node> KadasMilxItem::nodes( const QgsMapSettings &settings ) const
{
  QList<Node> nodes;
  for ( int i = 0, n = constState()->points.size(); i < n; ++i )
  {
    if ( constState()->controlPoints.contains( i ) )
    {
      nodes.append( {toMapPos( constState()->points[i], settings ), ctrlPointNodeRenderer} );
    }
    else
    {
      nodes.append( {toMapPos( constState()->points[i], settings ), posPointNodeRenderer} );
    }
  }
  for ( const KadasItemPos &pos : constState()->attributePoints )
  {
    nodes.append( {toMapPos( pos, settings ), ctrlPointNodeRenderer} );
  }
  return nodes;
}

bool KadasMilxItem::intersects( const KadasMapRect &rect, const QgsMapSettings &settings ) const
{
  QPoint screenPos = settings.mapToPixel().transform( rect.center() ).toQPointF().toPoint();
  int selectedSymbol = -1;
  QList<KadasMilxClient::NPointSymbol> symbols;
  symbols.append( toSymbol( settings.mapToPixel(), settings.destinationCrs() ) );
  QRect bbox;
  return KadasMilxClient::pickSymbol( symbols, screenPos, selectedSymbol, bbox ) && selectedSymbol >= 0;
}

void KadasMilxItem::render( QgsRenderContext &context ) const
{
  KadasMilxClient::NPointSymbol symbol = toSymbol( context.mapToPixel(), context.coordinateTransform().destinationCrs() );
  if ( mCachedGraphic.isNull() || context.mapExtent() != mCachedExtent )
  {
    KadasMilxClient::NPointSymbolGraphic result;

    int dpi = context.painter()->device()->logicalDpiX();
    QRect screenExtent = computeScreenExtent( context.mapExtent(), context.mapToPixel() );
    if ( !KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, result, false ) )
    {
      return;
    }
    mCachedGraphic = result.graphic;
    mCachedGraphicOffset = result.offset;
    mCachedExtent = context.mapExtent();
  }
  QPoint renderPos = symbol.points.front() + mCachedGraphicOffset;
  if ( !isMultiPoint() )
  {
    // Draw line from visual reference point to actual refrence point
    context.painter()->drawLine( symbol.points.front(), symbol.points.front() - constState()->userOffset );
  }
  context.painter()->drawImage( renderPos, mCachedGraphic );
}

QString KadasMilxItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  if ( !kmzZip )
  {
    // Can only export to KMZ
    return "";
  }

  KadasMilxClient::NPointSymbol symbol = toSymbol( context.mapToPixel(), context.coordinateTransform().destinationCrs() );
  KadasMilxClient::NPointSymbolGraphic result;

  int dpi = context.painter()->device()->logicalDpiX();
  QRect screenExtent = computeScreenExtent( context.extent(), context.mapToPixel() );
  if ( !KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, result, true ) )
  {
    return "";
  }

  QString fileName = QUuid::createUuid().toString();
  fileName = fileName.mid( 1, fileName.length() - 2 ) + ".png";
  QuaZipFile outputFile( kmzZip );
  QuaZipNewInfo info( fileName );
  info.setPermissions( QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther );
  if ( !outputFile.open( QIODevice::WriteOnly, info ) || !result.graphic.save( &outputFile, "PNG" ) )
  {
    return "";
  }

  QgsPoint pos = QgsPoint( position() );
  pos.transform( QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ) );

  QString outString;
  QTextStream outStream( &outString );

  if ( !isMultiPoint() )
  {
    double hotSpotX = 0.5 * result.graphic.width();
    double hotSpotY = 0.5 * result.graphic.height();

    QString id = QUuid::createUuid().toString();
    id = id.mid( 1, id.length() - 2 );
    outStream << "<StyleMap id=\"" << id << "\">" << "\n";
    outStream << "  <Pair>" << "\n";
    outStream << "    <key>normal</key>" << "\n";
    outStream << "    <Style>" << "\n";
    outStream << "      <IconStyle>" << "\n";
    outStream << "        <scale>1.0</scale>" << "\n";
    outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
    outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />" << "\n";
    outStream << "      </IconStyle>" << "\n";
    outStream << "    </Style>" << "\n";
    outStream << "  </Pair>" << "\n";
    outStream << "  <Pair>" << "\n";
    outStream << "    <key>highlight</key>" << "\n";
    outStream << "    <Style>" << "\n";
    outStream << "      <IconStyle>" << "\n";
    outStream << "        <scale>1.0</scale>" << "\n";
    outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
    outStream << "        <hotSpot x=\"" << hotSpotX << "\" y=\"" << hotSpotY << "\" xunits=\"insetPixels\" yunits=\"insetPixels\" />" << "\n";
    outStream << "      </IconStyle>" << "\n";
    outStream << "    </Style>" << "\n";
    outStream << "  </Pair>" << "\n";
    outStream << "</StyleMap>" << "\n";
    outStream << "<Placemark>" << "\n";
    outStream << "  <name>" << militaryName() << "</name>" << "\n";
    outStream << "  <styleUrl>#" << id << "</styleUrl>" << "\n";
    outStream << "  <Point>" << "\n";
    outStream << "    <coordinates>" << QString::number( pos.x(), 'f', 10 ) << "," << QString::number( pos.y(), 'f', 10 ) << ",0</coordinates>" << "\n";
    outStream << "  </Point>" << "\n";
    outStream << "</Placemark>" << "\n";
  }
  else
  {
    QPoint offset = result.adjustedPoints.front() + result.offset;

    QgsPointXY pNW = context.mapToPixel().toMapCoordinates( offset.x(), offset.y() );
    QgsPointXY pSE = context.mapToPixel().toMapCoordinates( offset.x() + result.graphic.width(), offset.y() + result.graphic.height() );

    outStream << "<GroundOverlay>" << "\n";
    outStream << "<name>" << militaryName() << "</name>" << "\n";
    outStream << "<Icon><href>" << fileName << "</href></Icon>" << "\n";
    outStream << "<LatLonBox>" << "\n";
    outStream << "<north>" << pNW.y() << "</north>" << "\n";
    outStream << "<south>" << pSE.y() << "</south>" << "\n";
    outStream << "<east>" << pSE.x() << "</east>" << "\n";
    outStream << "<west>" << pNW.x() << "</west>" << "\n";
    outStream << "</LatLonBox>" << "\n";
    outStream << "</GroundOverlay>" << "\n";
  }
  outStream.flush();

  return outString;
}

bool KadasMilxItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  if ( mMssString.isEmpty() )
  {
    return false;
  }
  state()->drawStatus = State::Drawing;
  state()->points.append( toItemPos( firstPoint, mapSettings ) );
  state()->pressedPoints = 1;

  KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
  KadasMilxClient::NPointSymbolGraphic result;
  QRect screenExtent = computeScreenExtent( mapSettings.visibleExtent(), mapSettings.mapToPixel() );
  int dpi = mapSettings.outputDpi();
  KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, result, true );
  updateSymbol( mapSettings, result );

  return state()->pressedPoints < mMinNPoints || mHasVariablePoints;
}

bool KadasMilxItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasMilxItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  QRect screenRect = computeScreenExtent( mapSettings.visibleExtent(), mapSettings.mapToPixel() );
  int dpi = mapSettings.outputDpi();
  KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
  // Last possible non-control point
  int index = constState()->points.size() - 1;
  for ( ; constState()->controlPoints.contains( index ); --index );

  QPoint screenPoint = mapSettings.mapToPixel().transform( p ).toQPointF().toPoint();
  KadasMilxClient::NPointSymbolGraphic result;
  if ( KadasMilxClient::movePoint( screenRect, dpi, symbol, index, screenPoint, result ) )
  {
    updateSymbol( mapSettings, result );
  }
}

void KadasMilxItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  setCurrentPoint( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

bool KadasMilxItem::continuePart( const QgsMapSettings &mapSettings )
{
  // Only actually add a new point if more than the minimum number have been specified
  // The server automatically adds points up to the minimum number
  ++state()->pressedPoints;

  if ( state()->pressedPoints >= mMinNPoints && mHasVariablePoints )
  {
    QRect screenRect = computeScreenExtent( mapSettings.visibleExtent(), mapSettings.mapToPixel() );
    int dpi = mapSettings.outputDpi();
    KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
    QgsCoordinateTransform crst( crs(), mapSettings.destinationCrs(), QgsProject::instance()->transformContext() );
    // Last possible non-control point
    int index = constState()->points.size() - 1;
    for ( ; constState()->controlPoints.contains( index ); --index );

    QPoint screenPoint = mapSettings.mapToPixel().transform( crst.transform( constState()->points[index] ) ).toQPointF().toPoint();
    KadasMilxClient::NPointSymbolGraphic result;
    if ( KadasMilxClient::appendPoint( screenRect, dpi, symbol, screenPoint, result ) )
    {
      updateSymbol( mapSettings, result );
    }
  }
  return state()->pressedPoints < mMinNPoints || mHasVariablePoints;
}

void KadasMilxItem::endPart()
{
  state()->drawStatus = State::Finished;
}

KadasMapItem::AttribDefs KadasMilxItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute{"x"} );
  attributes.insert( AttrY, NumericAttribute{"y"} );
  return attributes;
}

KadasMapItem::AttribValues KadasMilxItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasMilxItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasMilxItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  for ( int iPoint = 0, nPoints = constState()->points.size(); iPoint < nPoints; ++iPoint )
  {
    KadasMapPos testPos = toMapPos( constState()->points[iPoint], mapSettings );
    if ( pos.sqrDist( testPos ) < pickTolSqr( mapSettings ) )
    {
      return EditContext( QgsVertexId( 0, 0, iPoint ), testPos, drawAttribs() );
    }
  }
  for ( auto it = constState()->attributePoints.begin(), itEnd = constState()->attributePoints.end(); it != itEnd; ++it )
  {
    KadasMapPos testPos = toMapPos( it.value(), mapSettings );
    if ( pos.sqrDist( testPos ) < pickTolSqr( mapSettings ) )
    {
      AttribDefs attributes;
      double min = it.key() == KadasMilxClient::AttributeAttitude ? std::numeric_limits<double>::lowest() : 0;
      double max = std::numeric_limits<double>::max();
      int decimals = it.key() == KadasMilxClient::AttributeAttitude ? 1 : 0;
      attributes.insert( it.key(), NumericAttribute{KadasMilxClient::attributeName( it.key() ), min, max, decimals } );
      return EditContext( QgsVertexId( 0, 1, it.key() ), testPos, attributes );
    }
  }
  if ( intersects( KadasMapRect( pos, pickTol( mapSettings ) ), mapSettings ) )
  {
    KadasMapPos refPos = toMapPos( constState()->points.front(), mapSettings );
    if ( !isMultiPoint() )
    {
      // Current offset symbol map position
      refPos = KadasMapPos::fromPoint( mapSettings.mapToPixel().toMapCoordinates( mapSettings.mapToPixel().transform( refPos ).toQPointF().toPoint() + constState()->userOffset ) );
    }
    return EditContext( QgsVertexId(), refPos, KadasMapItem::AttribDefs(), Qt::ArrowCursor );
  }
  return EditContext();
}

void KadasMilxItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  if ( context.vidx.isValid() )
  {
    // Move node
    QRect screenRect = computeScreenExtent( mapSettings.visibleExtent(), mapSettings.mapToPixel() );
    int dpi = mapSettings.outputDpi();
    KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );

    QPoint screenPoint = mapSettings.mapToPixel().transform( newPoint ).toQPointF().toPoint();
    KadasMilxClient::NPointSymbolGraphic result;
    if ( context.vidx.ring == 0 ) // Regular point
    {
      if ( KadasMilxClient::movePoint( screenRect, dpi, symbol, context.vidx.vertex, screenPoint, result ) )
      {
        updateSymbol( mapSettings, result );
      }
    }
    else if ( context.vidx.ring == 1 )  // Attribute point
    {
      if ( KadasMilxClient::moveAttributePoint( screenRect, dpi, symbol, context.vidx.vertex, screenPoint, result ) )
      {
        updateSymbol( mapSettings, result );
      }
    }
  }
  else if ( isMultiPoint() )
  {
    // Move multipoint symbol as a whole
    KadasMapPos refMapPos = toMapPos( constState()->points.front(), mapSettings );
    for ( KadasItemPos &pos : state()->points )
    {
      KadasMapPos mapPos = toMapPos( pos, mapSettings );
      pos = toItemPos( KadasMapPos( newPoint.x() + mapPos.x() - refMapPos.x(), newPoint.y() + mapPos.y() - refMapPos.y() ), mapSettings );
    }
    for ( auto it = state()->attributePoints.begin(), itEnd = state()->attributePoints.end(); it != itEnd; ++it )
    {
      KadasMapPos mapPos = toMapPos( it.value(), mapSettings );
      it.value() = toItemPos( KadasMapPos( newPoint.x() + mapPos.x() - refMapPos.x(), newPoint.y() + mapPos.y() - refMapPos.y() ), mapSettings );
    }
    // No need for full symbol update just for moving
    update();
  }
  else
  {
    // Set one-point symbol offset
    QPoint anchorPos = mapSettings.mapToPixel().transform( toMapPos( constState()->points.front(), mapSettings ) ).toQPointF().toPoint();
    QPoint newPos = mapSettings.mapToPixel().transform( newPoint ).toQPointF().toPoint();
    state()->userOffset = QPoint( newPos - anchorPos );
    // No need for full symbol update just for moving
    update();
  }
}

void KadasMilxItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  if ( values.size() == 1 )
  {
    // Single attribute
    KadasMilxClient::AttributeType attr = static_cast<KadasMilxClient::AttributeType>( values.firstKey() );
    state()->attributes[attr] = values[values.firstKey()];

    KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );
    KadasMilxClient::NPointSymbolGraphic result;
    QRect screenExtent = computeScreenExtent( mapSettings.visibleExtent(), mapSettings.mapToPixel() );
    int dpi = mapSettings.outputDpi();
    KadasMilxClient::updateSymbol( screenExtent, dpi, symbol, result, true );
    updateSymbol( mapSettings, result );
  }
  else
  {
    edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
  }
}

void KadasMilxItem::populateContextMenu( QMenu *menu, const EditContext &context, const KadasMapPos &clickPos, const QgsMapSettings &mapSettings )
{
  QRect screenRect = computeScreenExtent( mapSettings.visibleExtent(), mapSettings.mapToPixel() );
  QPoint screenPos = mapSettings.mapToPixel().transform( clickPos ).toQPointF().toPoint();
  int dpi = mapSettings.outputDpi();
  KadasMilxClient::NPointSymbol symbol = toSymbol( mapSettings.mapToPixel(), mapSettings.destinationCrs() );

  QAction *actionEdit = menu->addAction( tr( "Symbol editor..." ), [ = ]
  {
    KadasMilxClient::NPointSymbolGraphic result;
    WId winId = QApplication::topLevelWidgets().front()->winId();
    if ( KadasMilxClient::editSymbol( screenRect, dpi, symbol, mMssString, mMilitaryName, result, winId ) )
    {
      updateSymbol( mapSettings, result );
    }
  } );
  if ( isMultiPoint() )
  {
    if ( context.vidx.vertex >= 0 )
    {
      QAction *actionDeletePoint = menu->addAction( tr( "Delete node" ), [ = ]
      {
        KadasMilxClient::NPointSymbolGraphic result;
        if ( KadasMilxClient::deletePoint( screenRect, dpi, symbol, context.vidx.vertex, result ) )
          updateSymbol( mapSettings, result );
      } );
      bool canDelete = false;
      actionDeletePoint->setEnabled( KadasMilxClient::canDeletePoint( symbol, context.vidx.vertex, canDelete ) && canDelete );
    }
    else
    {
      menu->addAction( tr( "Add node" ), [ = ]
      {
        KadasMilxClient::NPointSymbolGraphic result;
        if ( KadasMilxClient::insertPoint( screenRect, dpi, symbol, screenPos, result ) )
          updateSymbol( mapSettings, result );
      } );
    }
  }
  else
  {
    menu->addAction( tr( "Reset offset" ), [ = ]
    {
      state()->userOffset = QPoint();
      update();
    } );
  }
}

KadasMapItem::AttribValues KadasMilxItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  if ( context.attributes.size() == 1 )
  {
    // Single attribute
    KadasMilxClient::AttributeType attr = static_cast<KadasMilxClient::AttributeType>( context.attributes.firstKey() );
    AttribValues values;
    values.insert( attr, constState()->attributes[attr] );
    return values;
  }
  else
  {
    return drawAttribsFromPosition( pos, mapSettings );
  }
}

KadasMapPos KadasMilxItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  if ( values.size() == 1 )
  {
    // Single attribute
    KadasMilxClient::AttributeType attr = static_cast<KadasMilxClient::AttributeType>( values.firstKey() );
    return toMapPos( constState()->attributePoints[attr], mapSettings );
  }
  else
  {
    return positionFromDrawAttribs( values, mapSettings );
  }
}

QList<QPoint> KadasMilxItem::computeScreenPoints( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const
{
  QList<QPoint> points;
  for ( const KadasItemPos &p : constState()->points )
  {
    points.append( mapToPixel.transform( mapCrst.transform( p ) ).toQPointF().toPoint() );
  }
  return points;
}

QList< QPair<int, double> > KadasMilxItem::computeScreenAttributes( const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const
{
  if ( constState()->attributes.isEmpty() )
  {
    return QList<QPair<int, double> >();
  }
  double m2p = metersToPixels( constState()->points.front(), mapToPixel, mapCrst );

  QList< QPair<int, double> > screenAttribs;
  for ( auto it = constState()->attributes.begin(), itEnd = constState()->attributes.end(); it != itEnd; ++it )
  {
    double value = it.value();
    if ( it.key() != KadasMilxClient::AttributeAttitude )
    {
      value = value * m2p;
    }
    screenAttribs.append( qMakePair( it.key(), value ) );
  }
  return screenAttribs;
}

bool KadasMilxItem::isMultiPoint() const
{
  return constState()->points.size() > 1 || !constState()->attributes.isEmpty();
}

KadasMilxClient::NPointSymbol KadasMilxItem::toSymbol( const QgsMapToPixel &mapToPixel, const QgsCoordinateReferenceSystem &mapCrs, bool colored ) const
{
  QgsCoordinateTransform mapCrst( mCrs, mapCrs, QgsProject::instance()->transformContext() );
  QList<QPoint> points = computeScreenPoints( mapToPixel, mapCrst );
  for ( int i = 0, n = points.size(); i < n; ++i )
  {
    points[i] += constState()->userOffset;
  }
  QList< QPair<int, double> > screenAttribs = computeScreenAttributes( mapToPixel, mapCrst );
  bool finalized = constState()->drawStatus == State::DrawStatus::Finished;
  return KadasMilxClient::NPointSymbol( mMssString, points, constState()->controlPoints, screenAttribs, finalized, colored );
}

void KadasMilxItem::writeMilx( QDomDocument &doc, QDomElement &itemElement ) const
{
  QDomElement stringXmlEl = doc.createElement( "MssStringXML" );
  stringXmlEl.appendChild( doc.createTextNode( mMssString ) );
  itemElement.appendChild( stringXmlEl );

  QDomElement nameEl = doc.createElement( "Name" );
  nameEl.appendChild( doc.createTextNode( mMilitaryName ) );
  itemElement.appendChild( nameEl );

  QDomElement pointListEl = doc.createElement( "PointList" );
  itemElement.appendChild( pointListEl );

  for ( const QgsPointXY &p : constState()->points )
  {
    QDomElement pEl = doc.createElement( "Point" );
    pointListEl.appendChild( pEl );

    QDomElement pXEl = doc.createElement( "X" );
    pXEl.appendChild( doc.createTextNode( QString::number( p.x(), 'f', 6 ) ) );
    pEl.appendChild( pXEl );
    QDomElement pYEl = doc.createElement( "Y" );
    pYEl.appendChild( doc.createTextNode( QString::number( p.y(), 'f', 6 ) ) );
    pEl.appendChild( pYEl );
  }
  if ( !constState()->attributes.isEmpty() )
  {
    QDomElement attribListEl = doc.createElement( "LocationAttributeList" );
    itemElement.appendChild( attribListEl );
    for ( auto it = constState()->attributes.begin(), itEnd = constState()->attributes.end(); it != itEnd; ++it )
    {
      QDomElement attrTypeEl = doc.createElement( "AttrType" );
      attrTypeEl.appendChild( doc.createTextNode( KadasMilxClient::attributeName( it.key() ) ) );
      QDomElement attrValueEl = doc.createElement( "Value" );
      attrValueEl.appendChild( doc.createTextNode( QString::number( it.value() ) ) );
      QDomElement attribEl = doc.createElement( "LocationAttribute" );
      attribEl.appendChild( attrTypeEl );
      attribEl.appendChild( attrValueEl );
      attribListEl.appendChild( attribEl );
    }
  }

  if ( !isMultiPoint() )
  {
    QDomElement offsetEl = doc.createElement( "Offset" );
    itemElement.appendChild( offsetEl );

    QDomElement factorXEl = doc.createElement( "FactorX" );
    factorXEl.appendChild( doc.createTextNode( QString::number( double( constState()->userOffset.x() ) / KadasMilxClient::getSymbolSize() ) ) );
    offsetEl.appendChild( factorXEl );

    QDomElement factorYEl = doc.createElement( "FactorY" );
    factorYEl.appendChild( doc.createTextNode( QString::number( -double( constState()->userOffset.y() ) / KadasMilxClient::getSymbolSize() ) ) );
    offsetEl.appendChild( factorYEl );
  }
}

KadasMilxItem *KadasMilxItem::fromMilx( const QDomElement &itemElement, const QgsCoordinateTransform &crst, int symbolSize )
{
  KadasMilxItem *item = new KadasMilxItem();

  item->mMilitaryName = itemElement.firstChildElement( "Name" ).text();
  item->mMssString = itemElement.firstChildElement( "MssStringXML" ).text();

  bool isCorridor = itemElement.firstChildElement( "IsMIPCorridorPointList" ).text().toInt();

  QDomNodeList pointEls = itemElement.firstChildElement( "PointList" ).elementsByTagName( "Point" );
  for ( int iPoint = 0, nPoints = pointEls.count(); iPoint < nPoints; ++iPoint )
  {
    QDomElement pointEl = pointEls.at( iPoint ).toElement();
    double x = pointEl.firstChildElement( "X" ).text().toDouble();
    double y = pointEl.firstChildElement( "Y" ).text().toDouble();
    QgsPointXY pos = crst.transform( QgsPoint( x, y ) );
    item->state()->points.append( KadasItemPos( pos.x(), pos.y() ) );
  }
  QDomNodeList attribEls = itemElement.firstChildElement( "LocationAttributeList" ).elementsByTagName( "LocationAttribute" );
  for ( int iAttr = 0, nAttrs = attribEls.count(); iAttr < nAttrs; ++iAttr )
  {
    QDomElement attribEl = attribEls.at( iAttr ).toElement();
    item->state()->attributes.insert( KadasMilxClient::attributeIdx( attribEl.firstChildElement( "AttrType" ).text() ), attribEl.firstChildElement( "Value" ).text().toDouble() );
  }
  double offsetX = itemElement.firstChildElement( "Offset" ).firstChildElement( "FactorX" ).text().toDouble() * symbolSize;
  double offsetY = -1. * ( itemElement.firstChildElement( "Offset" ).firstChildElement( "FactorY" ).text().toDouble() * symbolSize );

  item->state()->userOffset = QPoint( offsetX, offsetY );

  if ( item->state()->points.size() > 1 )
  {
    if ( isCorridor )
    {
      const QList<KadasItemPos> &points = item->state()->points;

      // Do some fake geo -> screen transform, since here we have no idea about screen coordinates
      double scale = 100000.;
      QPoint origin = QPoint( points[0].x() * scale, points[0].y() * scale );
      QList<QPoint> screenPoints = QList<QPoint>() << QPoint( 0, 0 );
      for ( int i = 1, n = points.size(); i < n; ++i )
      {
        screenPoints.append( QPoint( points[i].x() * scale, points[i].y() * scale ) - origin );
      }
      QList< QPair<int, double> > screenAttributes;
      if ( !item->state()->attributes.isEmpty() )
      {
        QgsDistanceArea da;
        da.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );
        da.setEllipsoid( "WGS84" );
        QgsPointXY otherPoint( points[0].x() + 0.001, points[0].y() );
        QPointF otherScreenPoint = QPointF( otherPoint.x() * scale, otherPoint.y() * scale ) - origin;
        double ellipsoidDist = da.measureLine( points[0], otherPoint ) * QgsUnitTypes::fromUnitToUnitFactor( da.lengthUnits(), QgsUnitTypes::DistanceMeters );
        double screenDist = QVector2D( screenPoints[0] - otherScreenPoint ).length();
        for ( auto it = item->constState()->attributes.begin(), itEnd = item->constState()->attributes.end(); it != itEnd; ++it )
        {
          double value = it.value();
          if ( it.key() != KadasMilxClient::AttributeAttitude )
          {
            value = value / ellipsoidDist * screenDist;
          }
          screenAttributes.append( qMakePair( it.key(), value ) );
        }
        item->state()->attributes.clear();
      }
      if ( KadasMilxClient::getControlPoints( item->mMssString, screenPoints, screenAttributes, item->state()->controlPoints, isCorridor ) )
      {
        item->state()->points.clear();
        for ( const QPoint &screenPoint : screenPoints )
        {
          double x = ( origin.x() + screenPoint.x() ) / scale;
          double y = ( origin.y() + screenPoint.y() ) / scale;
          item->state()->points.append( KadasItemPos( x, y ) );
        }
      }
    }
    else
    {
      KadasMilxClient::getControlPointIndices( item->mMssString, item->state()->points.count(), item->state()->controlPoints );
    }
  }

  if ( item->mMilitaryName.isEmpty() )
  {
    KadasMilxClient::getMilitaryName( item->mMssString, item->mMilitaryName );
  }

  item->state()->drawStatus = State::DrawStatus::Finished;

  return item;
}

void KadasMilxItem::posPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::black, 1 ) );
  painter->setBrush( Qt::yellow );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}

void KadasMilxItem::ctrlPointNodeRenderer( QPainter *painter, const QPointF &screenPoint, int nodeSize )
{
  painter->setPen( QPen( Qt::black, 1 ) );
  painter->setBrush( Qt::red );
  painter->drawEllipse( screenPoint.x() - 0.5 * nodeSize, screenPoint.y() - 0.5 * nodeSize, nodeSize, nodeSize );
}

QRect KadasMilxItem::computeScreenExtent( const QgsRectangle &mapExtent, const QgsMapToPixel &mapToPixel )
{
  QPoint topLeft = mapToPixel.transform( mapExtent.xMinimum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  QPoint topRight = mapToPixel.transform( mapExtent.xMaximum(), mapExtent.yMinimum() ).toQPointF().toPoint();
  QPoint bottomLeft = mapToPixel.transform( mapExtent.xMinimum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  QPoint bottomRight = mapToPixel.transform( mapExtent.xMaximum(), mapExtent.yMaximum() ).toQPointF().toPoint();
  int xMin = qMin( qMin( topLeft.x(), topRight.x() ), qMin( bottomLeft.x(), bottomRight.x() ) );
  int xMax = qMax( qMax( topLeft.x(), topRight.x() ), qMax( bottomLeft.x(), bottomRight.x() ) );
  int yMin = qMin( qMin( topLeft.y(), topRight.y() ), qMin( bottomLeft.y(), bottomRight.y() ) );
  int yMax = qMax( qMax( topLeft.y(), topRight.y() ), qMax( bottomLeft.y(), bottomRight.y() ) );
  return QRect( xMin, yMin, xMax - xMin, yMax - yMin ).normalized();
}

double KadasMilxItem::metersToPixels( const QgsPointXY &refPoint, const QgsMapToPixel &mapToPixel, const QgsCoordinateTransform &mapCrst ) const
{
  QgsDistanceArea da;
  da.setSourceCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance()->transformContext() );
  da.setEllipsoid( "WGS84" );

  QgsPointXY point = constState()->points.front();
  QPointF screenPoint = mapToPixel.transform( mapCrst.transform( point ) ).toQPointF();
  QgsPointXY otherPoint( point.x() + 0.001, point.y() );
  QPointF otherScreenPoint = mapToPixel.transform( mapCrst.transform( otherPoint ) ).toQPointF();
  double ellipsoidDist = da.measureLine( point, otherPoint ) * QgsUnitTypes::fromUnitToUnitFactor( da.lengthUnits(), QgsUnitTypes::DistanceMeters );
  double screenDist = QVector2D( screenPoint - otherScreenPoint ).length();
  return screenDist / ellipsoidDist;
}

void KadasMilxItem::updateSymbol( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result )
{
  QgsCoordinateTransform mapCrst( crs(), mapSettings.destinationCrs(), QgsProject::instance()->transformContext() );
  state()->points.clear();
  for ( const QPoint &screenPoint : result.adjustedPoints )
  {
    QgsPointXY pos = mapCrst.transform( mapSettings.mapToPixel().toMapCoordinates( screenPoint ), QgsCoordinateTransform::ReverseTransform );
    state()->points.append( KadasItemPos( pos.x(), pos.y() ) );
  }

  state()->controlPoints = result.controlPoints;

  state()->attributes.clear();
  double m2p = metersToPixels( state()->points.first(), mapSettings.mapToPixel(), mapCrst );
  for ( auto it = result.attributes.begin(), itEnd = result.attributes.end(); it != itEnd; ++it )
  {
    double value = it.value();
    if ( it.key() != KadasMilxClient::AttributeAttitude )
    {
      value /= m2p;
    }
    state()->attributes.insert( it.key(), value );
  }

  state()->attributePoints.clear();
  for ( auto it = result.attributePoints.begin(), itEnd = result.attributePoints.end(); it != itEnd; ++it )
  {
    QgsPointXY itemPos = mapCrst.transform( mapSettings.mapToPixel().toMapCoordinates( it.value() ), QgsCoordinateTransform::ReverseTransform );
    state()->attributePoints.insert( it.key(), KadasItemPos( itemPos.x(), itemPos.y() ) );
  }

  QRect pointBounds( result.adjustedPoints.front(), result.adjustedPoints.front() );
  for ( int i = 1, n = result.adjustedPoints.size(); i < n; ++i )
  {
    const QPoint &p = result.adjustedPoints[i];
    pointBounds.setLeft( qMin( pointBounds.left(), p.x() ) );
    pointBounds.setRight( qMax( pointBounds.right(), p.x() ) );
    pointBounds.setTop( qMin( pointBounds.top(), p.y() ) );
    pointBounds.setBottom( qMax( pointBounds.bottom(), p.y() ) );
  }

  QPoint offset = result.adjustedPoints.front() + result.offset;
  QRect symbolBounds( offset.x(), offset.y(), result.graphic.width(), result.graphic.height() );
  mMargin.left = qMax( 0, pointBounds.left() - symbolBounds.left() );
  mMargin.top = qMax( 0, pointBounds.top() - symbolBounds.top() );
  mMargin.right = qMax( 0, symbolBounds.right() - pointBounds.right() );
  mMargin.bottom = qMax( 0, symbolBounds.bottom() - pointBounds.bottom() );

  // Invalidate cache
  mCachedGraphic = QImage();

  update();
}
