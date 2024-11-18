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

#include <thread>
#include <mutex>

#include "kadas/gui/mapitemeditors/kadasmapitemeditor.h"
#include "kadas/gui/mapitemeditors/kadasgpxrouteeditor.h"
#include "kadas/gui/mapitemeditors/kadasgpxwaypointeditor.h"
#include "kadas/gui/mapitemeditors/kadasredliningitemeditor.h"
#include "kadas/gui/mapitemeditors/kadasredliningtexteditor.h"
#include "kadas/gui/mapitemeditors/kadassymbolattributeseditor.h"
#include "kadas/gui/maptools/kadasmaptoolmeasure.h"

Q_GLOBAL_STATIC( KadasMapItemEditor::Registry, sRegistry )

std::once_flag onceFlag;

KadasMapItemEditor::Registry *KadasMapItemEditor::registry()
{
  std::call_once( onceFlag, []() {
    sRegistry->insert( QStringLiteral( "KadasGpxRouteEditor" ), []( KadasMapItem *item, KadasMapItemEditor::EditorType ) { return new KadasGpxRouteEditor( item ); } );
    sRegistry->insert( QStringLiteral( "KadasSymbolAttributesEditor" ), []( KadasMapItem *item, KadasMapItemEditor::EditorType ) { return new KadasSymbolAttributesEditor( item ); } );
    sRegistry->insert( QStringLiteral( "KadasGpxWaypointEditor" ), []( KadasMapItem *item, KadasMapItemEditor::EditorType ) { return new KadasGpxWaypointEditor( item ); } );
    sRegistry->insert( QStringLiteral( "KadasRedliningItemEditor" ), []( KadasMapItem *item, KadasMapItemEditor::EditorType ) { return new KadasRedliningItemEditor( item ); } );
    sRegistry->insert( QStringLiteral( "KadasRedliningTextEditor" ), []( KadasMapItem *item, KadasMapItemEditor::EditorType ) { return new KadasRedliningTextEditor( item ); } );
    sRegistry->insert( QStringLiteral( "KadasMeasureWidget" ), []( KadasMapItem *item, KadasMapItemEditor::EditorType ) { return new KadasMeasureWidget( item ); } );
  } );
  return sRegistry;
};
