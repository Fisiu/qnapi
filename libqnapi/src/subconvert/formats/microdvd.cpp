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

#include "microdvd.h"

#include <QRegularExpression>

bool MicroDVDSubtitleFormat::detect(const QStringList &lines) const {
  foreach (QString line, lines) {
    if (!line.trimmed().isEmpty()) {
      QRegularExpression r("^\\{(\\d+)\\}\\{(\\d+)\\}(.*)\\z");
      return r.match(line).hasMatch();
    }
  }

  return false;
}

SubFile MicroDVDSubtitleFormat::decode(const QStringList &lines) const {
  SubFile sf;

  foreach (QString line, lines) {
    if (!line.trimmed().isEmpty()) {
      QRegularExpression r("^\\{(\\d+)\\}\\{(\\d+)\\}(.*)\\z");
      const auto match = r.match(line);
      if (match.hasMatch()) {
        SubEntry se;
        se.frameStart = match.captured(1).toLong();
        se.frameStop = match.captured(2).toLong();
        QString tokenStream = match.captured(3);

        se.tokens = decodeTokenStream(tokenStream);

        sf.entries.push_back(se);
      }
    }
  }

  return sf;
}

QStringList MicroDVDSubtitleFormat::encode(const SubFile &subFile) const {
  QStringList lines;
  foreach (const SubEntry &entry, subFile.entries) {
    lines.append(encodeEntry(entry));
  }
  return lines;
}

QString MicroDVDSubtitleFormat::encodeEntry(const SubEntry &entry) const {
  QString tokensBuff;
  foreach (const SubToken &token, entry.tokens) {
    tokensBuff += encodeToken(token);
  }
  return QString("{%1}{%2}%3")
      .arg(entry.frameStart)
      .arg(entry.frameStop)
      .arg(tokensBuff);
}

QString MicroDVDSubtitleFormat::encodeToken(const SubToken &entry) const {
  switch (entry.type) {
    case STT_WS:
      return " ";
    case STT_WORD:
      return entry.payload;
    case STT_NEWLINE:
      return "|";
    case STT_BOLD:
      return "{y:b}";
    case STT_ITALIC:
      return "{y:i}";
    case STT_UNDERLINE:
      return "{y:u}";
    case STT_FONTCOLOR:
      return QString("{c:$%1}").arg(entry.payload);
    default:
      return "";
  }
}
