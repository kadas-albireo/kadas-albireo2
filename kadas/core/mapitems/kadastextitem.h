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

#include <kadas/core/kadas_core.h>
#include <kadas/core/mapitems/kadasanchoreditem.h>


class KADAS_CORE_EXPORT KadasTextItem : public KadasAnchoredItem
{
public:
  KadasTextItem ( const QgsCoordinateReferenceSystem& crs, QObject* parent = nullptr );

  void setText ( const QString& text );
  const QString& text() const { return mText; }
  void setFillColor ( const QColor& c );
  QColor fillColor() const { return mFillColor; }
  void setOutlineColor ( const QColor& c );
  QColor outlineColor() const { return mOutlineColor; }
  void setFont ( const QFont& font );
  const QFont& font() const { return mFont; }

  void render ( QgsRenderContext& context ) const override;

private:
  QString mText;
  QColor mOutlineColor;
  QColor mFillColor;
  QFont mFont;
};

#endif // KADASTEXTITEM_H
