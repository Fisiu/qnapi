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

#ifndef CLIARGPARSERSEXECUTOR_H
#define CLIARGPARSERSEXECUTOR_H

#include <optional>
#include <variant>
#include "config/qnapiconfig.h"
#include "parser/cliargparser.h"
#include "tr.h"

#include <QList>
#include <QMetaType>
#include <QSharedPointer>
#include <QString>

namespace CliArgParsersExecutor {

Q_DECLARE_NAMESPACE_TR(CliArgParsersExecutor)

using ExecutionResult =
    std::variant<QString, std::optional<CliArgParser::ParsedCommand>>;

ExecutionResult executeParsers(
    const QList<QSharedPointer<CliArgParser>> &parsers, const QStringList &args,
    const QNapiConfig &config);

QStringList formatHelpLines(const QList<QSharedPointer<CliArgParser>> &parsers,
                            int switchesBlockSize, int descBlockSize,
                            int preSwitchSpaceSize);

}  // namespace CliArgParsersExecutor

#endif  // CLIARGPARSERSEXECUTOR_H
