/***************************************************************************
    kadasstatehistory.cpp
    ---------------------
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

#include <kadas/core/kadasstatehistory.h>

KadasStateHistory::KadasStateHistory( QObject* parent )
    : QObject( parent )
{}

KadasStateHistory::~KadasStateHistory()
{
  qDeleteAll( mStates );
}

void KadasStateHistory::clear()
{
  qDeleteAll( mStates );
  mStates.clear();
  mCurrent = -1;
  emit canUndoChanged( false );
  emit canRedoChanged( false );
}

void KadasStateHistory::undo()
{
  if( canUndo() )
  {
    emit stateChanged(mStates[--mCurrent]);
  }
  emit canUndoChanged( canUndo() );
  emit canRedoChanged( canRedo() );
}

void KadasStateHistory::redo()
{
  if( canRedo() )
  {
    emit stateChanged(mStates[++mCurrent]);
  }
  emit canUndoChanged( canUndo() );
  emit canRedoChanged( canRedo() );
}

void KadasStateHistory::push( State* state )
{
  mStates.resize(++mCurrent);
  mStates.append(state);
  emit canUndoChanged( canUndo() );
  emit canRedoChanged( canRedo() );
}
