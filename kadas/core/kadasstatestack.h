/***************************************************************************
    kadasstatestack.h
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

#ifndef KADASSTATESTACK_H
#define KADASSTATESTACK_H

#include <QObject>
#include <QSharedPointer>
#include <QStack>

#include <kadas/core/kadas_core.h>


class KADAS_CORE_EXPORT KadasStateStack : public QObject
{
  Q_OBJECT
public:
  struct State {
    virtual ~State() {}
  };

  class StateChangeCommand
  {
  public:
    StateChangeCommand ( KadasStateStack* stateStack, State* newState, bool compress );
    virtual ~StateChangeCommand() {}
    virtual void undo();
    virtual void redo();
    virtual bool compress() const { return mCompress; }
  private:
    KadasStateStack* mStateStack;
    QSharedPointer<State> mPrevState, mNextState;
    bool mCompress;
  };

  KadasStateStack ( State* initialState, QObject* parent = 0 );
  ~KadasStateStack();
  void clear ( State* cleanState );
  void updateState ( State* newState, bool mergeable = false );
  void push ( StateChangeCommand* command );
  void undo();
  void redo();
  bool canUndo() const { return !mUndoStack.isEmpty(); }
  bool canRedo() const { return !mRedoStack.isEmpty(); }
  const State* state() const { return mState.data(); }
  State* mutableState() { return mState.data(); }

signals:
  void canUndoChanged ( bool );
  void canRedoChanged ( bool );
  void stateChanged();

protected:
  QSharedPointer<State> mState;

private:
  QStack<StateChangeCommand*> mUndoStack;
  QStack<StateChangeCommand*> mRedoStack;
};

#endif // KADASSTATESTACK_H
