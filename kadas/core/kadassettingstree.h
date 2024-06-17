/***************************************************************************
  kadassettingstree.h
  --------------------------------------
  Date                 : February 2023
  Copyright            : (C) 2023 by Denis Rouzaud
  Email                : denis@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASSETTINGSTREE_H
#define KADASSETTINGSTREE_H

#include <qgis/qgssettingstreenode.h>

#include <kadas/core/kadas_core.h>

/**
 * \ingroup core
 * \class KadasSettingsTree
 * \brief KadasSettingsTree holds the tree structure for the settings
 */
class KADAS_CORE_EXPORT KadasSettingsTree
{

  public:

#ifndef SIP_RUN

    /**
     * Returns the tree root node for the settings tree
     */
    static QgsSettingsTreeNode *treeRoot();

    // only create first level here
    static inline QgsSettingsTreeNode *sTreeApp = treeRoot()->createChildNode( QStringLiteral( "app" ) );
    static inline QgsSettingsTreeNode *sTreePlugins = treeRoot()->createChildNode( QStringLiteral( "plugins" ) );

#endif

    /**
     * Returns the tree node at the given \a key
     * \note For Plugins, use createPluginTreeNode() to create nodes for plugin settings.
     */
    static const QgsSettingsTreeNode *node( const QString &key ) {return treeRoot()->childNode( key );}

    /**
     * Creates a settings tree node for the given \a pluginName
     */
    static QgsSettingsTreeNode *createPluginTreeNode( const QString &pluginName );


    /**
     * Unregisters the tree node for the given plugin
     */
    static void unregisterPluginTreeNode( const QString &pluginName );
};

#endif // KADASSETTINGSTREE_H
