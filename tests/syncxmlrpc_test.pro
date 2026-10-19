QT += core network xml
CONFIG += console c++17
TEMPLATE = app
TARGET = syncxmlrpc-test

INCLUDEPATH += ../libqnapi/src

SOURCES += syncxmlrpc_test.cpp \
    ../libqnapi/src/utils/syncxmlrpc.cpp

HEADERS += ../libqnapi/src/utils/syncxmlrpc.h
