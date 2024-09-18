/***************************************************************************
 *  KadasCrashRpt.cpp                                                        *
 *  ---------------                                                        *
 *  begin                : Sep 15, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <GdbCrashHandler/GdbCrashHandler.hpp>

#include <qgis/qgslogger.h>
#include <qgis/qgsproject.h>
#include <qgis/qgssettings.h>

#include <kadas/core/kadas.h>
#include <kadascrashrpt.h>


bool KadasCrashRpt::install()
{
  GdbCrashHandler::Configuration config;
  config.applicationName = QString( "%1" ).arg( Kadas::KADAS_FULL_RELEASE_NAME );
  config.applicationVersion = QString( "%1 (%2/%3)" ).arg( Kadas::KADAS_VERSION ).arg( Kadas::KADAS_BUILD_DATE ).arg( Kadas::KADAS_DEV_VERSION );
  config.applicationIcon = ":/kadas/icon-60x60";
  config.submitAddress = QgsSettings().value( "/kadas/crashrpt_url" ).toString();
  config.submitMethod = GdbCrashHandler::Configuration::SubmitService;
  if ( !config.submitAddress.isEmpty() )
  {
    GdbCrashHandler::init( config );
  }
  return true;
}
