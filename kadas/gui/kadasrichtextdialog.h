/***************************************************************************
    kadasrichtextdialog.h
    ---------------------
    copyright            : (C) 2026 by Denis Rouzaud
    email                : denis at opengis dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KADASRICHTEXTDIALOG_H
#define KADASRICHTEXTDIALOG_H

#include <QDialog>
#include <QString>

#include "kadas/gui/kadas_gui.h"

class QTextDocument;
class QgsRichTextEditor;

/**
 * \ingroup gui
 * \brief Dialog for editing a rich-text field stored in a Kadas project.
 *
 * Wraps QgsRichTextEditor, whose toolbar needs far more room than an inline form
 * row affords. On accept, images the editor embedded inline are moved into the
 * project archive (see KadasAttachmentUtils::materializeInlineImages) and the
 * document skeleton is stripped, so html() returns compact markup that can be
 * dropped straight into a larger document such as a map tooltip.
 */
class KADAS_GUI_EXPORT KadasRichTextDialog : public QDialog
{
    Q_OBJECT

  public:
    //! Edits \a html, which may equally be plain text, under the window title \a title.
    KadasRichTextDialog( const QString &title, const QString &html, QWidget *parent = nullptr );

    //! The edited markup. Only meaningful once exec() has returned QDialog::Accepted.
    QString html() const { return mHtml; }

    /**
     * Turns bare URLs in \a document into anchors, leaving text the author
     * already linked alone.
     *
     * Applied to what the dialog stores rather than to what a viewer renders, so
     * a typed URL is a real link from then on — in the tooltip, and to whatever
     * else reads the markup.
     */
    static void linkifyBareUrls( QTextDocument *document );

  private slots:
    //! Downloads an image the user names by URL into the project, and inserts it at the cursor.
    void insertImageFromUrl();

  public slots:
    void accept() override;

  private:
    //! Lays its items out left to right, wrapping to a new row when the next one would not fit.
    class FlowLayout;

    //! Keeps only what is inside <body> of \a html.
    static QString documentBody( const QString &html );

    //! Rehosts the editor's toolbar in a wrapping layout with \a iconSize icons, dropping the tools this dialog has no use for and adding those it needs.
    void reflowToolbar( int iconSize );

    QgsRichTextEditor *mEditor = nullptr;
    QString mHtml;
    //! Guards the textChanged handler against the edit it makes itself.
    bool mClampingImages = false;
};

#endif // KADASRICHTEXTDIALOG_H
