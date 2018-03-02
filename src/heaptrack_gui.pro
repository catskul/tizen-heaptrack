QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = heaptrack_gui_qt
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/analyze/gui

win32 {
    CONFIG += NO_K_LIB NO_K_CHART QWT_CHART
    DEFINES += NO_K_LIB NO_K_CHART
    INCLUDEPATH += $$(BOOST_LIB)
    LIBS += -L$$(BOOST_LIB)/stage/lib
}

unix {
#    QMAKE_CXXFLAGS += -std=c++0x
#    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter # disable 'unused parameter' warning
    DEFINES *= USE_CHART
    LIBS += -lboost_program_options -lboost_iostreams -lpthread
}

#Test only!
#CONFIG *= NO_K_LIB
#DEFINES *= NO_K_LIB

QWT_CHART {
    # QMAKEFEATURES and QWT_ROOT environment variables must be set (e.g. to d:\Qwt\Qwt-6.2).
    # This is the directory where qwt.prf and qwt*.pri files reside.
    CONFIG *= USE_CHART QWT
    DEFINES *= USE_CHART
}

SOURCES += \
    analyze/accumulatedtracedata.cpp \
    analyze/gui/gui.cpp \
    analyze/gui/callercalleemodel.cpp \
    analyze/gui/costdelegate.cpp \
    analyze/gui/flamegraph.cpp \
    analyze/gui/mainwindow.cpp \
    analyze/gui/objecttreemodel.cpp \
    analyze/gui/objecttreeproxy.cpp \
    analyze/gui/parser.cpp \
    analyze/gui/stacksmodel.cpp \
    analyze/gui/topproxy.cpp \
    analyze/gui/treemodel.cpp \
    analyze/gui/treeproxy.cpp \
    analyze/gui/util.cpp

HEADERS += \
    analyze/accumulatedtracedata.h \
    analyze/gui/callercalleemodel.h \
    analyze/gui/costdelegate.h \
    analyze/gui/flamegraph.h \
    analyze/gui/gui_config.h \
    analyze/gui/locationdata.h \
    analyze/gui/mainwindow.h \
    analyze/gui/objecttreemodel.h \
    analyze/gui/objecttreeproxy.h \
    analyze/gui/parser.h \
    analyze/gui/stacksmodel.h \
    analyze/gui/summarydata.h \
    analyze/gui/topproxy.h \
    analyze/gui/treemodel.h \
    analyze/gui/treeproxy.h \
    analyze/gui/util.h \
    util/config.h

!NO_K_LIB {
    QT += KCoreAddons # for KFormat
    QT += KI18n

    QT += KIOCore
    QT += KIOFileWidgets

    QT += KConfigWidgets
    QT += KIOWidgets

    QT += KItemModels

    QT += ThreadWeaver
}

USE_CHART {
    SOURCES += \
        analyze/gui/chartmodel.cpp \
#        analyze/gui/chartproxy.cpp \
        analyze/gui/chartwidget.cpp \
        analyze/gui/histogrammodel.cpp \
#        analyze/gui/histogramwidget.cpp

    HEADERS += \
        analyze/gui/chartmodel.h \
#        analyze/gui/chartproxy.h \
        analyze/gui/chartwidget.h \
        analyze/gui/histogrammodel.h \
#        analyze/gui/histogramwidget.h
}

!NO_K_CHART {
    QT += KChart

    SOURCES += \
#        analyze/gui/chartmodel.cpp \
        analyze/gui/chartproxy.cpp \
#        analyze/gui/chartwidget.cpp \
#        analyze/gui/histogrammodel.cpp \
        analyze/gui/histogramwidget.cpp

    HEADERS += \
#        analyze/gui/chartmodel.h \
        analyze/gui/chartproxy.h \
#        analyze/gui/chartwidget.h \
#        analyze/gui/histogrammodel.h \
        analyze/gui/histogramwidget.h

    FORMS += \
        analyze/gui/mainwindow.ui
}

NO_K_LIB {
    HEADERS += \
        analyze/gui/noklib.h

    FORMS += \
        analyze/gui/mainwindow_noklib.ui
}
