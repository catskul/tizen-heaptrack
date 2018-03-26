QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = heaptrack_gui_qt
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/analyze/gui

win32 {
    CONFIG += NO_K_LIB NO_K_CHART

#   comment the next line to not use QWT (i.e. don't show charts)
    CONFIG += QWT_CHART

    DEFINES += NO_K_LIB NO_K_CHART
    INCLUDEPATH += $$(BOOST_LIB)
    LIBS += -L$$(BOOST_LIB)/stage/lib
}

unix {
    CONFIG *= USE_CHART
    DEFINES *= USE_CHART

#   uncomment the next line to use QWT instead of KChart
#   CONFIG += QWT_CHART

    QWT_CHART {
        CONFIG *= NO_K_LIB NO_K_CHART QWT_CHART
        DEFINES *= NO_K_LIB NO_K_CHART
        INCLUDEPATH += /usr/include/qwt
        LIBS += -lqwt-qt5 # correct the library name if needed (e.g. to 'qwt')
    }

    LIBS += -lboost_program_options -lboost_iostreams -lpthread
}

#Test only!
#CONFIG *= NO_K_LIB NO_K_CHART
#DEFINES *= NO_K_LIB NO_K_CHART

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

QWT_CHART {
    # QMAKEFEATURES and QWT_ROOT environment variables must be set (e.g. to d:\Qwt\Qwt-6.2).
    # This is the directory where qwt.prf and qwt*.pri files reside.
    CONFIG *= USE_CHART QWT
    DEFINES *= USE_CHART QWT

    SOURCES += \
        analyze/gui/chartmodel2qwtseriesdata.cpp \
        analyze/gui/chartwidgetqwtplot.cpp \
        analyze/gui/contextmenuqwt.cpp \
        analyze/gui/histogramwidgetqwtplot.cpp

    HEADERS += \
        analyze/gui/chartmodel2qwtseriesdata.h \
        analyze/gui/chartwidgetqwtplot.h \
        analyze/gui/contextmenuqwt.h \
        analyze/gui/histogramwidgetqwtplot.h
}

USE_CHART {
    SOURCES += \
        analyze/gui/chartmodel.cpp \
        analyze/gui/chartproxy.cpp \
        analyze/gui/chartwidget.cpp \
        analyze/gui/histogrammodel.cpp \
        analyze/gui/histogramwidget.cpp

    HEADERS += \
        analyze/gui/chartmodel.h \
        analyze/gui/chartproxy.h \
        analyze/gui/chartwidget.h \
        analyze/gui/histogrammodel.h \
        analyze/gui/histogramwidget.h
}

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

!NO_K_CHART {
    QT += KChart

    FORMS += \
        analyze/gui/mainwindow.ui
}

NO_K_LIB {
    HEADERS += \
        analyze/gui/noklib.h \
        analyze/gui/aboutdialog.h

    SOURCES += \
        analyze/gui/aboutdialog.cpp

    FORMS += \
        analyze/gui/mainwindow_noklib.ui \
        analyze/gui/aboutdialog.ui

    RESOURCES += \
        analyze/gui/gui.qrc
}
