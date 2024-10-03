/***************************************************************************
    kadasmapitemeditor.h
    --------------------
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

#ifndef KADASMAPITEMEDITOR_H
#define KADASMAPITEMEDITOR_H

#include <functional>

#include <QMap>
#include <QWidget>

#include <qgis/qgis_sip.h>
#include "kadas/gui/kadas_gui.h"


class KadasMapItem;

class KADAS_GUI_EXPORT KadasMapItemEditor : public QWidget
{
    Q_OBJECT

  public:
    static inline const QString GPX_ROUTE = QStringLiteral( "KadasGpxRouteEditor" );
    static inline const QString SYMBOL_ATTRIBUTES =  QStringLiteral( "KadasSymbolAttributesEditor" );
    static inline const QString GPX_WAYPOINT = QStringLiteral( "KadasGpxWaypointEditor" );
    static inline const QString REDLINING_ITEM = QStringLiteral( "KadasRedliningItemEditor" );
    static inline const QString REDLINING_TEXT = QStringLiteral( "KadasRedliningTextEditor" );
    static inline const QString MEASURE_WIDGET = QStringLiteral( "KadasMeasureWidget" );

    enum class EditorType SIP_MONKEYPATCH_SCOPEENUM
    {
      CreateItemEditor,
      EditItemEditor
    };

    KadasMapItemEditor( KadasMapItem *item, QWidget *parent = nullptr ) : QWidget( parent ), mItem( item ) {}

    virtual void setItem( KadasMapItem *item ) { mItem = item; }

    // TODO: SIP
#ifndef SIP_RUN
    typedef std::function<KadasMapItemEditor*( KadasMapItem *, EditorType ) > Factory;
    typedef QMap<QString, Factory> Registry;
    static Registry *registry();
#endif

  public slots:
    virtual void reset() {}
    virtual void syncItemToWidget() = 0;
    virtual void syncWidgetToItem() = 0;

  protected:
    KadasMapItem *mItem;
};

#endif // KADASMAPITEMEDITOR_H
