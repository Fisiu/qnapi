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

#include "config/postprocessingconfig.h"
#include <QTextStream>

QString PostProcessingConfig::toString() const {
  QString s;
  QTextStream(&s) << "enabled: " << enabled() << Qt::endl
                  << "encodingChangeMethod: " << encodingChangeMethod() << Qt::endl
                  << "encodingFrom: " << encodingFrom() << Qt::endl
                  << "encodingAutoDetectFrom: " << encodingAutoDetectFrom()
                  << Qt::endl
                  << "encodingTo: " << encodingTo() << Qt::endl
                  << "showAllEncodings: " << showAllEncodings() << Qt::endl
                  << "subFormat: " << subFormat() << Qt::endl
                  << "subExtension: " << subExtension() << Qt::endl
                  << "skipConvertAds: " << skipConvertAds() << Qt::endl
                  << "removeLines: " << removeLines() << Qt::endl
                  << "removeLinesWords: " << removeLinesWords().join("; ") << Qt::endl;
  return s;
}
