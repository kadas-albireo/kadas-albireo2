/***************************************************************************
    kadasprojecttemplateselectiondialog.cpp
    ---------------------------------------
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

#include <QDialogButtonBox>
#include <QFileSystemModel>
#include <QKeyEvent>
#include <QPushButton>
#include <QSettings>
#include <QTreeView>
#include <QVBoxLayout>

#include <kadas/core/kadas.h>
//#include "qgsapplication.h"
//#include "qgisapp.h"

#include "kadasprojecttemplateselectiondialog.h"

KadasProjectTemplateSelectionDialog::KadasProjectTemplateSelectionDialog( QWidget *parent )
    : QDialog( parent )
{
  setupUi( this );

  mModel = new QFileSystemModel( this );
  mModel->setNameFilters( QStringList() << "*.qgs" );
  mModel->setNameFilterDisables( false );
  mModel->setReadOnly( true );

  mTreeView->setModel( mModel );
  mTreeView->setRootIndex( mModel->setRootPath( Kadas::projectTemplatesPath() ) );
  for ( int i = 1, n = mModel->columnCount(); i < n; ++i )
  {
    mTreeView->setColumnHidden( i, true );
  }
  connect( mTreeView, &QTreeView::clicked, this, &KadasProjectTemplateSelectionDialog::itemClicked );
  connect( mTreeView, &QTreeView::doubleClicked, this, &KadasProjectTemplateSelectionDialog::itemDoubleClicked );

  mCreateButton = mButtonBox->addButton( tr( "Create" ), QDialogButtonBox::AcceptRole );
  mCreateButton->setEnabled( false );
  connect( mCreateButton, &QAbstractButton::clicked, this, &KadasProjectTemplateSelectionDialog::createProject );
}

void KadasProjectTemplateSelectionDialog::itemClicked( const QModelIndex& index )
{
  mCreateButton->setEnabled( mModel->fileInfo( index ).isFile() );
}

void KadasProjectTemplateSelectionDialog::itemDoubleClicked( const QModelIndex& index )
{
  if ( mModel->fileInfo( index ).isFile() )
  {
    createProject();
  }
}

void KadasProjectTemplateSelectionDialog::createProject()
{
  mSelectedTemplate = mModel->fileInfo( mTreeView->currentIndex() ).absoluteFilePath();
  accept();
}
