/***************************************************************************
    kadasannotationstyleeditor.h
    ----------------------------
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

#ifndef KADASANNOTATIONSTYLEEDITOR_H
#define KADASANNOTATIONSTYLEEDITOR_H

#include <QWidget>

#include "kadas/gui/kadas_gui.h"

class QComboBox;
class QDoubleSpinBox;
class QFontComboBox;
class QLineEdit;
class QSpinBox;
class QgsAnnotationItem;
class QgsColorButton;

/**
 * \ingroup gui
 * \brief Per-type style editor widget for a stock QgsAnnotationItem.
 *
 * Concrete subclasses build the type-specific styling row (e.g. shape /
 * size / fill / stroke for markers, font / size / color / buffer for
 * point text) and translate between item state and widget state. The
 * map tool that hosts the bottom bar simply asks the controller for an
 * editor and connects to its two signals.
 *
 * Two signals separate live preview from committed edits:
 *  - \c previewChanged() fires for every interim change (e.g. a keystroke
 *    in the text field). The host should re-apply state to the item and
 *    refresh the layer, but should not push a new history entry.
 *  - \c committed() fires when a discrete edit is finalized. The host
 *    should apply, refresh, push history, and persist the style.
 */
class KADAS_GUI_EXPORT KadasAnnotationStyleEditor : public QWidget
{
    Q_OBJECT

  public:
    using QWidget::QWidget;

    //! Populates the widgets from \a item's current state.
    virtual void loadFromItem( const QgsAnnotationItem *item ) = 0;

    //! Writes the widgets' state back into \a item.
    virtual void applyToItem( QgsAnnotationItem *item ) const = 0;

  signals:
    //! Emitted continuously while the user is interacting (live preview).
    void previewChanged();
    //! Emitted when a discrete edit is finalized (push history + persist).
    void committed();
};


#ifndef SIP_RUN

class QBoxLayout;

/**
 * \brief Style editor for QgsAnnotationMarkerItem (shape, size, stroke, fill).
 */
class KadasMarkerStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasMarkerStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  private:
    QComboBox *mShapeCombo = nullptr;
    QSpinBox *mSizeSpin = nullptr;
    QDoubleSpinBox *mStrokeWidthSpin = nullptr;
    QgsColorButton *mFillColorBtn = nullptr;
    QgsColorButton *mStrokeColorBtn = nullptr;
    QComboBox *mStrokeStyleCombo = nullptr;
};

/**
 * \brief Style editor for QgsAnnotationLineItem (width, color, pen style).
 */
class KadasLineStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasLineStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  private:
    QDoubleSpinBox *mStrokeWidthSpin = nullptr;
    QgsColorButton *mStrokeColorBtn = nullptr;
    QComboBox *mStrokeStyleCombo = nullptr;
};

/**
 * \brief Style editor for QgsAnnotationPolygonItem (fill, stroke, brush).
 */
class KadasPolygonStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasPolygonStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  private:
    QDoubleSpinBox *mStrokeWidthSpin = nullptr;
    QgsColorButton *mFillColorBtn = nullptr;
    QgsColorButton *mStrokeColorBtn = nullptr;
    QComboBox *mStrokeStyleCombo = nullptr;
    QComboBox *mFillStyleCombo = nullptr;
};

/**
 * \brief Style editor for QgsAnnotationPointTextItem (text, font, color, buffer).
 *
 * The text input edits the item's text content and emits \c previewChanged()
 * on every keystroke and \c committed() on focus-out / Enter; the styling
 * widgets emit \c committed() directly.
 */
class KadasPointTextStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasPointTextStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  private:
    QLineEdit *mTextEdit = nullptr;
    QFontComboBox *mFontCombo = nullptr;
    QDoubleSpinBox *mSizeSpin = nullptr;
    QgsColorButton *mColorBtn = nullptr;
    QgsColorButton *mBufferColorBtn = nullptr;
    QDoubleSpinBox *mBufferWidthSpin = nullptr;
};

#endif // SIP_RUN

#endif // KADASANNOTATIONSTYLEEDITOR_H
