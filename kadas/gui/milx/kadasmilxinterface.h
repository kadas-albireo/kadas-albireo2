/***************************************************************************
    kadasmilxinterface.h
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


#ifndef KADASMILXINTERFACE_H
#define KADASMILXINTERFACE_H
#ifndef SIP_RUN

#include <qglobal.h>

typedef quint8 MilXServerRequest;

quint64 MILX_INTERFACE_VERSION = 202108191150;

MilXServerRequest MILX_REQUEST_INIT = 1; // {MILX_REQUEST_INIT, Lang:QString, InterfaceVersion:int64}

MilXServerRequest MILX_REQUEST_GET_SYMBOL_METADATA = 10; // {MILX_REQUEST_GET_SYMBOL_METADATA, SymbolXml:QString}
MilXServerRequest MILX_REQUEST_GET_SYMBOLS_METADATA = 11; // {MILX_REQUEST_GET_SYMBOLS_METADATA, SymbolXmls:QStringList}
MilXServerRequest MILX_REQUEST_GET_MILITARY_NAME = 12; // {MILX_REQUEST_GET_MILITARY_NAME, SymbolXml:QString}
MilXServerRequest MILX_REQUEST_GET_CONTROL_POINT_INDICES = 13; // {MILX_REQUEST_GET_CONTROL_POINT_INDICES, SymbolXml:QString, nPoints:int, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_GET_CONTROL_POINTS = 14;  // {MILX_REQUEST_GET_CONTROL_POINTS, SymbolXml:QString, Points:QList<QPoint>, Attributes:QList<QPair<int, double>>, isCorridor:bool, symbolSize:int, lineWidth:int, workMode: int}

MilXServerRequest MILX_REQUEST_APPEND_POINT = 20; // {MILX_REQUEST_APPEND_POINT, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, NewPoint:QPoint, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_INSERT_POINT = 21; // {MILX_REQUEST_INSERT_POINT, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, NewPoint:QPoint, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_MOVE_POINT = 22; // {MILX_REQUEST_MOVE_POINT, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int, NewPos:QPoint, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_MOVE_ATTRIBUTE_POINT = 23; // {MILX_REQUEST_MOVE_ATTRIBUTE_POINT, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int, NewPos:QPoint, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_CAN_DELETE_POINT = 24; // {MILX_REQUEST_CAN_DELETE_POINT, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_DELETE_POINT = 25; // {MILX_REQUEST_DELETE_POINT, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, index:int, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_EDIT_SYMBOL = 26; // {MILX_REQUEST_EDIT_SYMBOL, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, symbolSize:int, lineWidth:int, workMode: int, wid:WId}
MilXServerRequest MILX_REQUEST_CREATE_SYMBOL = 27; // {MILX_REQUEST_CREATE_SYMBOL, wid:WId, workMode:int}

MilXServerRequest MILX_REQUEST_UPDATE_SYMBOL = 30; // {MILX_REQUEST_UPDATE_SYMBOL, VisibleExtent:QRect, dpi:int, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, symbolSize:int, lineWidth:int, workMode: int, returnPoints:bool}
MilXServerRequest MILX_REQUEST_UPDATE_SYMBOLS = 31; // {MILX_REQUEST_UPDATE_SYMBOLS, VisibleExtent:QRect, dpi:int, scaleFactor:double, symbolSize:int, lineWidth:int, workMode: int, nSymbols:int, SymbolXml1:QString, Points1:QList<QPoint>, ControlPoints1:QList<int>, Attributes1:QList<QPair<int,double>>, finalized1:bool, colored1:bool, SymbolXml2:QString, Points2:QList<QPoint>, ControlPoints2:QList<int>, Attributes2:QList<QPair<int,double>>, finalized2:bool, colored2:bool, ...}

MilXServerRequest MILX_REQUEST_HIT_TEST = 40; // {MILX_REQUEST_HIT_TEST, SymbolXml:QString, Points:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, finalized:bool, colored:bool, clickPos:QPoint, symbolSize:int, lineWidth:int, workMode: int}
MilXServerRequest MILX_REQUEST_PICK_SYMBOL = 41; // {MILX_REQUEST_PICK_SYMBOL, ClickPos:QPoint, symbolSize:int, lineWidth:int, workMode: int, nSymbols:int, SymbolXml1:QString, Points1:QList<QPoint>, ControlPoints1:QList<int>, Attributes1:QList<QPair<int,double>>, finalized1:bool, colored1:bool, SymbolXml2:QString, Points2:QList<QPoint>, ControlPoints2:QList<int>, Attributes2:QList<QPair<int,double>>, finalized2:bool, colored2:bool, ...}

MilXServerRequest MILX_REQUEST_GET_LIBRARY_VERSION_TAGS = 50; // {MILX_REQUEST_GET_LIBRARY_VERSION_TAGS}
MilXServerRequest MILX_REQUEST_UPGRADE_MILXLY = 51; // {MILX_REQUEST_UPGRADE_MILXLY, InputXml:QString}
MilXServerRequest MILX_REQUEST_DOWNGRADE_MILXLY = 52; // {MILX_REQUEST_DOWNGRADE_MILXLY, InputXml:QString, MssVersion:QString}
MilXServerRequest MILX_REQUEST_VALIDATE_SYMBOLXML = 53; // {MILX_REQUEST_VALIDATE_SYMBOLXML, SymbolXml:QString, MssVersion:QString}

typedef quint8 MilXServerReply;

MilXServerReply MILX_REPLY_ERROR = 99; // {MILX_REPLY_ERROR, Message:QString}
MilXServerReply MILX_REPLY_INIT_OK = 101; // {MILX_REPLY_INIT_OK, Version:QString}

MilXServerReply MILX_REPLY_GET_SYMBOL_METADATA = 110; // {MILX_REPLY_GET_SYMBOL_METADATA, Name:QString, MilitaryName:QString, SvgXML:QByteArray, HasVariablePoints:bool, MinPointCount:int, SymbolType:QString}
MilXServerReply MILX_REPLY_GET_SYMBOLS_METADATA = 111; // {MILX_REPLY_GET_SYMBOLS_METADATA, count:int, Name1:QString, MilitaryName1:QString, SvgXML1:QByteArray, HasVariablePoints1:bool, MinPointCount1:int, SymbolType1:QString, Name2:QString, MilitaryName2:QString, SvgXML2:QByteArray, HasVariablePoints2:bool, MinPointCount2:int, SymbolType2:QString, ...}
MilXServerReply MILX_REPLY_GET_MILITARY_NAME = 112; // {MILX_REPLY_GET_MILITARY_NAME, MilitaryName:QString}
MilXServerReply MILX_REPLY_GET_CONTROL_POINT_INDICES = 113; // {MILX_REPLY_GET_CONTROL_POINT_INDICES, ControlPoints:QList<int>}
MilXServerReply MILX_REPLY_GET_CONTROL_POINTS = 114;  // {MILX_REPLY_GET_CONTROL_POINTS, Points:QList<QPoint>, ControlPoints:QList<int>}

MilXServerReply MILX_REPLY_APPEND_POINT = 120; // {MILX_REPLY_APPEND_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_INSERT_POINT = 121; // {MILX_REPLY_INSERT_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_MOVE_POINT = 122; // {MILX_REPLY_MOVE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_MOVE_ATTRIBUTE_POINT = 123; // {MILX_REPLY_MOVE_ATTRIBUTE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_CAN_DELETE_POINT = 124; // {MILX_REPLY_CAN_DELETE_POINT, canDelete:bool}
MilXServerReply MILX_REPLY_DELETE_POINT = 125; // {MILX_REPLY_DELETE_POINT, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_EDIT_SYMBOL = 126; // {MILX_REPLY_EDIT_SYMBOL, SymbolXml:QString, MilitaryName:QString, SvgString:QByteArray, Offset:QPoint, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>}
MilXServerReply MILX_REPLY_CREATE_SYMBOL = 127; // {MILX_REPLY_CREATE_SYMBOL, SymbolXml:QString, Name:QString, MilitaryName:QString, SvgXML:QByteArray, HasVariablePoints:bool, MinPointCount:int, SymbolType:QString}


MilXServerReply MILX_REPLY_UPDATE_SYMBOL = 130; // {MILX_REPLY_UPDATE_SYMBOL, SvgXml:QByteArray, Offset:QPoint[, AdjustedPoints:QList<QPoint>, ControlPoints:QList<int>, Attributes:QList<QPair<int,double>>, AttributePoints:QList<QPair<int,QPoint>>]} // Last four depending on whether returnPoints is true in the request
MilXServerReply MILX_REPLY_UPDATE_SYMBOLS = 131; // {MILX_REPLY_UPDATE_SYMBOLS, nSymbols:int, SvgXml1:QByteArray, Offset1:QPoint, SvgXml2:QByteArray, Offset2:QPoint, ...}

MilXServerReply MILX_REPLY_HIT_TEST = 140; // {MILX_REPLY_HIT_TEST, hitTestResult:bool}
MilXServerReply MILX_REPLY_PICK_SYMBOL = 141; // {MILX_REPLY_PICK_SYMBOL, SelectedSymbol:int}

MilXServerReply MILX_REPLY_GET_LIBRARY_VERSION_TAGS = 150; // {MILX_REPLY_GET_LIBRARY_VERSION_TAGS, versionTags:QStringList, versionNames:QStringList}
MilXServerReply MILX_REPLY_UPGRADE_MILXLY = 151; // {MILX_REPLY_UPGRADE_MILXLY, OutputXml:QString, valid:bool, messages:QString}
MilXServerReply MILX_REPLY_DOWNGRADE_MILXLY = 152; // {MILX_REPLY_DOWNGRADE_MILXLY, OutputXml:QString, valid:bool, messages:QString}
MilXServerReply MILX_REPLY_VALIDATE_SYMBOLXML = 153; // {MILX_REPLY_VALIDATE_SYMBOLXML, AdjustedSymbolXml:QString, valid:bool, messages:QString}

#endif // SIP_RUN
#endif // KADASMILXINTERFACE_H
