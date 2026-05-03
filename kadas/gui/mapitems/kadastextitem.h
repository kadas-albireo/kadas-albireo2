/***************************************************************************
    kadastextitem.h
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

#ifndef KADASTEXTITEM_H
#define KADASTEXTITEM_H

#include <qgis/qgsannotationpointtextitem.h>

#include "kadas/gui/mapitems/kadaspointitem.h"


class KADAS_GUI_EXPORT KadasTextItem : public KadasAbstractPointItem
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QColor color READ color WRITE setColor )
    Q_PROPERTY( QColor outlineColor READ outlineColor WRITE setOutlineColor )
    Q_PROPERTY( QFont font READ font WRITE setFont )
    Q_PROPERTY( double angle READ angle WRITE setAngle )

  public:
    KadasTextItem( const QgsCoordinateReferenceSystem &crs );
    ~KadasTextItem() override;

    QString itemName() const override { return tr( "Text" ); }

    virtual void setItemGeometry( const QgsPointXY &point ) override;

    virtual QgsAnnotationPointTextItem *annotationItem( const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) const override SIP_FACTORY;

    void setText( const QString &text );
    const QString &text() const { return mText; }

    void setColor( const QColor &color );
    QColor color() const { return mColor; }

    //! Sets the text outline color. An invalid color disables the outline buffer.
    void setOutlineColor( const QColor &color );
    QColor outlineColor() const { return mOutlineColor; }

    void setFont( const QFont &font );
    const QFont &font() const { return mFont; }

    //! Sets the text rotation \a angle, in degrees clockwise.
    void setAngle( double angle );
    double angle() const { return mAngle; }

    virtual QgsRectangle boundingBox() const override;
    virtual void render( QgsRenderContext &context ) const override;


    // virtual EditContext getEditContext(const KadasMapPos &pos, const QgsMapSettings &mapSettings) const override;
    // virtual void edit(const EditContext &context, const KadasMapPos &newPoint, const QgsMapSettings &mapSettings) override;
    // virtual void edit(const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings) override;
    // virtual AttribValues editAttribsFromPosition(const EditContext &context, const KadasMapPos &pos, const QgsMapSettings &mapSettings) const override;
    // virtual KadasMapPos positionFromEditAttribs(const EditContext &context, const AttribValues &values, const QgsMapSettings &mapSettings) const override;

#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

  protected:
    virtual KadasMapItem *_clone() const override;

  private:
#ifndef SIP_RUN
    //! Private constructor taking ownership of \a qgsItem
    KadasTextItem( const QgsCoordinateReferenceSystem &crs, QgsAnnotationPointTextItem *qgsItem );
#endif

    void updateQgsAnnotation() override;

    QgsAnnotationPointTextItem *mQgsItem = nullptr;

    QString mText;
    QColor mColor;
    QColor mOutlineColor;
    QFont mFont;
    double mAngle = 0.0;
    // TODO: frame (background + border) and callout were supported by the previous
    // KadasRectangleItemBase-derived implementation. QgsAnnotationPointTextItem does
    // not expose these; restoring parity requires either a custom QgsAnnotationItem
    // subclass or an upstream contribution to QGIS.
};

#endif // KADASTEXTITEM_H
