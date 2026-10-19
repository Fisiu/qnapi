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

#include <optional>
#include "subtitleconverter.h"
#include <QFile>
#include <QDebug>
#include <QTextCodec>
#include <cmath>
#include "libqnapi.h"
#include "movieinfo/movieinfoprovider.h"
#include "utils/encodingutils.h"

SubtitleConverter::SubtitleConverter(
    QSharedPointer<const SubtitleFormatsRegistry> subtitleFormatsRegistry,
    QSharedPointer<const MovieInfoProvider> movieInfoProvider,
    bool skipConvertAds)
    : subtitleFormatsRegistry(subtitleFormatsRegistry),
      movieInfoProvider(movieInfoProvider),
      skipConvertAds(skipConvertAds) {}

QString SubtitleConverter::detectFormat(const QString &subtitleFile) const {
  const QStringList &lines = readFile(subtitleFile, "UTF-8", 15);
  return detectFormat(lines);
}

QString SubtitleConverter::detectFormat(
    const QStringList &subtitleLines) const {
  foreach (QString format, subtitleFormatsRegistry->listFormatNames()) {
    if (subtitleFormatsRegistry->select(format)->detect(subtitleLines)) {
      return format;
    }
  }
  return "";
}

bool SubtitleConverter::convertSubtitles(QString subtitleFile,
                                         QString targetFormatName,
                                         QString targetFileName,
                                         double movieFPS, double fpsRatio,
                                         double delayOffset) const {
  return convertSubtitles(subtitleFile, targetFormatName, targetFileName,
                          [=]() -> double { return movieFPS; }, fpsRatio,
                          delayOffset);
}

bool SubtitleConverter::convertSubtitles(QString subtitleFile,
                                         QString targetFormatName,
                                         QString targetFileName,
                                         QString movieFile) const {
  return convertSubtitles(
      subtitleFile, targetFormatName, targetFileName, [&]() -> double {
        const std::optional<MovieInfo> mmi =
            movieInfoProvider->getMovieInfo(movieFile);
        return mmi ? mmi.value().frameRate() : 0.0;
      });
}

bool SubtitleConverter::convertSubtitles(QString subtitleFile,
                                         QString targetFormatName,
                                         QString targetFileName,
                                         std::function<double()> determineFPS,
                                         double fpsRatio,
                                         double delayOffset) const {
  EncodingUtils eu;
  QString encoding = eu.detectFileEncoding(subtitleFile);

  QStringList subtitleLines = readFile(subtitleFile, encoding);

  QString detectedFormat = detectFormat(subtitleLines);
  if (detectedFormat.isEmpty()) return false;

  QSharedPointer<const SubtitleFormat> inputFormat =
      subtitleFormatsRegistry->select(detectedFormat);
  QSharedPointer<const SubtitleFormat> targetFormat =
      subtitleFormatsRegistry->select(targetFormatName);

  SubFile sf = inputFormat->decode(subtitleLines);
  double frameRate = 0.0;

  if (inputFormat->isTimeBased() != targetFormat->isTimeBased()) {
    frameRate = determineFPS();
    if (frameRate <= 0.0) return false;

    if (targetFormat->isTimeBased()) {
      // convert frames to timestamps
      for (int i = 0; i < sf.entries.size(); ++i) {
        sf.entries[i].frameStart =
            frame2ts(sf.entries[i].frameStart, frameRate);
        sf.entries[i].frameStop = frame2ts(sf.entries[i].frameStop, frameRate);
      }
    } else {
      // convert timestamps to frames
      for (int i = 0; i < sf.entries.size(); ++i) {
        sf.entries[i].frameStart =
            ts2frame(sf.entries[i].frameStart, frameRate);
        sf.entries[i].frameStop = ts2frame(sf.entries[i].frameStop, frameRate);
      }
    }
  }

  if (fpsRatio != 1.0) {
    for (int i = 0; i < sf.entries.size(); ++i) {
      sf.entries[i].frameStart =
          (long)floor(fpsRatio * sf.entries[i].frameStart);
      sf.entries[i].frameStop = (long)floor(fpsRatio * sf.entries[i].frameStop);
    }
  }

  if (delayOffset != 0.0) {
    long offset = (long)(delayOffset * 1000.0);

    if (targetFormat->isTimeBased()) {
      for (int i = 0; i < sf.entries.size(); ++i) {
        sf.entries[i].frameStart =
            std::max(0L, sf.entries[i].frameStart + offset);
        sf.entries[i].frameStop =
            std::max(0L, sf.entries[i].frameStop + offset);
      }
    } else {
      if (frameRate <= 0.0) frameRate = determineFPS();
      if (frameRate <= 0.0) return false;

      for (int i = 0; i < sf.entries.size(); ++i) {
        sf.entries[i].frameStart = std::max(
            0L, ts2frame(frame2ts(sf.entries[i].frameStart, frameRate) + offset,
                         frameRate));
        sf.entries[i].frameStop = std::max(
            0L, ts2frame(frame2ts(sf.entries[i].frameStop, frameRate) + offset,
                         frameRate));
      }
    }
  }

  if (!skipConvertAds && !sf.entries.isEmpty()) {
    SubToken stQNapi;
    stQNapi.type = STT_WORD;
    stQNapi.payload = "QNapi";

    if (!sf.entries.last().tokens.contains(stQNapi)) {
      SubEntry adEntry;
      if (targetFormat->isTimeBased()) {
        adEntry.frameStart = sf.entries.back().frameStop + 2000L;
        adEntry.frameStop = adEntry.frameStart + 8000L;
      } else {
        adEntry.frameStart = sf.entries.back().frameStop + 50L;
        adEntry.frameStop = adEntry.frameStart + 200L;
      }
      QString ad = QObject::tr("Subtitles downloaded and processed by QNapi|");
      adEntry.tokens = targetFormat->decodeTokenStream(ad);
      SubToken urlToken;
      urlToken.type = STT_WORD;
      urlToken.payload = LibQNapi::webpageUrl();
      adEntry.tokens.push_back(urlToken);
      sf.entries.push_back(adEntry);
    }
  }

  QStringList targetLines = targetFormat->encode(sf);

  return writeFile(targetFileName, encoding, targetLines);
}

long SubtitleConverter::ts2frame(long ts, double frameRate) const {
  return static_cast<long>(floor(frameRate * ts / 1000.0));
}

long SubtitleConverter::frame2ts(long frame, double frameRate) const {
  return static_cast<long>(floor(1000.0 * frame / frameRate));
}

QStringList SubtitleConverter::readFile(const QString &filename,
                                        QString encoding,
                                        long atMostLines) const {
  QStringList buff;
  QFile inputFile(filename);
  if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextCodec* codec = QTextCodec::codecForName(
        qPrintable(encoding.isEmpty() ? QStringLiteral("UTF-8") : encoding));
    if (!codec) {
      qWarning() << "Unsupported subtitle encoding:" << encoding;
      return buff;
    }

    const QString text = codec->toUnicode(inputFile.readAll());
    if (text.isEmpty()) return buff;

    QStringList lines = text.split('\n', Qt::KeepEmptyParts);
    if (text.endsWith('\n')) lines.removeLast();
    for (QString line : lines) {
      if (atMostLines != 0 && buff.size() >= atMostLines) break;
      if (line.endsWith('\r')) line.chop(1);
      buff.push_back(line);
    }
    inputFile.close();
  }
  return buff;
}

bool SubtitleConverter::writeFile(const QString &filename, QString encoding,
                                  const QStringList &lines) const {
  QTextCodec* codec = QTextCodec::codecForName(
      qPrintable(encoding.isEmpty() ? QStringLiteral("UTF-8") : encoding));
  if (!codec) {
    qWarning() << "Unsupported subtitle encoding:" << encoding;
    return false;
  }

  QFile outputFile(filename);
  if (outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QString text = lines.join('\n');
    if (!lines.isEmpty()) text += '\n';
    const QByteArray encoded = codec->fromUnicode(text);
    const bool written = outputFile.write(encoded) == encoded.size();
    outputFile.close();
    return written;
  } else {
    return false;
  }
}
