#include "aboutdata.h"

#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KLocalizedString>
#endif

#ifdef SAMSUNG_TIZEN_BRANCH
const QString& AboutData::Organization = "Samsung Electronics Co.";

const QString& AboutData::CopyrightStatement =
        i18n("Copyright 2015, Milian Wolff. " \
             "Copyright 2018, Samsung Electronics Co.");

const QString& AboutData::ComponentName = QStringLiteral("TizenMemoryProfiler");

const QString& AboutData::ShortName = QStringLiteral("Tizen Memory Profiler");

const QString& AboutData::DisplayName = i18n("Tizen Memory Profiler");

// Version corresponds to CMake @HEAPTRACK_VERSION_MAJOR@.@HEAPTRACK_VERSION_MINOR@.@HEAPTRACK_VERSION_PATCH@-@HEAPTRACK_VERSION_SUFFIX@
const QString& AboutData::Version = QStringLiteral("1.1.0-0.1");

const QString& AboutData::ShortDescription = i18n("A visualizer for Tizen Memory Profiler data files");

const QString& AboutData::BugAddress = "TODO"; // TODO!!
#else
const QString& AboutData::Organization = "Milian Wolff";

const QString& AboutData::CopyrightStatement = i18n("Copyright 2015, Milian Wolff <mail@milianw.de>");

const QString& AboutData::ComponentName = QStringLiteral("heaptrack_gui");

const QString& AboutData::ShortName = QStringLiteral("heaptrack");

const QString& AboutData::DisplayName = i18n("Heaptrack GUI");

// Version corresponds to CMake @HEAPTRACK_VERSION_MAJOR@.@HEAPTRACK_VERSION_MINOR@.@HEAPTRACK_VERSION_PATCH@
const QString& AboutData::Version = QStringLiteral("1.1.0");

const QString& AboutData::ShortDescription = i18n("A visualizer for heaptrack data files");

const QString& AboutData::BugAddress = QStringLiteral("mail@milianw.de");
#endif
