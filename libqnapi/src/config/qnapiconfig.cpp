/*****************************************************************************
** QNapi
** Copyright (C) 2008-2017 Piotr Krzemiński <pio.krzeminski@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
*****************************************************************************/

#include "config/qnapiconfig.h"
#include <QTextStream>

QString QNapiConfig::toString() const {
  QString enabledEnginesStr, enginesCfgStr;

  QTextStream ees(&enabledEnginesStr);
  typedef QPair<QString, bool> EngineEnableCfg;
  foreach (EngineEnableCfg engineCfg, enabledEngines()) {
    QString engineName = engineCfg.first;
    bool engineEnabled = engineCfg.second;
    QString engineEnabledStr = engineEnabled ? "enabled" : "disabled";
    ees << " " << engineName << ": " << engineEnabledStr << Qt::endl;
  }

  QTextStream es(&enginesCfgStr);
  foreach (QString engineName, enginesConfig().keys()) {
    EngineConfig cfg = *enginesConfig().find(engineName);
    es << " " << engineName << ": " << Qt::endl
       << "  nick: " << cfg.nick() << Qt::endl
       << "  password: " << cfg.password() << Qt::endl;
  }

  QString s;
  QTextStream(&s) << "Version: " << version() << Qt::endl
                  << "First run? " << (firstrun() ? "yes" : "no") << Qt::endl
                  << Qt::endl
                  << "General config:" << Qt::endl
                  << generalConfig().toString() << Qt::endl
                  << "Enabled Engines:" << Qt::endl
                  << enabledEnginesStr << Qt::endl
                  << "Engines config:" << Qt::endl
                  << enginesCfgStr << Qt::endl
                  << "Post-processing config:" << Qt::endl
                  << postProcessingConfig().toString() << Qt::endl
                  << "Scan config:" << Qt::endl
                  << scanConfig().toString() << Qt::endl
                  << "Last open-dialog dir:" << Qt::endl
                  << lastOpenedDir() << Qt::endl;
  return s;
}
