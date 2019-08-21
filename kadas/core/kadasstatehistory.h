/***************************************************************************
    kadasstatehistory.h
    -------------------
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

#ifndef KADASSTATEHISTORY_H
#define KADASSTATEHISTORY_H

#include <QObject>
#include <QSharedPointer>
#include <QStack>

#include <kadas/core/kadas_core.h>


class KADAS_CORE_EXPORT KadasStateHistory : public QObject
{
    Q_OBJECT
  public:
    struct State
    {
      virtual ~State() {}
    };

    KadasStateHistory( QObject *parent = 0 );
    ~KadasStateHistory();
    void clear();
    void push( State *state );
    void undo();
    void redo();
    bool canUndo() const { return mCurrent > 0; }
    bool canRedo() const { return mCurrent < mStates.length() - 1; }

  signals:
    void canUndoChanged( bool );
    void canRedoChanged( bool );
    void stateChanged( State *state );

  private:
    QVector<State *> mStates;
    int mCurrent = -1;
};

#endif // KADASSTATEHISTORY_H
