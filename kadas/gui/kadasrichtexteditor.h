/***************************************************************************
    kadasrichtexteditor.h
    ---------------------
    copyright            : (C) 2021 by Sandro Mani
                           (C) 2016 The Qt Company Ltd.
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

#ifndef KADASRICHTEXTEDITOR_H
#define KADASRICHTEXTEDITOR_H

#include <QDialog>
#include <QTextEdit>
#include <QToolBar>

#include <kadas/gui/kadas_gui.h>

class QComboBox;
class QDialogButtonBox;
class QLineEdit;
class QSpinBox;
class QTabWidget;
class QToolButton;


class KADAS_GUI_EXPORT KadasRichTextEditor : public QTextEdit
{
    Q_OBJECT
  public:
    explicit KadasRichTextEditor( QWidget *parent = nullptr );
    QVariant loadResource( int type, const QUrl &url ) override;

  public slots:
    void setFontBold( bool b );
    void setFontPointSize( double );
    void setText( const QString &text );
    QString text( Qt::TextFormat format ) const;

  signals:
    void stateChanged();

  private slots:
    void checkImageRemoved( int position, int charsRemoved, int charsAdded );
};


class KADAS_GUI_EXPORT KadasColorAction : public QAction
{
    Q_OBJECT

  public:
    KadasColorAction( QObject *parent );
    const QColor &color() const { return m_color; }
    void setColor( const QColor &color );

  signals:
    void colorChanged( const QColor &color );

  private slots:
    void chooseColor();

  private:
    QColor m_color;
};

class KADAS_GUI_EXPORT KadasAddLinkDialog : public QDialog
{
    Q_OBJECT

  public:
    KadasAddLinkDialog( KadasRichTextEditor *editor, QWidget *parent = nullptr );
    int showDialog();

  public slots:
    void accept() override;

  private:
    KadasRichTextEditor *m_editor = nullptr;
    QLineEdit *m_titleInput = nullptr;
    QLineEdit *m_urlInput = nullptr;
};

class KADAS_GUI_EXPORT KadasAddImageDialog : public QDialog
{
    Q_OBJECT

  public:
    KadasAddImageDialog( KadasRichTextEditor *editor, QWidget *parent = nullptr );

  public slots:
    void accept() override;

  private:
    KadasRichTextEditor *m_editor = nullptr;
    QLineEdit *m_urlInput = nullptr;
    QSpinBox *m_widthInput = nullptr;
    QSpinBox *m_heightInput = nullptr;
    QToolButton *m_aspectButton = nullptr;
    QDialogButtonBox *m_bbox = nullptr;
    QSize m_imageSize;

  private slots:
    void showFileDialog();
    void widthChanged();
    void heightChanged();
};

class KADAS_GUI_EXPORT KadasRichTextEditorToolBar : public QToolBar
{
    Q_OBJECT
  public:
    KadasRichTextEditorToolBar( KadasRichTextEditor *editor,
                                QWidget *parent = nullptr );

  public slots:
    void updateActions();

  private slots:
    void alignmentActionTriggered( QAction *action );
    void sizeInputActivated( const QString &size );
    void colorChanged( const QColor &color );
    void setVAlignSuper( bool super );
    void setVAlignSub( bool sub );
    void insertLink();
    void insertImage();

  private:
    QAction *m_bold_action;
    QAction *m_italic_action;
    QAction *m_underline_action;
    QAction *m_valign_sup_action;
    QAction *m_valign_sub_action;
    QAction *m_align_left_action;
    QAction *m_align_center_action;
    QAction *m_align_right_action;
    QAction *m_align_justify_action;
    QAction *m_link_action;
    QAction *m_image_action;
    QAction *m_simplify_richtext_action;
    KadasColorAction *m_color_action;
    QComboBox *m_font_size_input;

    KadasRichTextEditor *m_editor = nullptr;
};

#endif // KADASRICHTEXTEDITOR_H
