#ifndef WINDOWUTILS_H
#define WINDOWUTILS_H

#include <QGuiApplication>
#include <QScreen>
#include <QWidget>

inline void centerOnPrimaryScreen(QWidget& widget) {
  if (QScreen* screen = QGuiApplication::primaryScreen()) {
    QRect frame = widget.frameGeometry();
    frame.moveCenter(screen->availableGeometry().center());
    widget.move(frame.topLeft());
  }
}

#endif  // WINDOWUTILS_H
