QT += core gui widgets

CONFIG += c++17
TEMPLATE = app
TARGET = Tinyopen

# 头文件搜索路径：项目根目录 + core 子目录
# （源码里用 "core/xxx.h" 或 "xxx.h" 都能找到）
INCLUDEPATH += $$PWD $$PWD/core

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    groupwidget.cpp \
    fileitemwidget.cpp \
    settingsdialog.cpp \
    filedialog.cpp \
    logdialog.cpp \
    core/configstore.cpp \
    core/launcher.cpp \
    core/logger.cpp \
    core/iconpainter.cpp

HEADERS += \
    mainwindow.h \
    groupwidget.h \
    fileitemwidget.h \
    settingsdialog.h \
    filedialog.h \
    logdialog.h \
    core/groupdata.h \
    core/configstore.h \
    core/launcher.h \
    core/logger.h \
    core/iconpainter.h

win32 {
    RC_FILE = app.rc
}

# 输出到 build 目录，方便直接运行
DESTDIR = $$OUT_PWD/build
