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
#include "libmediainfomovieinfoprovider.h"
#include <QRegularExpression>
#include <string>

#ifndef UNICODE
#define UNICODE
#endif

#ifdef UNICODE
#define _UNICODE
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
#include "MediaInfoDLL/MediaInfoDLL.h"  //Dynamicly-loaded library (.dll or .so)
#define MediaInfoNameSpace MediaInfoDLL;
#else
#include "MediaInfo/MediaInfo.h"  //Staticly-loaded library (.lib or .a or .so)
#define MediaInfoNameSpace MediaInfoLib;
#endif
using namespace MediaInfoNameSpace;

const std::optional<MovieInfo> LibmediainfoMovieInfoProvider::getMovieInfo(
    const QString& moviePath) const {
  MediaInfo mi;
  mi.Option(__T("Internet"), __T("No"));

  mi.Open(moviePath.toStdWString());

#define GET_VIDEO_INFO(__streamIdx, __key) \
  QString::fromStdWString(                 \
      std::wstring(mi.Get(Stream_Video, __streamIdx, __T(__key), Info_Text)))

  QString widthS = GET_VIDEO_INFO(0, "Width");
  QString heightS = GET_VIDEO_INFO(0, "Height");
  QString frameRateS = GET_VIDEO_INFO(0, "FrameRate");
  QString durationS = GET_VIDEO_INFO(0, "Duration");

  mi.Close();

  QRegularExpression rWidth("(\\d+)");
  const auto widthMatch = rWidth.match(widthS);
  if (!widthMatch.hasMatch()) return std::nullopt;

  QRegularExpression rHeight("(\\d+)");
  const auto heightMatch = rHeight.match(heightS);
  if (!heightMatch.hasMatch()) return std::nullopt;

  QRegularExpression rFrameRate("(\\d+).(\\d+)");
  const auto frameRateMatch = rFrameRate.match(frameRateS);
  if (!frameRateMatch.hasMatch()) return std::nullopt;
  long frFloor = frameRateMatch.captured(1).toLong();
  long frFrac = frameRateMatch.captured(2).toLong();

  QRegularExpression rDuration("(\\d+)");
  const auto durationMatch = rDuration.match(durationS);
  if (!durationMatch.hasMatch()) return std::nullopt;

  MovieInfo info(widthMatch.captured(1).toInt(),
                 heightMatch.captured(1).toInt(),
                 (double)frFloor + (double)frFrac / 1000.0,
                 durationMatch.captured(1).toDouble() / 1000.0);

  return std::make_optional<MovieInfo>(info);
}
