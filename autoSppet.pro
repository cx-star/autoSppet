TEMPLATE = app
TARGET = autoSppet
QT += webenginewidgets core
CONFIG += c++11

PRECOMPILED_HEADER += utf8.h
HEADERS += \
    browser.h \
    browserwindow.h \
    tabwidget.h \
    urllineedit.h \
    webview.h \
    webpage.h \
    webpopupwindow.h \
    utf8.h \
    plugform.h

SOURCES += \
    browser.cpp \
    browserwindow.cpp \
    main.cpp \
    tabwidget.cpp \
    urllineedit.cpp \
    webview.cpp \
    webpage.cpp \
    webpopupwindow.cpp \
    plugform.cpp

FORMS += \
    certificateerrordialog.ui \
    passworddialog.ui \
    plugform.ui

RESOURCES += data/simplebrowser.qrc

