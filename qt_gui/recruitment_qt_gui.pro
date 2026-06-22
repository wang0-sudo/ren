QT += widgets

CONFIG += c++17
CONFIG += console
TEMPLATE = app
TARGET = recruitment_qt_gui

SOURCES += \
    main.cpp \
    login_window.cpp \
    dashboard_window.cpp \
    recruitment_service.cpp

HEADERS += \
    login_window.h \
    dashboard_window.h \
    recruitment_service.h
