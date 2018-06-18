TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#INCLUDEPATH += $$PWD/src

LIBS += -lboost_program_options -lboost_iostreams -lpthread

SOURCES += \
    analyze/print/heaptrack_print.cpp \
    analyze/accumulatedtracedata.cpp

HEADERS += \
    analyze/accumulatedtracedata.h
    util/config.h
