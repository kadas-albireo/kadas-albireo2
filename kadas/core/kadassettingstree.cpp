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

#include "kadassettingstree.h"

QgsSettingsTreeNode *KadasSettingsTree::treeRoot()
{
  // this must be defined in cpp code so we are sure only one instance is around
  static QgsSettingsTreeNode *sTreeRoot = QgsSettingsTreeNode::createRootNode();
  return sTreeRoot;
}

QgsSettingsTreeNode *KadasSettingsTree::createPluginTreeNode( const QString &pluginName )
{
  QgsSettingsTreeNode *te = sTreePlugins->childNode( pluginName );
  if ( te )
    return te;
  else
    return sTreePlugins->createChildNode( pluginName );
}

void KadasSettingsTree::unregisterPluginTreeNode( const QString &pluginName )
{
  QgsSettingsTreeNode *pluginTreeNode = sTreePlugins->childNode( pluginName );
  delete pluginTreeNode;
}
