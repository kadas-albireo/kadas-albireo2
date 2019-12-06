/***************************************************************************
    kadasmilxclient.h
    -----------------
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


#ifndef KADASMILXCLIENT_H
#define KADASMILXCLIENT_H

#include <qglobal.h>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QPoint>
#include <QPixmap>
#include <QThread>

#include <kadas/gui/kadas_gui.h>

class QNetworkSession;
class QRect;
class QProcess;
class QStringList;
class QTcpSocket;

struct KADAS_GUI_EXPORT KadasMilxSymbolDesc
{
  QString symbolXml;
  QString name;
  QString militaryName;
  QImage icon;
  bool hasVariablePoints;
  int minNumPoints;
};

#ifndef SIP_RUN

typedef int KadasMilxAttrType;
constexpr KadasMilxAttrType MilxAttributeUnknown = 0;
constexpr KadasMilxAttrType MilxAttributeWidth = 1;
constexpr KadasMilxAttrType MilxAttributeLength = 2;
constexpr KadasMilxAttrType MilxAttributeRadius = 4;
constexpr KadasMilxAttrType MilxAttributeAttitude = 8;

class KADAS_GUI_EXPORT KadasMilxClientWorker : public QObject
{
    Q_OBJECT
  public:
    KadasMilxClientWorker( bool sync );

  public slots:
    bool initialize();
    bool getCurrentLibraryVersionTag( QString &versionTag );
    bool processRequest( const QByteArray &request, QByteArray &response, quint8 expectedReply, bool forceSync = false );
    void cleanup();

  private:
    bool mSync;
    QProcess *mProcess;
    QNetworkSession *mNetworkSession;
    QTcpSocket *mTcpSocket;
    QString mLastError;
    QString mLibraryVersionTag;

  private slots:
    void handleSocketError();
};


class KADAS_GUI_EXPORT KadasMilxClient : public QThread
{
    Q_OBJECT
  public:
    struct NPointSymbol
    {
      NPointSymbol( const QString &_xml, const QList<QPoint> &_points, const QList<int> &_controlPoints, const QList< QPair<int, double> > &_attributes, bool _finalized, bool _colored )
        : xml( _xml ), points( _points ), controlPoints( _controlPoints ), attributes( _attributes ), finalized( _finalized ), colored( _colored ) {}

      QString xml;
      QList<QPoint> points;
      QList<int> controlPoints;
      QList< QPair<int, double> > attributes;
      bool finalized;
      bool colored;
    };

    struct NPointSymbolGraphic
    {
      QImage graphic;
      QPoint offset;
      QList<QPoint> adjustedPoints;
      QList<int> controlPoints;
      QMap<KadasMilxAttrType, double> attributes;
      QMap<KadasMilxAttrType, QPoint> attributePoints;
    };

    static QString attributeName( KadasMilxAttrType idx );
    static KadasMilxAttrType attributeIdx( const QString &name );

    static void setSymbolSize( int value ) { instance()->mSymbolSize = value; instance()->setSymbolOptions( instance()->mSymbolSize, instance()->mLineWidth, instance()->mWorkMode ); }
    static void setLineWidth( int value ) { instance()->mLineWidth = value; instance()->setSymbolOptions( instance()->mSymbolSize, instance()->mLineWidth, instance()->mWorkMode ); }
    static int getSymbolSize() { return instance()->mSymbolSize; }
    static int getLineWidth() { return instance()->mLineWidth; }
    static void setWorkMode( int workMode ) { instance()->mWorkMode = workMode; instance()->setSymbolOptions( instance()->mSymbolSize, instance()->mLineWidth, instance()->mWorkMode ); }

    static bool init();
    static bool getSymbolMetadata( const QString &symbolId, KadasMilxSymbolDesc &result );
    static bool getSymbolsMetadata( const QStringList &symbolIds, QList<KadasMilxSymbolDesc> &result );
    static bool getMilitaryName( const QString &symbolXml, QString &militaryName );
    static bool getControlPointIndices( const QString &symbolXml, int nPoints, QList<int> &controlPoints );
    static bool getControlPoints( const QString &symbolXml, QList<QPoint> &points, const QList<QPair<int, double> > &attributes, QList<int> &controlPoints, bool isCorridor );

    static bool appendPoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const QPoint &newPoint, NPointSymbolGraphic &result );
    static bool insertPoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const QPoint &newPoint, NPointSymbolGraphic &result );
    static bool movePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int index, const QPoint &newPos, NPointSymbolGraphic &result );
    static bool moveAttributePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int attr, const QPoint &newPos, NPointSymbolGraphic &result );
    static bool canDeletePoint( const NPointSymbol &symbol, int index, bool &canDelete );
    static bool deletePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int index, NPointSymbolGraphic &result );
    static bool editSymbol( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, QString &newSymbolXml, QString &newSymbolMilitaryName, NPointSymbolGraphic &result, WId parentWid );
    static bool createSymbol( QString &symbolId, KadasMilxSymbolDesc &result, WId parentWid );

    static bool updateSymbol( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, NPointSymbolGraphic &result, bool returnPoints );
    static bool updateSymbols( const QRect &visibleExtent, int dpi, double scaleFactor, const QList<NPointSymbol> &symbols, QList<NPointSymbolGraphic> &result );

    static bool hitTest( const NPointSymbol &symbol, const QPoint &clickPos, bool &hitTestResult );
    static bool pickSymbol( const QList<NPointSymbol> &symbols, const QPoint &clickPos, int &selectedSymbol, QRect &boundingBox );

    static bool getCurrentLibraryVersionTag( QString &versionTag );
    static bool getSupportedLibraryVersionTags( QStringList &versionTags, QStringList &versionNames );
    static bool upgradeMilXFile( const QString &inputXml, QString &outputXml, bool &valid, QString &messages );
    static bool downgradeMilXFile( const QString &inputXml, QString &outputXml, const QString &mssVersion, bool &valid, QString &messages );
    static bool validateSymbolXml( const QString &symbolXml, const QString &mssVersion, QString &adjustedSymbolXml, bool &valid, QString &messages );

    static void quit() { delete instance(); }

  private:
    static KadasMilxClient *sInstance;
    KadasMilxClientWorker mAsyncWorker;
    KadasMilxClientWorker mSyncWorker;
    int mSymbolSize;
    int mLineWidth;
    int mWorkMode;

    KadasMilxClient();
    ~KadasMilxClient();
    static KadasMilxClient *instance();
    static QImage renderSvg( const QByteArray &xml );
    static void deserializeSymbol( QDataStream &ostream, NPointSymbolGraphic &result, bool deserializePoints = true );

    bool processRequest( const QByteArray &request, QByteArray &response, quint8 expectedReply, bool async = false );
    bool setSymbolOptions( int symbolSize, int lineWidth, int workMode );
};

#endif // SIP_RUN
#endif // KADASMILXCLIENT_H
