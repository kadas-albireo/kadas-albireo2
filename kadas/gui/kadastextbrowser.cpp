/***************************************************************************
    kadastextbrowser.cpp
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

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QMenu>
#include <QMimeData>

#include <kadas/gui/kadastextbrowser.h>


KadasTextBrowser::KadasTextBrowser( QWidget *parent )
  : QTextBrowser( parent )
{
  setOpenLinks( true );
  setOpenExternalLinks( true );
  setReadOnly( false );
}

void KadasTextBrowser::contextMenuEvent( QContextMenuEvent *e )
{
  QString anchor = anchorAt( e->pos() );
  if ( !anchor.isEmpty() )
  {
    QMenu menu;
    QAction *openAction = menu.addAction( tr( "Open link..." ) );
    QAction *copyAction = menu.addAction( tr( "Copy link location" ) );
    QAction *clickedAction = menu.exec( e->globalPos() );
    if ( clickedAction == openAction )
    {
      QDesktopServices::openUrl( QUrl::fromUserInput( anchor ) );
    }
    else if ( clickedAction == copyAction )
    {
      QApplication::clipboard()->setText( anchor );
    }
    e->accept();
  }
  else
  {
    QTextBrowser::contextMenuEvent( e );
  }
}

void KadasTextBrowser::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Control )
  {
    setReadOnly( true );
    e->accept();
    return;
  }
  setReadOnly( false );
  QTextBrowser::keyPressEvent( e );
}

void KadasTextBrowser::keyReleaseEvent( QKeyEvent *e )
{
  setReadOnly( false );
  QTextBrowser::keyReleaseEvent( e );
  if ( e->key() == Qt::Key_Space || e->key() == Qt::Key_Return || e->key() == Qt::Key_Tab )
  {
    QTextCursor cursor = textCursor();
    cursor.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor );
    cursor.setCharFormat( QTextCharFormat() );
    cursor.setPosition( cursor.position() ); // Move anchor
    QTextCursor c2 = document()->find( QRegExp( "^|\\s" ), cursor.position(), QTextDocument::FindBackward );
    if ( c2.isNull() )
    {
      c2.setPosition( 0 );
    }
    cursor.movePosition( QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, cursor.position() - c2.position() );
    if ( urlRegEx().indexIn( cursor.selectedText() ) != -1 )
    {
      cursor.insertHtml( QString( "<a href=\"%1\">%1</a>" ).arg( cursor.selectedText() ) );
      cursor.movePosition( QTextCursor::NextCharacter );
      setTextCursor( cursor );
    }
  }
}

void KadasTextBrowser::insertFromMimeData( const QMimeData *source )
{
  QString text = source->text();
  text.replace( urlRegEx(), "<a href=\"\\1\">\\1</a>" );
  insertHtml( text );
}

const QRegExp &KadasTextBrowser::urlRegEx() const
{
  static QRegExp re( "((http:\\/\\/www\\.|https:\\/\\/www\\.|http:\\/\\/|https:\\/\\/)?[a-z0-9]+([\\-\\.]{1}[a-z0-9]+)*\\.[a-z]{2,5}(:[0-9]{1,5})?(\\/.*)?)" );
  return re;
}
