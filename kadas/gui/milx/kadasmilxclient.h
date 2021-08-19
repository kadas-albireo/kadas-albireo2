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
  QString symbolType;
};

struct KADAS_GUI_EXPORT KadasMilxSymbolSettings
{
  static constexpr int MinSymbolSize = 25;
  static constexpr int MaxSymbolSize = 150;
  static constexpr int DefaultSymbolSize = 60;

  static constexpr int MinLineWidth = 1;
  static constexpr int MaxLineWidth = 5;
  static constexpr int DefaultLineWidth = 2;

  enum WorkMode
  {
    WorkModeInternational = 0,
    WorkModeCH = 1
  };
  static constexpr WorkMode DefaultWorkMode = WorkModeCH;

  int symbolSize = DefaultSymbolSize;
  int lineWidth = DefaultLineWidth;
  WorkMode workMode = DefaultWorkMode;
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
    QProcess *mProcess = nullptr;
    QNetworkSession *mNetworkSession = nullptr;
    QTcpSocket *mTcpSocket = nullptr;
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

    static void setSymbolSize( int value ) { instance()->mGlobalSymbolSettings.symbolSize = value; }
    static void setLineWidth( int value ) { instance()->mGlobalSymbolSettings.lineWidth = value; }
    static void setWorkMode( KadasMilxSymbolSettings::WorkMode workMode ) { instance()->mGlobalSymbolSettings.workMode = workMode; }
    static const KadasMilxSymbolSettings &globalSymbolSettings() { return instance()->mGlobalSymbolSettings; }

    static bool init();
    static bool getSymbolMetadata( const QString &symbolId, KadasMilxSymbolDesc &result );
    static bool getSymbolsMetadata( const QStringList &symbolIds, QList<KadasMilxSymbolDesc> &result );
    static bool getMilitaryName( const QString &symbolXml, QString &militaryName );
    static bool getControlPointIndices( const QString &symbolXml, int nPoints, const KadasMilxSymbolSettings &settings, QList<int> &controlPoints );
    static bool getControlPoints( const QString &symbolXml, QList<QPoint> &points, const QList<QPair<int, double> > &attributes, QList<int> &controlPoints, bool isCorridor, const KadasMilxSymbolSettings &settings );

    static bool appendPoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const QPoint &newPoint, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result );
    static bool insertPoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const QPoint &newPoint, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result );
    static bool movePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int index, const QPoint &newPos, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result );
    static bool moveAttributePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int attr, const QPoint &newPos, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result );
    static bool canDeletePoint( const NPointSymbol &symbol, const KadasMilxSymbolSettings &settings, int index, bool &canDelete );
    static bool deletePoint( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, int index, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result );
    static bool editSymbol( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, QString &newSymbolXml, QString &newSymbolMilitaryName, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result, WId parentWid );
    static bool createSymbol( QString &symbolId, KadasMilxSymbolDesc &result, WId parentWid );

    static bool updateSymbol( const QRect &visibleExtent, int dpi, const NPointSymbol &symbol, const KadasMilxSymbolSettings &settings, NPointSymbolGraphic &result, bool returnPoints );
    static bool updateSymbols( const QRect &visibleExtent, int dpi, double scaleFactor, const QList<NPointSymbol> &symbols, const KadasMilxSymbolSettings &settings, QList<NPointSymbolGraphic> &result );

    static bool hitTest( const NPointSymbol &symbol, const QPoint &clickPos, const KadasMilxSymbolSettings &settings, bool &hitTestResult );
    static bool pickSymbol( const QList<NPointSymbol> &symbols, const QPoint &clickPos, const KadasMilxSymbolSettings &settings, int &selectedSymbol, QRect &boundingBox );

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
    KadasMilxSymbolSettings mGlobalSymbolSettings;

    KadasMilxClient();
    ~KadasMilxClient();
    static KadasMilxClient *instance();
    static QImage renderSvg( const QByteArray &xml );
    static void deserializeSymbol( QDataStream &ostream, NPointSymbolGraphic &result, bool deserializePoints = true );

    bool processRequest( const QByteArray &request, QByteArray &response, quint8 expectedReply, bool async = false );
};

#endif // SIP_RUN
#endif // KADASMILXCLIENT_H
