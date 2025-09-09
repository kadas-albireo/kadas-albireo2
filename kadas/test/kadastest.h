/***************************************************************************
  qgstest - %{Cpp:License:ClassName}

 ---------------------
 begin                : 7.09.2025
 copyright            : (C) 2025 by Damiano Lombardi
 email                : damiano@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef KADASTEST_H
#define KADASTEST_H

#include <QtTest/QTest>

#include <kadas/app/kadasapplication.h>

#define KADASTEST_MAIN( TestObject )           \
  QT_BEGIN_NAMESPACE                           \
  QT_END_NAMESPACE                             \
  int main( int argc, char *argv[] )           \
  {                                            \
    KadasApplication app( argc, argv );        \
    app.init();                                \
    app.setAttribute( Qt::AA_Use96Dpi, true ); \
    QTEST_DISABLE_KEYPAD_NAVIGATION            \
    TestObject tc;                             \
    QTEST_SET_MAIN_SOURCE_PATH                 \
    return QTest::qExec( &tc, argc, argv );    \
  }


#endif // QGSTEST_H
