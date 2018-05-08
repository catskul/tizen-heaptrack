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

# uncomment the next line to disable using KDE libraries unconditionally (i.e. on Linux too)
#CONFIG *= NO_K_LIB NO_K_CHART

# Third-party libraries which may be used:

# QWT: http://qwt.sourceforge.net (SVN: https://svn.code.sf.net/p/qwt/code/trunk).
# Used on Windows but not on Linux by default.

# ThreadWeaver: https://cgit.kde.org/threadweaver.git (git://anongit.kde.org/threadweaver.git).
# Used on both Windows and Linux by default.

# comment the next line to not use ThreadWeaver library (used to parse data files faster)
!NO_K_LIB:CONFIG += THREAD_WEAVER

win32 {
    # comment the next line to not use QWT library (charts will not be displayed in this case)
    CONFIG += QWT_CHART

    CONFIG *= NO_K_LIB NO_K_CHART

    DEFINES *= WINDOWS
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

unix {
    # uncomment the next line to use QWT instead of KChart on Linux
#   CONFIG += QWT_CHART

    QWT_CHART {
        CONFIG *= NO_K_CHART USE_CHART QWT_CHART
        INCLUDEPATH += /usr/include/qwt
        LIBS += -lqwt-qt5 # correct the library name if needed (e.g. to 'qwt')
    }
    else {
        !NO_K_LIB {
            CONFIG *= USE_CHART
        }
     }

    LIBS += -lboost_program_options -lboost_iostreams -lpthread
}

QWT_CHART {
    # QMAKEFEATURES and QWT_ROOT environment variables must be set (e.g. to d:\Qwt\Qwt-6.2).
    # This is the directory where qwt.prf and qwt*.pri files reside.
    # Windows: file qwt.dll must exist in $${DESTDIR}\release and qwtd.dll in $${DESTDIR}\debug
    # to be able to run the application.
    CONFIG *= USE_CHART QWT
}

# add defines if have the following values in CONFIG
NO_K_LIB: DEFINES *= NO_K_LIB
NO_K_CHART: DEFINES *= NO_K_CHART
USE_CHART: DEFINES *= USE_CHART
QWT: DEFINES *= QWT

THREAD_WEAVER {
    DEFINES += THREAD_WEAVER
    win32 {
        # ThreadWeaver shall be built beforehand - use ThreadWeaver.pro file (edit it if necessary)

        # change the variable if ThreadWeaver headers are located in another directory
        THREAD_WEAVER_PATH = ../../kf5/threadweaver

        THREAD_WEAVER_HEADER_PATH = $${THREAD_WEAVER_PATH}/src/

        INCLUDEPATH += $$THREAD_WEAVER_HEADER_PATH ThreadWeaver
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
        analyze/gui/charthelpwindow.cpp \
        analyze/gui/chartmodel2qwtseriesdata.cpp \
        analyze/gui/chartwidgetqwtplot.cpp \
        analyze/gui/contextmenuqwt.cpp \
        analyze/gui/histogramwidgetqwtplot.cpp

    HEADERS += \
        analyze/gui/charthelpwindow.h \
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

    RESOURCES += \
        analyze/gui/gui.qrc
}

FORMS += \
    analyze/gui/aboutdialog.ui

# copy extra files (Win32, release)
win32 {
    CONFIG(release, debug|release) {
        # copy Qt dlls
        EXTRA_BINFILES += \
            $$(QTDIR)/bin/Qt5Core.dll \
            $$(QTDIR)/bin/Qt5Gui.dll \
            $$(QTDIR)/bin/Qt5OpenGL.dll \
            $$(QTDIR)/bin/Qt5Svg.dll \
            $$(QTDIR)/bin/Qt5Widgets.dll
        QWT_CHART {
            # ... and qwt.dll
            EXTRA_BINFILES += $$(QWT_ROOT)/lib/qwt.dll
        }
        EXTRA_BINFILES_WIN = $${EXTRA_BINFILES}
        EXTRA_BINFILES_WIN ~= s,/,\\,g
        DESTDIR_WIN = $${DESTDIR}
        DESTDIR_WIN ~= s,/,\\,g
        for (FILE, EXTRA_BINFILES_WIN) {
            QMAKE_POST_LINK += $$quote(cmd /c copy /y $${FILE} $${DESTDIR_WIN}$$escape_expand(\n\t))
        }
        QT_PLUGIN_DIR_WIN = $$(QTDIR)/plugins
        QT_PLUGIN_DIR_WIN ~= s,/,\\,g
        # copy imageformats\qjpeg.dll
        DEST_PLUGIN_DIR = $${DESTDIR_WIN}\\imageformats
        QMAKE_POST_LINK += $$quote(cmd /c if not exist $${DEST_PLUGIN_DIR} mkdir $${DEST_PLUGIN_DIR}$$escape_expand(\n\t))
        QMAKE_POST_LINK += $$quote(cmd /c copy /y $${QT_PLUGIN_DIR_WIN}\\imageformats\\qjpeg.dll $${DEST_PLUGIN_DIR}$$escape_expand(\n\t))
        # copy platforms\qwindows.dll
        DEST_PLUGIN_DIR = $${DESTDIR_WIN}\\platforms
        QMAKE_POST_LINK += $$quote(cmd /c if not exist $${DEST_PLUGIN_DIR} mkdir $${DEST_PLUGIN_DIR}$$escape_expand(\n\t))
        QMAKE_POST_LINK += $$quote(cmd /c copy /y $${QT_PLUGIN_DIR_WIN}\\platforms\\qwindows.dll $${DEST_PLUGIN_DIR}$$escape_expand(\n\t))
        # copy styles\qwindowsvistastyle.dll
        DEST_PLUGIN_DIR = $${DESTDIR_WIN}\\styles
        QMAKE_POST_LINK += $$quote(cmd /c if not exist $${DEST_PLUGIN_DIR} mkdir $${DEST_PLUGIN_DIR}$$escape_expand(\n\t))
        QMAKE_POST_LINK += $$quote(cmd /c copy /y $${QT_PLUGIN_DIR_WIN}\\styles\\qwindowsvistastyle.dll $${DEST_PLUGIN_DIR}$$escape_expand(\n\t))
    }
}
