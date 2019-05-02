/***************************************************************************
    kadasredlininglayer.cpp
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

#include <QUuid>

#include <qgis/qgsfeaturestore.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssymbollayerutils.h>

#include <kadas/core/kadasredlininglayer.h>
#include <kadas/core/kadasredliningrenderer.h>

KadasRedliningLayer::KadasRedliningLayer( const QString& name , const QString &crs ) : QgsVectorLayer(
      QString( "unknown?crs=%1&memoryid=%2" ).arg( crs ).arg( QUuid::createUuid().toString() ),
      name,
      "memory" )
{
  mValid = true;

  QgsEditFormConfig config = editFormConfig();
  config.setSuppress(QgsEditFormConfig::SuppressOn);
  setEditFormConfig(config);

  dataProvider()->addAttributes( QList<QgsField>()
                                 << QgsField( "size", QVariant::Double, "real", 2, 2 )
                                 << QgsField( "outline", QVariant::String, "string", 15 )
                                 << QgsField( "fill", QVariant::String, "string", 15 )
                                 << QgsField( "outline_style", QVariant::Int, "integer", 1 )
                                 << QgsField( "fill_style", QVariant::Int, "integer", 1 )
                                 << QgsField( "text", QVariant::String, "string", 128 )
                                 << QgsField( "flags", QVariant::String, "string", 64 )
                                 << QgsField( "tooltip", QVariant::String, "string", 128 )
                                 << QgsField( "attributes", QVariant::String, "string", 8192 ) );
  setRenderer( new KadasRedliningRenderer );
  updateFields();

  setCustomProperty( "labeling", "pal" );
  setCustomProperty( "labeling/enabled", true );
  setCustomProperty( "labeling/displayAll", true );
  setCustomProperty( "labeling/fieldName", "text" );
  setCustomProperty( "labeling/dataDefined/Color", "1~~0~~~~fill" );
  setCustomProperty( "labeling/dataDefined/Size", "1~~1~~regexp_substr(\"flags\",'fontSize=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Bold", "1~~1~~regexp_substr(\"flags\",'bold=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Italic", "1~~1~~regexp_substr(\"flags\",'italic=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Family", "1~~1~~regexp_substr(\"flags\",'family=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Rotation", "1~~1~~regexp_substr(\"flags\",'rotation=([^,]+)')~~" );
  setDisplayExpression( "\"tooltip\"" );
  connect( this, SIGNAL( layerTransparencyChanged( int ) ), this, SLOT( changeTextTransparency( int ) ) );
}

bool KadasRedliningLayer::addShape( const QgsGeometry& geometry, const QColor &outline, const QColor &fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle, const QString& flags, const QString& tooltip , const QString &text, const QString& attributes )
{
  QFont font;
  QgsFeature f( fields() );
  f.setGeometry( geometry );
  f.setAttribute( "size", outlineSize );
  f.setAttribute( "text", text );
  f.setAttribute( "outline", QgsSymbolLayerUtils::encodeColor( outline ) );
  f.setAttribute( "fill", QgsSymbolLayerUtils::encodeColor( fill ) );
  f.setAttribute( "outline_style", QgsSymbolLayerUtils::encodePenStyle( outlineStyle ) );
  f.setAttribute( "fill_style" , QgsSymbolLayerUtils::encodeBrushStyle( fillStyle ) );
  QMap<QString, QString> flagsMap = deserializeFlags( flags );
  if ( !text.isEmpty() )
  {
    flagsMap["family"] = flagsMap.value( "family", font.family() );
    flagsMap["italic"] = flagsMap.value( "italic", QString( "%1" ).arg( font.italic() ) );
    flagsMap["bold"] = flagsMap.value( "bold", QString( "%1" ).arg( font.bold() ) );
    flagsMap["fontSize"] = flagsMap.value( "fontSize", QString::number( font.pointSize() ) );
  }
  flagsMap["rotation"] = flagsMap.value( "rotation", "0" );
  f.setAttribute( "flags", serializeFlags( flagsMap ) );
  f.setAttribute( "tooltip", tooltip );
  f.setAttribute( "attributes", attributes );
  return dataProvider()->addFeatures( QgsFeatureList() << f );
}

bool KadasRedliningLayer::addText( const QString &text, const QgsPoint& pos, const QColor& color, const QFont& font, const QString& tooltip, double rotation, int markerSize )
{
  while ( rotation <= -180 ) rotation += 360.;
  while ( rotation > 180 ) rotation -= 360.;
  QgsFeature f( fields() );
  f.setGeometry( QgsGeometry( pos.clone() ) );
  f.setAttribute( "text", text );
  f.setAttribute( "size", markerSize );
  f.setAttribute( "fill", QgsSymbolLayerUtils::encodeColor( color ) );
  f.setAttribute( "outline", QgsSymbolLayerUtils::encodeColor( color ) );
  f.setAttribute( "flags", QString( "family=%1,italic=%2,bold=%3,rotation=%4,fontSize=%5" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() ).arg( rotation ).arg( font.pointSize() ) );
  f.setAttribute( "tooltip", tooltip );
  return dataProvider()->addFeatures( QgsFeatureList() << f );
}

QgsFeatureList KadasRedliningLayer::pasteFeatures( const QgsFeatureStore& featureStore, QgsPointXY* targetPos )
{
  QgsCoordinateTransform ct = QgsCoordinateTransform( featureStore.crs(), crs(), QgsProject::instance() );
  QgsFeatureList newFeatures;
  QgsPointXY oldCenter;
  for ( const QgsFeature& feature : featureStore.features() )
  {
    QString flags = feature.attribute( "flags" ).toString();
    if ( flags.isEmpty() )
    {
      if ( feature.geometry().type() == QgsWkbTypes::PointGeometry )
      {
        flags = "shape=point,symbol=circle";
      }
      else if ( feature.geometry().type() == QgsWkbTypes::LineGeometry )
      {
        flags = "shape=line";
      }
      else if ( feature.geometry().type() == QgsWkbTypes::PolygonGeometry )
      {
        flags = "shape=polygon";
      }
      else
      {
        continue;
      }
    }
    QgsAbstractGeometry* transformedGeometry = feature.geometry().get()->clone();
    transformedGeometry->transform(ct);
    QgsFeature newFeature( dataProvider()->fields() );
    newFeature.setGeometry( QgsGeometry( transformedGeometry ) );
    QgsPointXY c = newFeature.geometry().boundingBox().center();
    oldCenter.setX( oldCenter.x() + c.x());
    oldCenter.setY( oldCenter.y() + c.y());

    newFeature.setAttributes( feature.attributes() );

    QMap<QString, QVariant> attribs;
    attribs["flags"] = flags;
    attribs["size"] = 1;
    attribs["outline"] = QgsSymbolLayerUtils::encodeColor( Qt::black );
    attribs["fill"] = QgsSymbolLayerUtils::encodeColor( Qt::yellow );
    attribs["outline_style"] = QgsSymbolLayerUtils::encodePenStyle( Qt::SolidLine );
    attribs["fill_style"] = QgsSymbolLayerUtils::encodeBrushStyle( Qt::SolidPattern );

    for ( const QString& key : attribs.keys() )
    {
      QVariant srcValue = feature.attribute( key );
      newFeature.setAttribute( key, srcValue.isNull() ? attribs[key] : srcValue );
    }
    newFeatures.append( newFeature );
  }
  if ( targetPos )
  {
    int n = newFeatures.size();
    oldCenter = QgsPoint( oldCenter.x() / n, oldCenter.y() / n );
    QgsVector delta = *targetPos - oldCenter;
    for ( int i = 0; i < n; ++i )
    {
      newFeatures[i].geometry().translate( delta.x(), delta.y() );
    }
  }

  dataProvider()->addFeatures( newFeatures );
  return newFeatures;
}

QgsFeatureId KadasRedliningLayer::addFeature( QgsFeature& f )
{
  QgsFeatureList features = QgsFeatureList() << f;
  if ( dataProvider()->addFeatures( features ) )
  {
    updateExtents();
    emit featureAdded( features.front().id() );
  }
  return features.front().id();
}

void KadasRedliningLayer::deleteFeature( QgsFeatureId fid )
{
  if ( dataProvider()->deleteFeatures( QgsFeatureIds() << fid ) )
  {
    updateExtents();
    emit featureDeleted( fid );
  }
}

void KadasRedliningLayer::changeGeometry( QgsFeatureId fid, const QgsGeometry& geom )
{
  QgsGeometryMap geomMap;
  geomMap[fid] = geom;
  dataProvider()->changeGeometryValues( geomMap );
  updateExtents();
  emit geometryChanged( fid, geom );
}

void KadasRedliningLayer::changeAttributes( QgsFeatureId fid, const QgsAttributeMap& attribs )
{
  QgsChangedAttributesMap changedAttribs;
  changedAttribs[fid] = attribs;
  dataProvider()->changeAttributeValues( changedAttribs );
  updateExtents();
  foreach ( int key, attribs.keys() )
  {
    emit attributeValueChanged( fid, key, attribs[key] );
  }
}


QMap<QString, QString> KadasRedliningLayer::deserializeFlags( const QString& flagsStr )
{
  QMap<QString, QString> flagsMap;
  foreach ( const QString& flag, flagsStr.split( ",", QString::SkipEmptyParts ) )
  {
    int pos = flag.indexOf( "=" );
    flagsMap.insert( flag.left( pos ), pos >= 0 ? flag.mid( pos + 1 ) : QString() );
  }
  return flagsMap;
}

QString KadasRedliningLayer::serializeFlags( const QMap<QString, QString>& flagsMap )
{
  QString flagsStr;
  foreach ( const QString& key, flagsMap.keys() )
  {
    flagsStr += QString( "%1=%2," ).arg( key ).arg( flagsMap.value( key ) );
  }
  return flagsStr;
}

bool KadasRedliningLayer::readXml(const QDomNode &layer_node, QgsReadWriteContext &context)
{
  QgsFeatureList features;
  QDomNodeList nodes = layer_node.toElement().childNodes();
  setOpacity(1. - layer_node.firstChildElement( "layerTransparency" ).text().toInt() / 255);
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement redliningItemElem = nodes.at( iNode ).toElement();
    if ( redliningItemElem.nodeName() == "RedliningItem" )
    {
      QgsFeature feature( dataProvider()->fields() );
      feature.setAttribute( "size", redliningItemElem.attribute( "size", "1" ) );
      feature.setAttribute( "outline", redliningItemElem.attribute( "outline", "255,0,0,255" ) );
      feature.setAttribute( "fill", redliningItemElem.attribute( "fill", "0,0,255,255" ) );
      feature.setAttribute( "outline_style", redliningItemElem.attribute( "outline_style", "1" ) );
      feature.setAttribute( "fill_style", redliningItemElem.attribute( "fill_style", "1" ) );
      feature.setAttribute( "text", redliningItemElem.attribute( "text", "" ) );
      feature.setAttribute( "flags", redliningItemElem.attribute( "flags", "" ) );
      feature.setAttribute( "tooltip", redliningItemElem.attribute( "tooltip" ) );
      feature.setAttribute( "attributes", redliningItemElem.attribute( "attributes", "" ) );
      feature.setGeometry( QgsGeometry::fromWkt( redliningItemElem.attribute( "geometry", "" ) ) );
      features.append( feature );
    }
  }
  dataProvider()->addFeatures( features );
  updateFields();
  emit layerModified();
  return true;
}

bool KadasRedliningLayer::writeXml(QDomNode &layer_node, QDomDocument &doc, const QgsReadWriteContext &context) const
{
  QDomElement layerElement = layer_node.toElement();
  layerElement.setAttribute( "type", "redlining" );

  QDomElement transparencyElem = doc.createElement( "layerTransparency" );
  QDomText transparencyValue = doc.createTextNode( QString::number( int(255 *  1. - opacity() ) ) );
  transparencyElem.appendChild( transparencyValue );
  layer_node.appendChild( transparencyElem );

  QgsFeatureIterator it = getFeatures();
  QgsFeature feature;
  while ( it.nextFeature( feature ) )
  {
    QDomElement redliningItemElem = doc.createElement( "RedliningItem" );
    redliningItemElem.setAttribute( "size", feature.attribute( "size" ).toString() );
    redliningItemElem.setAttribute( "outline", feature.attribute( "outline" ).toString() );
    redliningItemElem.setAttribute( "fill", feature.attribute( "fill" ).toString() );
    redliningItemElem.setAttribute( "outline_style", feature.attribute( "outline_style" ).toString() );
    redliningItemElem.setAttribute( "fill_style", feature.attribute( "fill_style" ).toString() );
    redliningItemElem.setAttribute( "text", feature.attribute( "text" ).toString() );
    redliningItemElem.setAttribute( "flags", feature.attribute( "flags" ).toString() );
    redliningItemElem.setAttribute( "geometry", feature.geometry().asWkt() );
    redliningItemElem.setAttribute( "tooltip", feature.attribute( "tooltip" ).toString() );
    redliningItemElem.setAttribute( "attributes", feature.attribute( "attributes" ).toString() );
    layer_node.appendChild( redliningItemElem );
  }
  return true;
}

void KadasRedliningLayer::changeTextTransparency( int transparency )
{
  setCustomProperty( "labeling/textTransp", transparency );
}
