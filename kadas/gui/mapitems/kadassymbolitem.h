/***************************************************************************
    kadassymbolitem.h
    -----------------
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

#ifndef KADASSYMBOLITEM_H
#define KADASSYMBOLITEM_H

#include <kadas/gui/mapitems/kadasanchoreditem.h>


class KADAS_GUI_EXPORT KadasSymbolItem : public KadasAnchoredItem
{
public:
  KadasSymbolItem ( const QgsCoordinateReferenceSystem& crs, QObject* parent = nullptr );

  void setFilePath ( const QString& path, double anchorX = 0.5, double anchorY = 0.5 );
  const QString& filePath() const { return mFilePath; }
  void setName ( const QString& name ) { mName = name; }
  const QString& name() const { return mName; }
  void setRemarks ( const QString& remarks ) { mRemarks = remarks; }
  const QString& remarks() const { return mRemarks; }

  void render ( QgsRenderContext& context ) const override;

private:
  QString mFilePath;
  QString mName;
  QString mRemarks;
};

#endif // KADASSYMBOLITEM_H
