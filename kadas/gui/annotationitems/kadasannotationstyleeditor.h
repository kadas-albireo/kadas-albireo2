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
class QPlainTextEdit;
class QSpinBox;
class QgsAnnotationItem;
class QgsColorButton;
class QgsSvgSelectorWidget;

/**
 * \ingroup gui
 * \brief Per-type style editor widget for a stock QgsAnnotationItem.
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
 * \brief Style editor for the Kadas pin (SVG marker: size, fill color).
 */
class KadasPinStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasPinStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  protected:
    bool eventFilter( QObject *watched, QEvent *event ) override;

  private:
    QLineEdit *mTitleEdit = nullptr;
    QPlainTextEdit *mDescriptionEdit = nullptr;
    QSpinBox *mSizeSpin = nullptr;
    QgsColorButton *mFillColorBtn = nullptr;
};

/**
 * \brief Style editor for the custom SVG marker (QGIS SVG picker, size, fill color).
 */
class KadasSvgMarkerStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasSvgMarkerStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  private:
    QgsSvgSelectorWidget *mSvgSelector = nullptr;
    QSpinBox *mSizeSpin = nullptr;
    QgsColorButton *mFillColorBtn = nullptr;
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


class QCheckBox;
class QPushButton;
class QToolButton;

/**
 * \brief Style editor for QgsAnnotationPictureItem with balloon callout.
 */
class KadasPictureStyleEditor : public KadasAnnotationStyleEditor
{
    Q_OBJECT

  public:
    explicit KadasPictureStyleEditor( QWidget *parent = nullptr );

    void loadFromItem( const QgsAnnotationItem *item ) override;
    void applyToItem( QgsAnnotationItem *item ) const override;

  private:
    QToolButton *mChangeImageBtn = nullptr;
    QSpinBox *mWidthSpin = nullptr;
    QSpinBox *mHeightSpin = nullptr;
    QCheckBox *mLockAspectBox = nullptr;
    QCheckBox *mShowCalloutBox = nullptr;
    QgsColorButton *mFillColorBtn = nullptr;
    QgsColorButton *mStrokeColorBtn = nullptr;
    QDoubleSpinBox *mStrokeWidthSpin = nullptr;
    QDoubleSpinBox *mWedgeWidthSpin = nullptr;
    QString mPath;
    // Aspect ratio captured when the lock was toggled on.
    double mAspectLockRatio = 0.0;
};

#endif // SIP_RUN

#endif // KADASANNOTATIONSTYLEEDITOR_H
