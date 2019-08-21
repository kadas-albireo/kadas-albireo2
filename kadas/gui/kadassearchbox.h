/***************************************************************************
    kadassearchbox.h
    ----------------
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

#ifndef KADASSEARCHBOX_H
#define KADASSEARCHBOX_H

#include <QList>
#include <QTimer>
#include <QWidget>
#include <QPointer>

#include <kadas/gui/kadas_gui.h>
#include <kadas/gui/kadassearchprovider.h>


class QToolButton;
class QgsMapCanvas;
class KadasMapItem;
class KadasSymbolItem;
class KadasMapToolCreateItem;

class KADAS_GUI_EXPORT KadasSearchBox : public QWidget
{
    Q_OBJECT

  public:
    KadasSearchBox( QWidget *parent = 0 );
    ~KadasSearchBox();

    void init( QgsMapCanvas *canvas );

    void addSearchProvider( KadasSearchProvider *provider );
    void removeSearchProvider( KadasSearchProvider *provider );

  public slots:
    void clearSearch();

  protected:
    bool eventFilter( QObject *obj, QEvent *ev ) override;

  private:
    class LineEdit;
    class TreeWidget;

    enum FilterType { FilterRect, FilterPoly, FilterCircle };

    enum EntryType { EntryTypeCategory, EntryTypeResult };
    static const int sEntryTypeRole;
    static const int sCatNameRole;
    static const int sCatPrecedenceRole;
    static const int sCatCountRole;
    static const int sResultDataRole;

    QgsMapCanvas *mMapCanvas;
    KadasSymbolItem *mPin = nullptr;
    KadasMapToolCreateItem *mFilterTool = nullptr;
    KadasMapItem *mFilterItem = nullptr;
    QList<KadasSearchProvider *> mSearchProviders;
    QTimer mTimer;
    LineEdit *mSearchBox;
    QToolButton *mFilterButton;
    QToolButton *mSearchButton;
    QToolButton *mClearButton;
    TreeWidget *mTreeWidget;
    int mNumRunningProviders;

    void cancelSearch();
    void clearPin();

  private slots:
    void textChanged();
    void startSearch();
    void searchResultFound( KadasSearchProvider::SearchResult result );
    void searchProviderFinished();
    void resultSelected();
    void resultActivated();
    void clearFilter();
    void setFilterTool();
    void filterToolFinished();
};

#endif // KADASSEARCHBOX_H
