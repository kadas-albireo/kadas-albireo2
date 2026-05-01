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

#include <QMap>
#include <QPoint>
#include <QString>

#include <qgis/qgsannotationitem.h>
#include <qgis/qgspointxy.h>
#include <qgis/qgsrectangle.h>

#include "kadas/gui/kadas_gui.h"
#include "kadas/gui/milx/kadasmilxclient.h"

class QgsMapSettings;

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

    //! Indices into points() that libmss reports as draggable control points
    //! (rendered as red ctrl-point handles, omitted from geometry export).
    QList<int> controlPoints() const { return mControlPoints; }
    void setControlPoints( const QList<int> &cp ) { mControlPoints = cp; }

    //! Symbol attribute values (width/length/radius/attitude) in WGS84 metres
    //! / degrees, keyed by \c KadasMilxAttrType.
    QMap<KadasMilxAttrType, double> attributes() const { return mAttributes; }
    void setAttributes( const QMap<KadasMilxAttrType, double> &a ) { mAttributes = a; }

    //! Per-attribute control points reported by libmss, in EPSG:4326.
    QMap<KadasMilxAttrType, QgsPointXY> attributePoints() const { return mAttributePoints; }
    void setAttributePoints( const QMap<KadasMilxAttrType, QgsPointXY> &p ) { mAttributePoints = p; }

    //! User-applied screen-space offset (drag of the symbol graphic).
    QPoint userOffset() const { return mUserOffset; }
    void setUserOffset( const QPoint &offset ) { mUserOffset = offset; }

    //! Number of physical clicks made during interactive draw. Reset to 0 when
    //! a finalized item is created; updated by the controller draw state machine.
    int pressedPoints() const { return mPressedPoints; }
    void setPressedPoints( int n ) { mPressedPoints = n; }

    enum class DrawStatus
    {
      Empty,
      Drawing,
      Finished
    };
    DrawStatus drawStatus() const { return mDrawStatus; }
    void setDrawStatus( DrawStatus s ) { mDrawStatus = s; }

    // ---- libmss helpers ---------------------------------------------------

    //! Builds the libmss \c NPointSymbol describing this item in the given
    //! map's screen-space coordinates. Uses the current draw status to set
    //! the \c finalized flag.
    KadasMilxClient::NPointSymbol toSymbol( const QgsMapSettings &mapSettings, bool colored = true ) const;

    //! Replaces this item's points / controlPoints / attributes / attributePoints
    //! from a libmss result graphic by un-projecting screen coordinates back to
    //! EPSG:4326 through \a mapSettings.
    void applySymbolResult( const QgsMapSettings &mapSettings, const KadasMilxClient::NPointSymbolGraphic &result );

    //! Computes the on-screen extent of \a mapSettings' visible map extent
    //! (helper for libmss IPC, which works in device pixels).
    static QRect computeScreenExtent( const QgsMapSettings &mapSettings );

    //! True when the symbol has more than one geometry point or carries any
    //! shape attribute (length / width / radius / attitude). Mirrors the
    //! legacy \c KadasMilxItem::isMultiPoint() heuristic that decides whether
    //! libmss IPC is needed for vertex moves vs a direct geometry update.
    bool isMultiPoint() const { return mPoints.size() > 1 || !mAttributes.isEmpty(); }

  private:
    QString mMssString;
    QString mMilitaryName;
    QString mSymbolType;
    int mMinNumPoints = 1;
    bool mHasVariablePoints = false;
    QList<QgsPointXY> mPoints;
    QList<int> mControlPoints;
    QMap<KadasMilxAttrType, double> mAttributes;
    QMap<KadasMilxAttrType, QgsPointXY> mAttributePoints;
    QPoint mUserOffset;
    int mPressedPoints = 0;
    DrawStatus mDrawStatus = DrawStatus::Finished;

    //! Pixel-per-metre scale at the first point, derived from the map's
    //! projection and the WGS84 ellipsoid (used to convert libmss attribute
    //! values, which are stored in metres, to/from screen pixels).
    double metersToPixels( const QgsMapSettings &mapSettings ) const;
};

#endif // KADASMILXANNOTATIONITEM_H
