/***************************************************************************
    kadaspointitem.h
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

#ifndef KADASPOINTITEM_H
#define KADASPOINTITEM_H

#include "qgis/qgsannotationmarkeritem.h"
#include "qgis/qgsmultipoint.h"

#include "kadas_gui.h"
#include "kadasmapitem.h"


class KADAS_GUI_EXPORT KadasPointItem : public KadasQgsAnnotationWrapper
{
    // Q_OBJECT

  public:
    KadasPointItem( Qgis::MarkerShape icon = Qgis::MarkerShape::Circle );
    virtual ~KadasPointItem();

    QString itemName() const override { return QObject::tr( "Point" ); }

    KadasPointItem *clone() const override;

    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip ) const override;


    void setShape( Qgis::MarkerShape shape );

    bool startPart( const KadasMapPos &firstPoint, const QgsMapSettings &mapSettings ) override;
    bool startPart( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    void setCurrentPoint( const KadasMapPos &p, const QgsMapSettings &mapSettings ) override;
    void setCurrentAttributes( const AttribValues &values, const QgsMapSettings &mapSettings ) override;
    bool continuePart( const QgsMapSettings &mapSettings ) override;
    void endPart() override;

    AttribDefs drawAttribs() const override;
    AttribValues drawAttribsFromPosition( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromDrawAttribs( const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    EditContext getEditContext( const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    void edit( const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings ) override;
    void edit( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) override;

    AttribValues editAttribsFromPosition( const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings ) const override;
    KadasMapPos positionFromEditAttribs( const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings ) const override;

    KadasItemPos position() const override;
    void setPosition( const KadasItemPos &pos ) override;

    //Qgis::GeometryType geometryType() const override { return Qgis::GeometryType::Point; }
    //void addPartFromGeometry( const QgsAbstractGeometry &geom ) override;

    const QgsMultiPoint *geometry() const;

  protected:
    bool mIsPointSymbol = true;

  private:
    void updateSymbol();

    Qgis::MarkerShape mShape = Qgis::MarkerShape::Circle;
    int mIconSize = 10;
    QColor mStrokeColor = Qt::red;
    double mStrokeWidth = 2;
    QColor mFillColor = Qt::white;


    enum AttribIds
    {
      AttrX,
      AttrY
    };

    QgsMultiPoint mGeometry;


    // KadasMapItemBase interface
  public:
    virtual bool intersects( const QgsRectangle &rect, const QgsMapSettings &settings, bool contains ) const override;
};

#endif // KADASPOINTITEM_H
