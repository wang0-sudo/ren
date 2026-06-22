QT += widgets

CONFIG += c++17
CONFIG += console
TEMPLATE = app
TARGET = 招聘图形界面

SOURCES += \
    主程序.cpp \
    登录窗口.cpp \
    工作台窗口.cpp \
    招聘服务.cpp

HEADERS += \
    登录窗口.h \
    工作台窗口.h \
    招聘服务.h
