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

#include "p7zipdecoder.h"

#include <QProcess>
#include <QRegularExpression>

P7ZipDecoder::P7ZipDecoder(const QString& p7zipPath, int operationTimeoutMsecs)
    : p7zipPath(p7zipPath), operationTimeoutMsecs(operationTimeoutMsecs) {}

QStringList P7ZipDecoder::listArchiveFiles(const QString& archivePath) const {
  QString stdoutBuff = readP7ZipOutput({"l", "-slt", archivePath});

#ifdef Q_OS_WIN
  QRegularExpression r("\r\nPath = ([^\r\n]*)\r\n");
#else
  QRegularExpression r("\nPath = ([^\n]*)\n");
#endif

  QStringList files;
  int offset = 0;

  auto match = r.match(stdoutBuff, offset);
  while (match.hasMatch()) {
    files << match.captured(1);
    offset = match.capturedEnd();
    match = r.match(stdoutBuff, offset);
  }

  return files;
}

bool P7ZipDecoder::unpackArchiveFile(const QString& archivePath,
                                     const QString& filename,
                                     const QString& targetPath) const {
  return runP7Zip({"e", "-y", "-o" + targetPath, archivePath, filename});
}

bool P7ZipDecoder::unpackArchiveFiles(const QString& archivePath,
                                      const QString& targetPath) const {
  return runP7Zip({"e", "-y", "-o" + targetPath, archivePath});
}

bool P7ZipDecoder::unpackSecureArchiveFiles(const QString& archivePath,
                                            const QString& password,
                                            const QString& targetPath) const {
  return runP7Zip({"e", "-y", "-p" + password, "-o" + targetPath, archivePath});
}

bool P7ZipDecoder::runP7Zip(const QStringList& args) const {
  QProcess p7zip;
  p7zip.start(p7zipPath, args);

  bool result = p7zip.waitForFinished(operationTimeoutMsecs);

  p7zip.close();

  return result;
}

QString P7ZipDecoder::readP7ZipOutput(const QStringList& args) const {
  QProcess p7zip;
  p7zip.start(p7zipPath, args);

  p7zip.waitForFinished(operationTimeoutMsecs);

  QString stdoutBuff = QByteArray(p7zip.readAll());

  p7zip.close();

  return stdoutBuff;
}
