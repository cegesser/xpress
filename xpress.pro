TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../boost_1_59_0

SOURCES += main.cpp \
    xpress.cpp

HEADERS += \
    type_switch.h \
    xpress.h


