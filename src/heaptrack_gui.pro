QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = heaptrack_gui
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/analyze/gui

# build heaptrack for Samsung Tizen OS
CONFIG += SAMSUNG_TIZEN_BRANCH

SAMSUNG_TIZEN_BRANCH {
    DEFINES += SAMSUNG_TIZEN_BRANCH
    TARGET = TizenMemoryProfiler
}

win32 {
    # Third-party libraries used:
    # QWT: http://qwt.sourceforge.net (SVN: https://svn.code.sf.net/p/qwt/code/trunk)
    # ThreadWeaver: https://cgit.kde.org/threadweaver.git (git://anongit.kde.org/threadweaver.git)

    # comment the next line to not use QWT library (charts will not be displayed in this case)
    CONFIG += QWT_CHART

    # comment the next line to not use ThreadWeaver library (used to parse data files faster)
    CONFIG += THREAD_WEAVER

    CONFIG += NO_K_LIB NO_K_CHART

    DEFINES += NO_K_LIB NO_K_CHART WINDOWS
    INCLUDEPATH += $$(BOOST_LIB)
    LIBS += -L$$(BOOST_LIB)/stage/lib

    RC_ICONS += analyze/gui/icons/if_diagram_v2-14_37134.ico

    CONFIG(debug, debug|release) {
        TARGET = $${TARGET}Dbg
        DESTDIR = ../bin/debug
    }
    else {
        DESTDIR = ../bin/release
    }
}

THREAD_WEAVER {
    DEFINES += THREAD_WEAVER
    win32 {
        # ThreadWeaver shall be built beforehand (load ThreadWeaver.pro, edit it if necessary, and build)

        # change the variable if ThreadWeaver headers are located in another directory
        THREAD_WEAVE_HEADER_PATH = ../../kf5/threadweaver/src/

        INCLUDEPATH += $$THREAD_WEAVE_HEADER_PATH ThreadWeaver
        CONFIG(debug, debug|release) {
            win32-msvc:LIBS += $${DESTDIR}/threadweaverd.lib
        }
        else {
            win32-msvc:LIBS += $${DESTDIR}/threadweaver.lib
        }
    }
    unix {
        QT += ThreadWeaver
    }
}

unix {
    CONFIG *= USE_CHART
    DEFINES *= USE_CHART

    # uncomment the next line to use QWT instead of KChart
#   CONFIG += QWT_CHART

    QWT_CHART {
        CONFIG *= NO_K_LIB NO_K_CHART QWT_CHART
        DEFINES *= NO_K_LIB NO_K_CHART
        INCLUDEPATH += /usr/include/qwt
        LIBS += -lqwt-qt5 # correct the library name if needed (e.g. to 'qwt')
    }

    LIBS += -lboost_program_options -lboost_iostreams -lpthread
}

QWT_CHART {
    # QMAKEFEATURES and QWT_ROOT environment variables must be set (e.g. to d:\Qwt\Qwt-6.2).
    # This is the directory where qwt.prf and qwt*.pri files reside.
    # Windows: file qwt.dll must exist in $${DESTDIR}\release and qwtd.dll in $${DESTDIR}\debug
    # to be able to run the application.
    CONFIG *= USE_CHART QWT
    DEFINES *= USE_CHART QWT
}

#Test only!
#CONFIG *= NO_K_LIB NO_K_CHART
#DEFINES *= NO_K_LIB NO_K_CHART

SOURCES += \
    analyze/accumulatedtracedata.cpp \
    analyze/gui/aboutdata.cpp \
    analyze/gui/aboutdialog.cpp \
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
    analyze/gui/aboutdata.h \
    analyze/gui/aboutdialog.h \
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

    QT *= ThreadWeaver
    DEFINES *= THREAD_WEAVER
}

!NO_K_CHART {
    QT += KChart

    FORMS += \
        analyze/gui/mainwindow.ui
}

NO_K_LIB {
    HEADERS += \
        analyze/gui/noklib.h

    FORMS += \
        analyze/gui/mainwindow_noklib.ui \
        analyze/gui/aboutdialog.ui

    RESOURCES += \
        analyze/gui/gui.qrc
}
