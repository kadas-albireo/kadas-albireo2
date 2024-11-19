/***************************************************************************
    kdaskmlimport.h
    ---------------
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

#ifndef KADASKMLIMPORT_H
#define KADASKMLIMPORT_H

#include <QImage>
#include <QObject>

#include <qgis/qgspoint.h>

class QDomDocument;
class QDomElement;
class QuaZip;
class QgsMapCanvas;
class QgsRedliningLayer;

class KadasKMLImport : public QObject
{
    Q_OBJECT
  public:
    bool importFile( const QString &filename, QString &errMsg );

  private:
    struct StyleData
    {
        int outlineSize = 1;
        Qt::PenStyle outlineStyle = Qt::SolidLine;
        Qt::BrushStyle fillStyle = Qt::SolidPattern;
        QColor outlineColor = Qt::black;
        QColor fillColor = Qt::white;
        bool isLabel = false;
        QColor labelColor = Qt::black;
        double labelScale = 1.;
        QString icon;
        QPointF hotSpot;
    };
    struct TileData
    {
        QString iconHref;
        QgsRectangle bbox;
        QSize size;
    };
    struct OverlayData
    {
        QgsRectangle bbox;
        QList<TileData> tiles;
    };

    bool importDocument( const QString &filename, const QDomDocument &doc, QString &errMsg, QuaZip *zip = nullptr );
    void buildVSIVRT( const QString &name, OverlayData &overlayData, QuaZip *kmzZip ) const;
    QVector<QgsPoint> parseCoordinates( const QDomElement &geomEl ) const;
    StyleData parseStyle( const QDomElement &styleEl, QuaZip *zip ) const;
    QList<QgsAbstractGeometry *> parseGeometries( const QDomElement &containerEl, int &types, bool *extrude = nullptr );
    QMap<QString, QString> parseExtendedData( const QDomElement &placemarkEl );
    QColor parseColor( const QString &abgr ) const;
};


#endif // KADASKMLIMPORT_H
