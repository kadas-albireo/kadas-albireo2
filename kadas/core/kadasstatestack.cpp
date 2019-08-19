/***************************************************************************
    kadasstatestack.cpp
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

#include <kadas/core/kadasstatestack.h>

KadasStateStack::KadasStateStack ( State* initialState, QObject* parent )
  : QObject ( parent ), mState ( initialState )
{}

KadasStateStack::~KadasStateStack()
{
  qDeleteAll ( mUndoStack );
  qDeleteAll ( mRedoStack );
}

void KadasStateStack::clear ( State* cleanState )
{
  qDeleteAll ( mUndoStack );
  mUndoStack.clear();
  qDeleteAll ( mRedoStack );
  mRedoStack.clear();
  emit canUndoChanged ( false );
  emit canRedoChanged ( false );
  mState = QSharedPointer<State> ( cleanState );
  emit stateChanged();
}

void KadasStateStack::undo()
{
  while ( !mUndoStack.isEmpty() ) {
    StateChangeCommand* command = mUndoStack.pop();
    command->undo();
    mRedoStack.push ( command );
    if ( !command->compress() ) {
      break;
    }
  }
  emit canUndoChanged ( !mUndoStack.isEmpty() );
  emit canRedoChanged ( !mRedoStack.isEmpty() );
}

void KadasStateStack::redo()
{
  while ( !mRedoStack.isEmpty() ) {
    StateChangeCommand* command = mRedoStack.pop();
    command->redo();
    mUndoStack.push ( command );
    if ( mRedoStack.isEmpty() || !mRedoStack.top()->compress() ) {
      break;
    }
  }
  emit canUndoChanged ( !mUndoStack.isEmpty() );
  emit canRedoChanged ( !mRedoStack.isEmpty() );
}

void KadasStateStack::updateState ( State* newState, bool mergeable )
{
  push ( new StateChangeCommand ( this, newState, mergeable ) );
}

void KadasStateStack::push ( StateChangeCommand* command )
{
  qDeleteAll ( mRedoStack );
  mRedoStack.clear();
  mUndoStack.push ( command );
  command->redo();
  emit canUndoChanged ( true );
}
///////////////////////////////////////////////////////////////////////////////

KadasStateStack::StateChangeCommand::StateChangeCommand ( KadasStateStack* stateStack, State* nextState, bool compress )
  : mStateStack ( stateStack ), mPrevState ( stateStack->mState ), mNextState ( nextState ), mCompress ( compress )
{
}

void KadasStateStack::StateChangeCommand::undo()
{
  mStateStack->mState = mPrevState;
  emit mStateStack->stateChanged();
}

void KadasStateStack::StateChangeCommand::redo()
{
  mStateStack->mState = mNextState;
  emit mStateStack->stateChanged();
}
