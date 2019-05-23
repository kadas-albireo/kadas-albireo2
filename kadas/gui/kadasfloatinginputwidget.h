/***************************************************************************
    kadasfloatinginputwidget.h
    --------------------------
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

#ifndef QGSMAPTOOLDRAWSHAPE_P_H
#define QGSMAPTOOLDRAWSHAPE_P_H

#include <QDoubleValidator>
#include <QGridLayout>
#include <QLineEdit>

#include <kadas/gui/kadas_gui.h>

class QgsMapCanvas;
class QgsPointXY;

class KADAS_GUI_EXPORT KadasFloatingInputWidgetField : public QLineEdit
{
    Q_OBJECT
  public:
    KadasFloatingInputWidgetField( QValidator* validator = new QDoubleValidator(), QWidget* parent = 0 ); // TODO Remove
    KadasFloatingInputWidgetField( int id, int decimals, double min, double max, QWidget* parent = nullptr );
    void setText( const QString& text ); // TODO Remove
    void setValue(double value);
    int id() const{ return mId; }

  signals:
    void inputChanged();
    void inputConfirmed();

  private:
    int mId = 0;
    int mDecimals = 0;
    QString mPrevText;

  private slots:
    void focusOutEvent( QFocusEvent *ev ) override;
    void checkInputChanged();
};

class KADAS_GUI_EXPORT KadasFloatingInputWidget : public QWidget
{
  public:
    KadasFloatingInputWidget( QgsMapCanvas* canvas );
    int addInputField( const QString& label, KadasFloatingInputWidgetField* widget, bool initiallyfocused = false );
    void removeInputField( int idx );
    void setInputFieldVisible( int idx, bool visible );
    void setFocusedInputField( KadasFloatingInputWidgetField* widget );
    const QList<KadasFloatingInputWidgetField*>& inputFields() const { return mInputFields; }
    KadasFloatingInputWidgetField* focusedInputField() const { return mFocusedInput; }
    bool eventFilter( QObject *obj, QEvent *ev ) override;

    void adjustCursorAndExtent(const QgsPointXY &geoPos );

  protected:
    bool focusNextPrevChild( bool next ) override;
    void keyPressEvent( QKeyEvent *ev ) override;
    void showEvent( QShowEvent *event ) override;
    void hideEvent( QHideEvent *event ) override;

  private:
    int mInitiallyFocusedInput = -1;
    QgsMapCanvas* mCanvas;
    KadasFloatingInputWidgetField* mFocusedInput;
    QList<KadasFloatingInputWidgetField*> mInputFields;

};

#endif // QGSMAPTOOLDRAWSHAPE_P_H
