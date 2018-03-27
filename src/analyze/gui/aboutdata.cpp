#include "aboutdata.h"

#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KLocalizedString>
#endif

#ifdef SAMSUNG_TIZEN_BRANCH
const QString& AboutData::Organization = "Samsung";

const QString& AboutData::CopyrightStatement =
        i18n("Copyright 2015, Milian Wolff <mail@milianw.de>. " \
             "Copyright 2018, Samsung.");

const QString& AboutData::BugAddress = "TODO"; // TODO!!
#else
const QString& AboutData::Organization = "Milian Wolff";

const QString& AboutData::CopyrightStatement = i18n("Copyright 2015, Milian Wolff <mail@milianw.de>");

const QString& AboutData::BugAddress = QStringLiteral("mail@milianw.de");
#endif

const QString& AboutData::ComponentName = QStringLiteral("heaptrack_gui");

const QString& AboutData::DisplayName = i18n("Heaptrack GUI");

const QString& AboutData::Version = QStringLiteral("0.1");

const QString& AboutData::ShortDescription = i18n("A visualizer for heaptrack data files.");
