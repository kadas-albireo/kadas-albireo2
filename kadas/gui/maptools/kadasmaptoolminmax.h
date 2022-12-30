/***************************************************************************
    kadasmaptoolminmax.h
    ----------------------
    copyright            : (C) 2022 by Sandro Mani
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

#ifndef KADASMAPTOOLMINMAX_H
#define KADASMAPTOOLMINMAX_H

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/maptools/kadasmaptoolcreateitem.h>

class KadasSymbolItem;

class KADAS_GUI_EXPORT KadasMapToolMinMax : public KadasMapToolCreateItem
{
    Q_OBJECT
  public:
    KadasMapToolMinMax( QgsMapCanvas *mapCanvas );
    ~KadasMapToolMinMax();
    enum FilterType { FilterRect, FilterPoly, FilterCircle };

    void setFilterType( FilterType filterType );
    void canvasPressEvent( QgsMapMouseEvent *e ) override;
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;

  private slots:
    void requestPick();
    void drawFinished();

  private:
    KadasMapToolCreateItem::ItemFactory itemFactory( const QgsMapCanvas *canvas, FilterType filterType ) const;
    FilterType mFilterType = FilterRect;
    QComboBox *mFilterTypeCombo = nullptr;
    QPointer<KadasSymbolItem> mPinMin;
    QPointer<KadasSymbolItem> mPinMax;
    bool mPickFeature = false;
};

#endif // KADASMAPTOOLMINMAX_H
