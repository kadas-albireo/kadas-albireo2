/***************************************************************************
    kadasribbonsplitbutton.h
    ------------------------
    copyright            : (C) 2026 by OPENGIS.ch
    email                : info@opengis.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASRIBBONSPLITBUTTON_H
#define KADASRIBBONSPLITBUTTON_H

#include <QIcon>
#include <QList>
#include <QObject>

class QAction;
class QMenu;
class QToolButton;
class QgsSettingsEntryString;
class KadasRibbonActionGallery;

/**
 * Turns a ribbon QToolButton into an Office-style split button: the main
 * (icon) area runs the last-used tool of a category, while the arrow area
 * opens a gallery of all the category's tools. The last-used tool is
 * remembered between sessions through a QgsSettingsEntryString.
 *
 * Usage: construct, addAction() each tool, then call finish() once.
 */
class KadasRibbonSplitButton : public QObject
{
    Q_OBJECT
  public:
    KadasRibbonSplitButton( QToolButton *button, const QString &categoryLabel, const QIcon &fallbackIcon, const QgsSettingsEntryString *lastToolSetting, QObject *parent = nullptr );

    //! Adds a tool action to the category. Actions should have a unique objectName for persistence.
    void addAction( QAction *action );

    //! Builds the dropdown gallery and restores the last-used tool. Call once after all addAction() calls.
    void finish();

  private:
    void setCurrentAction( QAction *action, bool persist );

    QToolButton *mButton = nullptr;
    QString mCategoryLabel;
    QIcon mFallbackIcon;
    const QgsSettingsEntryString *mLastToolSetting = nullptr;
    KadasRibbonActionGallery *mGallery = nullptr;
    QMenu *mMenu = nullptr;
    QList<QAction *> mActions;
    QAction *mCurrentAction = nullptr;
};

#endif // KADASRIBBONSPLITBUTTON_H
