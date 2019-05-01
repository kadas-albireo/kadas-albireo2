/***************************************************************************
    kadasredlininglayer.h
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

#ifndef KADASREDLININGLAYER_H
#define KADASREDLININGLAYER_H

#include <qgis/qgsvectorlayer.h>

#include <kadas/core/kadas_core.h>

class QgsFeatureStore;
class QgsPoint;

class KADAS_CORE_EXPORT KadasRedliningLayer : public QgsVectorLayer
{
    Q_OBJECT
  public:
    KadasRedliningLayer( const QString& name = QString( "" ), const QString& crs = "EPSG:3857" );
    bool addShape(const QgsGeometry &geometry, const QColor& outline, const QColor& fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle , const QString &flags = QString() , const QString &tooltip = QString(), const QString& text = QString() , const QString &attributes = QString() );
    bool addText( const QString &text, const QgsPoint &pos, const QColor& color, const QFont& font , const QString &tooltip = QString() , double rotation = 0, int markerSize = 2 );
    QgsFeatureList pasteFeatures(const QgsFeatureStore &featureStore , QgsPointXY *targetPos = 0 );

    QgsFeatureId addFeature( QgsFeature& f );
    void deleteFeature( QgsFeatureId fid );
    void changeGeometry( QgsFeatureId fid, const QgsGeometry &geom );
    void changeAttributes( QgsFeatureId fid, const QgsAttributeMap& attribs );

    static QMap<QString, QString> deserializeFlags( const QString& flagsStr );
    static QString serializeFlags( const QMap<QString, QString> &flagsMap );

  protected:
    bool readXml( const QDomNode& layer_node, QgsReadWriteContext &context ) override;
    bool writeXml( QDomNode &layer_node, QDomDocument &doc, const QgsReadWriteContext &context ) const override;

  private slots:
    void changeTextTransparency( int );
};

#endif // KADASREDLININGLAYER_H
