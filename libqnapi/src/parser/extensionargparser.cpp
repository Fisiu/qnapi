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
#include "extensionargparser.h"
#include <QRegularExpression>

ExtensionArgParser::ExtensionArgParser() {}

QVariant ExtensionArgParser::parse(const QStringList& args,
                                   const QNapiConfig& config) const {
  int idx = args.indexOf("-e");
  if (idx == -1) {
    idx = args.indexOf("--extension");
  }
  if (idx == -1) {
    return QVariant::fromValue(NothingParsed());
  }

  if (idx == args.size() - 1) {
    return QVariant::fromValue(
        ParseError{tr("You need to pass subtitles extension after %1 argument")
                       .arg(args[idx])});
  }

  QString ext = args[idx + 1];

  if (!QRegularExpression("^[A-Za-z0-9]+\\z").match(ext).hasMatch()) {
    return QVariant::fromValue(ParseError{
        tr("Invalid target subtitles extension passed: %1").arg(ext)});
  } else {
    return QVariant::fromValue(ParsedModifier{config.setPostProcessingConfig(
        config.postProcessingConfig().setSubExtension(ext))});
  }
}

std::optional<CliArgParser::HelpInfo> ExtensionArgParser::helpInfo() const {
  return std::make_optional(HelpInfo{"-e", "--extension", tr("ext"),
                       tr("Select target subtitles file extension")});
}
