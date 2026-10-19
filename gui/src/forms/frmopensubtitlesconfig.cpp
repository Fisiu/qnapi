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
#include <QDesktopServices>

#include "engines/opensubtitlesdownloadengine.h"
#include "frmopensubtitlesconfig.h"
#include "utils/windowutils.h"

frmOpenSubtitlesConfig::frmOpenSubtitlesConfig(const EngineConfig &config,
                                               QWidget *parent,
                                               Qt::WindowFlags f)
    : QDialog(parent, f), config(config) {
  ui.setupUi(this);

  ui.leNick->setText(config.nick());
  ui.lePass->setText(config.password());

  QIcon openSubtitlesIcon =
      QIcon(QPixmap(OpenSubtitlesDownloadEngine::pixmapData));
  setWindowIcon(openSubtitlesIcon);

  connect(ui.pbRegister, SIGNAL(clicked()), this, SLOT(pbRegisterClicked()));

  centerOnPrimaryScreen(*this);
}

EngineConfig frmOpenSubtitlesConfig::getConfig() const { return config; }

void frmOpenSubtitlesConfig::accept() {
  config = config.setNick(ui.leNick->text()).setPassword(ui.lePass->text());
  QDialog::accept();
}

void frmOpenSubtitlesConfig::pbRegisterClicked() {
  std::optional<QUrl> maybeRegistrationUrl =
      OpenSubtitlesDownloadEngine::metadata.registrationUrl();

  if (maybeRegistrationUrl) {
    QDesktopServices::openUrl(maybeRegistrationUrl.value());
  }
}
