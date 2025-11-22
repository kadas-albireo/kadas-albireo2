/***************************************************************************
    kadaspointitem.cpp
    ------------------
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

#include <QJsonArray>

#include <qgis/qgspoint.h>
#include <qgis/qgsproject.h>
#include <qgis/qgsmultipoint.h>
#include <qgis/qgsmapsettings.h>
#include <qgis/qgsmarkersymbol.h>
#include <qgis/qgsmarkersymbollayer.h>
#include <qgis/qgssymbollayerutils.h>

#include "kadas/gui/mapitems/kadaspointitem.h"


KadasPointItem::KadasPointItem( const QgsCoordinateReferenceSystem &crs, Qgis::MarkerShape icon )
  : KadasMapItem( crs )
{
  mQgsItem = new QgsAnnotationMarkerItem( QgsPoint() );
  setShape( icon );
  clear();
}

void KadasPointItem::setShape( Qgis::MarkerShape shape )
{
  mShape = shape;
  updateSymbol();
}

void KadasPointItem::translate( double dx, double dy )
{
  QgsGeometry geom = QgsGeometry::fromPointXY( mQgsItem->geometry() );
  geom.translate( dx, dy );
  mQgsItem->setGeometry( *qgsgeometry_cast<const QgsPoint *>( geom.constGet() ) );

  update();
}

void KadasPointItem::setIconSize( int size )
{
  mIconSize = size;
  updateSymbol();
}

void KadasPointItem::setColor( const QColor &color )
{
  mFillColor = color;
  updateSymbol();
}

void KadasPointItem::setStrokeColor( const QColor &strokeColor )
{
  mStrokeColor = strokeColor;
  updateSymbol();
}

void KadasPointItem::setStrokeWidth( double width )
{
  mStrokeWidth = width;
  updateSymbol();
}

void KadasPointItem::setStrokeStyle( const Qt::PenStyle &style )
{
  mStrokeStyle = style;
  updateSymbol();
}

bool KadasPointItem::startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings )
{
  mDrawStatus = DrawStatus::Drawing;
  setPoint( QgsPoint( firstPoint ) );
  return false;
}

bool KadasPointItem::startPart( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  return startPart( KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

void KadasPointItem::setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

void KadasPointItem::setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings )
{
  // Do nothing
}

bool KadasPointItem::continuePart( const QgsMapSettings &mapSettings )
{
  // No further action allowed
  return false;
}

void KadasPointItem::endPart()
{
  mDrawStatus = DrawStatus::Finished;
}

KadasMapItem::AttribDefs KadasPointItem::drawAttribs() const
{
  AttribDefs attributes;
  attributes.insert( AttrX, NumericAttribute { "x" } );
  attributes.insert( AttrY, NumericAttribute { "y" } );
  return attributes;
}

KadasMapItem::AttribValues KadasPointItem::drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  AttribValues values;
  values.insert( AttrX, pos.x() );
  values.insert( AttrY, pos.y() );
  return values;
}

KadasMapPos KadasPointItem::positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return KadasMapPos( values[AttrX], values[AttrY] );
}

KadasMapItem::EditContext KadasPointItem::getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  QgsPointXY testPos = toMapPos( point(), mapSettings );
  if ( pos.sqrDist( KadasMapPos::fromPoint( testPos ) ) < pickTolSqr( mapSettings ) )
  {
    return EditContext( QgsVertexId( 0, 0, 0 ), KadasMapPos::fromPoint( testPos ), drawAttribs() );
  }
  return EditContext();
}

void KadasPointItem::edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings )
{
  setPoint( QgsPoint( toItemPos( newPoint, mapSettings ) ) );
}

void KadasPointItem::edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings )
{
  edit( context, KadasMapPos( values[AttrX], values[AttrY] ), mapSettings );
}

KadasMapItem::AttribValues KadasPointItem::editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const
{
  return drawAttribsFromPosition( pos, mapSettings );
}

KadasMapPos KadasPointItem::positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const
{
  return positionFromDrawAttribs( values, mapSettings );
}

QgsPointXY KadasPointItem::point() const
{
  return mQgsItem->geometry();
}

void KadasPointItem::setPoint( const QgsPointXY &point )
{
  mQgsItem->setGeometry( QgsPoint( point ) );
  update();
}

void KadasPointItem::updateSymbol()
{
  QgsSimpleMarkerSymbolLayer *symbolLayer = new QgsSimpleMarkerSymbolLayer();
  symbolLayer->setSizeUnit( Qgis::RenderUnit::Points );
  symbolLayer->setShape( mShape );
  symbolLayer->setSize( mIconSize );
  symbolLayer->setStrokeWidth( mStrokeWidth );
  symbolLayer->setStrokeWidthUnit( Qgis::RenderUnit::Points );
  symbolLayer->setStrokeColor( mStrokeColor );
  symbolLayer->setColor( mFillColor );
  symbolLayer->setStrokeStyle( mStrokeStyle );
  mQgsItem->setSymbol( new QgsMarkerSymbol( { symbolLayer } ) );

  update();
}

KadasMapItem *KadasPointItem::_clone() const SIP_FACTORY
{
  KadasPointItem *item = new KadasPointItem( crs() );
  item->mQgsItem = mQgsItem->clone();
  item->mShape = mShape;
  item->mIconSize = mIconSize;
  item->mStrokeColor = mStrokeColor;
  item->mStrokeWidth = mStrokeWidth;
  item->mFillColor = mFillColor;
  return item;
}

void KadasPointItem::writeXmlPrivate( QDomElement &element ) const
{
  //element.setAttribute("status", static_cast<int>( state().drawStatus ));
  element.setAttribute( "shape", qgsEnumValueToKey( mShape ) );
  element.setAttribute( "size", mIconSize );
  element.setAttribute( "stroke_color", mStrokeColor.name() );
  element.setAttribute( "stroke_width", mStrokeWidth );
  element.setAttribute( "fill_color", mFillColor.name() );
  element.setAttribute( "geometry", point().asWkt() );
}

void KadasPointItem::readXmlPrivate( const QDomElement &element )
{
  if ( !element.hasAttribute( "shape" ) )
  {
    // migration code
    QJsonObject data = QJsonDocument::fromJson( element.firstChild().toCDATASection().data().toLocal8Bit() ).object();
    if ( data.contains( "props" ) )
    {
      QJsonObject props = data["props"].toObject();

      mCrs = QgsCoordinateReferenceSystem( props.value( "authId" ).toString() );
      mEditor = props.value( "editor" ).toString();
      mIconSize = props.value( "iconSize" ).toInt();

      QStringList brushStr = props.value( "iconFill" ).toString().split( ";" );
      if ( brushStr.size() )
        mFillColor = QColor( brushStr[0] );

      QStringList penStr = props.value( "iconOutline" ).toString().split( ";" );
      if ( penStr.size() > 1 )
      {
        mStrokeColor = QColor( penStr[0] );
        mStrokeWidth = penStr[1].toInt();
      }

      // this must be done at the end
      switch ( props.value( "iconType" ).toInt( 1 ) )
      {
        case 1:
          mShape = Qgis::MarkerShape::Cross;
          break;
        case 2:
          mShape = Qgis::MarkerShape::Cross2;
          break;
        case 3:
          mShape = Qgis::MarkerShape::Square;
          break;
        case 4:
          mShape = Qgis::MarkerShape::Circle;
          break;
        case 5:
          mShape = Qgis::MarkerShape::Square;
          break;
        case 6:
          mShape = Qgis::MarkerShape::Triangle;
          break;
        case 7:
          mShape = Qgis::MarkerShape::Triangle;
          break;
        default:
          mShape = Qgis::MarkerShape::Circle;
          break;
      }
    }
    if ( data.contains( "state" ) )
    {
      const QJsonArray point = data.value( "state" ).toObject().value( "points" ).toArray().first().toArray();
      setPoint( QgsPointXY( point.at( 0 ).toDouble(), point.at( 1 ).toDouble() ) );
    }
  }
  else
  {
    mShape = qgsEnumKeyToValue( element.attribute( "shape", qgsEnumValueToKey( Qgis::MarkerShape::Circle ) ), Qgis::MarkerShape::Circle );
    mIconSize = element.attribute( "size", "4" ).toInt();
    mStrokeColor = QColor( element.attribute( "stroke_color", QColor( Qt::red ).name() ) );
    mStrokeWidth = element.attribute( "stroke_width", "1" ).toInt();
    mFillColor = QColor( element.attribute( "fill_color", QColor( Qt::white ).name() ) );
    setPoint( QgsGeometry::fromWkt( element.attribute( "geometry" ) ).asPoint() );
  }
  updateSymbol();
}


QgsRectangle KadasPointItem::boundingBox() const
{
  return mQgsItem->boundingBox();
}

QList<KadasMapItem::Node> KadasPointItem::nodes( const QgsMapSettings &settings ) const
{
  return { { toMapPos( KadasItemPos::fromPoint( mQgsItem->geometry() ), settings ) } };
}

bool KadasPointItem::intersects( const QgsRectangle &rect, const QgsMapSettings &settings, bool contains ) const
{
  return rect.intersects( boundingBox() );
}

void KadasPointItem::render( QgsRenderContext &context ) const
{
  QgsFeedback fb;
  mQgsItem->render( context, &fb );
}

QString KadasPointItem::asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const
{
  if ( mQgsItem->geometry().isEmpty() )
    return QString();

  auto color2hex = []( const QColor &c ) { return QString( "%1%2%3%4" ).arg( c.alpha(), 2, 16, QChar( '0' ) ).arg( c.blue(), 2, 16, QChar( '0' ) ).arg( c.green(), 2, 16, QChar( '0' ) ).arg( c.red(), 2, 16, QChar( '0' ) ); };

  QString outString;
  QTextStream outStream( &outString );
  outStream << "<Placemark>\n";
  outStream << QString( "<name>%1</name>\n" ).arg( exportName() );
  outStream << "<Style>\n";
  outStream << QString( "<LineStyle><width>%1</width><color>%2</color></LineStyle>\n<PolyStyle><fill>%3</fill><color>%4</color></PolyStyle>\n" )
                 .arg( strokeWidth() )
                 .arg( color2hex( strokeColor() ) )
                 .arg( 1 )
                 .arg( color2hex( color() ) );
  outStream << "</Style>\n";
  outStream << "<ExtendedData>\n";
  outStream << "<SchemaData schemaUrl=\"#KadasGeometryItem\">\n";
  outStream << QString( "<SimpleData name=\"icon_type\">%1</SimpleData>\n" ).arg( static_cast<int>( mShape ) );
  outStream << QString( "<SimpleData name=\"outline_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodePenStyle( QPen( mStrokeColor, mStrokeWidth ).style() ) );
  outStream << QString( "<SimpleData name=\"fill_style\">%1</SimpleData>\n" ).arg( QgsSymbolLayerUtils::encodeBrushStyle( QBrush( mFillColor ).style() ) );
  outStream << "</SchemaData>\n";
  outStream << "</ExtendedData>\n";
  QgsPoint point = QgsPoint( mQgsItem->geometry() );
  point.transform( QgsCoordinateTransform( mCrs, QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsProject::instance() ) );
  outStream << point.asKml( 6 ) << "\n";
  outStream << "</Placemark>\n";
  outStream.flush();
  return outString;
}


void KadasPointItem::setState( const KadasMapItem::State *state )
{
  KadasMapItem::setState( state );
  updateSymbol();
}
