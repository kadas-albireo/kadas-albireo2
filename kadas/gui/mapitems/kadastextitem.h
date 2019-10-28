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

#include <kadas/gui/mapitems/kadasanchoreditem.h>


class KADAS_GUI_EXPORT KadasTextItem : public KadasAnchoredItem
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QColor outlineColor READ outlineColor WRITE setOutlineColor )
    Q_PROPERTY( QColor fillColor READ fillColor WRITE setFillColor )
    Q_PROPERTY( QFont font READ font WRITE setFont )

  public:
    KadasTextItem( const QgsCoordinateReferenceSystem &crs, QObject *parent = nullptr );

    QString itemName() const override { return tr( "Text" ); }

    void setText( const QString &text );
    const QString &text() const { return mText; }
    void setFillColor( const QColor &c );
    QColor fillColor() const { return mFillColor; }
    void setOutlineColor( const QColor &c );
    QColor outlineColor() const { return mOutlineColor; }
    void setFont( const QFont &font );
    const QFont &font() const { return mFont; }

    void render( QgsRenderContext &context ) const override;
#ifndef SIP_RUN
    QString asKml( const QgsRenderContext &context, QuaZip *kmzZip = nullptr ) const override;
#endif

  private:
    QString mText;
    QColor mOutlineColor;
    QColor mFillColor;
    QFont mFont;

    KadasMapItem *_clone() const override { return new KadasTextItem( crs() ); } SIP_FACTORY
};

#endif // KADASTEXTITEM_H
