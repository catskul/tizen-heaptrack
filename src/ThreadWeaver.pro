QT       -= gui

TEMPLATE = lib

TARGET = threadweaver

# change the variable if ThreadWeaver sources are located in another directory
SRC_PATH = ../../kf5/threadweaver/src/

win32 {
    CONFIG(debug, debug|release) {
        TARGET = $${TARGET}d
        DESTDIR = ../bin/debug
    }
    else {
        DESTDIR = ../bin/release
    }
}

# Probably you don't need to build the library on Linux - check whether it's available
# in your Linux distribution's repositories.
unix {
    target.path = /usr/lib
    INSTALLS += target
}

DEFINES += THREADWEAVER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ThreadWeaver

SOURCES += \
    $${SRC_PATH}collection.cpp \
    $${SRC_PATH}collection_p.cpp \
    $${SRC_PATH}debuggingaids.cpp \
    $${SRC_PATH}dependency.cpp \
    $${SRC_PATH}dependencypolicy.cpp \
    $${SRC_PATH}destructedstate.cpp \
    $${SRC_PATH}exception.cpp \
    $${SRC_PATH}executewrapper.cpp \
    $${SRC_PATH}executor.cpp \
    $${SRC_PATH}iddecorator.cpp \
    $${SRC_PATH}inconstructionstate.cpp \
    $${SRC_PATH}job.cpp \
    $${SRC_PATH}job_p.cpp \
    $${SRC_PATH}qobjectdecorator.cpp \
    $${SRC_PATH}queue.cpp \
    $${SRC_PATH}queueapi.cpp \
    $${SRC_PATH}queuesignals.cpp \
    $${SRC_PATH}queuesignals_p.cpp \
    $${SRC_PATH}queuestream.cpp \
    $${SRC_PATH}resourcerestrictionpolicy.cpp \
    $${SRC_PATH}sequence.cpp \
    $${SRC_PATH}sequence_p.cpp \
    $${SRC_PATH}shuttingdownstate.cpp \
    $${SRC_PATH}state.cpp \
    $${SRC_PATH}suspendedstate.cpp \
    $${SRC_PATH}suspendingstate.cpp \
    $${SRC_PATH}thread.cpp \
    $${SRC_PATH}threadweaver.cpp \
    $${SRC_PATH}weaver.cpp \
    $${SRC_PATH}weaver_p.cpp \
    $${SRC_PATH}weaverimplstate.cpp \
    $${SRC_PATH}workinghardstate.cpp

HEADERS += \
    ThreadWeaver/threadweaver_export.h \
    $${SRC_PATH}collection.h \
    $${SRC_PATH}collection_p.h \
    $${SRC_PATH}debuggingaids.h \
    $${SRC_PATH}dependency.h \
    $${SRC_PATH}dependencypolicy.h \
    $${SRC_PATH}destructedstate.h \
    $${SRC_PATH}exception.h \
    $${SRC_PATH}executewrapper_p.h \
    $${SRC_PATH}executor_p.h \
    $${SRC_PATH}iddecorator.h \
    $${SRC_PATH}inconstructionstate.h \
    $${SRC_PATH}job.h \
    $${SRC_PATH}job_p.h \
    $${SRC_PATH}jobinterface.h \
    $${SRC_PATH}jobpointer.h \
    $${SRC_PATH}lambda.h \
    $${SRC_PATH}managedjobpointer.h \
    $${SRC_PATH}qobjectdecorator.h \
    $${SRC_PATH}queue.h \
    $${SRC_PATH}queueapi.h \
    $${SRC_PATH}queueing.h \
    $${SRC_PATH}queueinterface.h \
    $${SRC_PATH}queuepolicy.h \
    $${SRC_PATH}queuesignals.h \
    $${SRC_PATH}queuesignals_p.h \
    $${SRC_PATH}queuestream.h \
    $${SRC_PATH}resourcerestrictionpolicy.h \
    $${SRC_PATH}sequence.h \
    $${SRC_PATH}sequence_p.h \
    $${SRC_PATH}shuttingdownstate.h \
    $${SRC_PATH}state.h \
    $${SRC_PATH}suspendedstate.h \
    $${SRC_PATH}suspendingstate.h \
    $${SRC_PATH}thread.h \
    $${SRC_PATH}threadweaver.h \
    $${SRC_PATH}weaver.h \
    $${SRC_PATH}weaver_p.h \
    $${SRC_PATH}weaverimplstate.h \
    $${SRC_PATH}weaverinterface.h \
    $${SRC_PATH}workinghardstate.h
