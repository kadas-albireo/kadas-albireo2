/***************************************************************************
    kadasmilxannotationitem.h
    -------------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASMILXANNOTATIONITEM_H
#define KADASMILXANNOTATIONITEM_H

#include <QString>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/kadas_gui.h"

/**
 * \ingroup gui
 * \brief Kadas MilX (military symbology) annotation item.
 *
 * Skeleton placeholder for the Phase 6 MilX port. Holds an MSS string and
 * a list of WGS84 points; rendering, hit-testing and edit logic will be
 * filled in by subsequent slices and ultimately delegate to
 * \c KadasMilxClient.
 *
 * Type id: \c "kadas:milx".
 */
class KADAS_GUI_EXPORT KadasMilxAnnotationItem : public QgsAnnotationItem
{
  public:
    KadasMilxAnnotationItem();

    static QString itemTypeId() { return QStringLiteral( "kadas:milx" ); }

    QString type() const override;
    QgsRectangle boundingBox() const override;
    void render( QgsRenderContext &context, QgsFeedback *feedback ) override;
    bool writeXml( QDomElement &element, QDomDocument &document, const QgsReadWriteContext &context ) const override;
    bool readXml( const QDomElement &element, const QgsReadWriteContext &context ) override;
    KadasMilxAnnotationItem *clone() const override;

    static KadasMilxAnnotationItem *create();

    //! MSS (Military Symbology Standard) symbol string.
    QString mssString() const { return mMssString; }
    void setMssString( const QString &mssString ) { mMssString = mssString; }

    //! Human-readable military name of the symbol.
    QString militaryName() const { return mMilitaryName; }
    void setMilitaryName( const QString &militaryName ) { mMilitaryName = militaryName; }

    //! Symbol type (single-point / multi-point classification used by libmss).
    QString symbolType() const { return mSymbolType; }
    void setSymbolType( const QString &symbolType ) { mSymbolType = symbolType; }

    //! Minimum number of control points required by the symbol.
    int minNumPoints() const { return mMinNumPoints; }
    void setMinNumPoints( int minNumPoints ) { mMinNumPoints = minNumPoints; }

    //! Whether the symbol accepts a variable number of points beyond the minimum.
    bool hasVariablePoints() const { return mHasVariablePoints; }
    void setHasVariablePoints( bool hasVariablePoints ) { mHasVariablePoints = hasVariablePoints; }

    //! Control points, stored in EPSG:4326 (lon, lat). MilX is always WGS84.
    QList<QgsPointXY> points() const { return mPoints; }
    void setPoints( const QList<QgsPointXY> &points ) { mPoints = points; }

  private:
    QString mMssString;
    QString mMilitaryName;
    QString mSymbolType;
    int mMinNumPoints = 1;
    bool mHasVariablePoints = false;
    QList<QgsPointXY> mPoints;
};

#endif // KADASMILXANNOTATIONITEM_H
