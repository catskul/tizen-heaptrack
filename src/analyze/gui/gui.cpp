/*
 * Copyright 2015-2017 Milian Wolff <mail@milianw.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KLocalizedString>

#include "../accumulatedtracedata.h"
#include "../allocationdata.h"
#include "mainwindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("heaptrack");

    KAboutData aboutData(QStringLiteral("heaptrack_gui"), i18n("Heaptrack GUI"), QStringLiteral("0.1"),
                         i18n("A visualizer for heaptrack data files."), KAboutLicense::LGPL,
                         i18n("Copyright 2015, Milian Wolff <mail@milianw.de>"), QString(),
                         QStringLiteral("mail@milianw.de"));

    aboutData.addAuthor(i18n("Milian Wolff"), i18n("Original author, maintainer"), QStringLiteral("mail@milianw.de"),
                        QStringLiteral("http://milianw.de"));

    aboutData.setOrganizationDomain("kde.org");

    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("office-chart-area")));

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    QCommandLineOption diffOption{{QStringLiteral("d"), QStringLiteral("diff")},
                                  i18n("Base profile data to compare other files to."),
                                  QStringLiteral("<file>")};
    parser.addOption(diffOption);
    parser.addPositionalArgument(QStringLiteral("files"), i18n("Files to load"), i18n("[FILE...]"));

    QCommandLineOption showMallocOption(QStringLiteral("malloc"), QStringLiteral("Show malloc-allocated memory consumption"));
    QCommandLineOption showPrivateDirtyOption(QStringLiteral("private_dirty"), QStringLiteral("Show Private_Dirty part of memory consumption"));
    QCommandLineOption showPrivateCleanOption(QStringLiteral("private_clean"), QStringLiteral("Show Private_Clean part of memory consumption"));
    QCommandLineOption showSharedOption(QStringLiteral("shared"), QStringLiteral("Show Shared_Clean + Shared_Dirty part of memory consumption"));

    parser.addOption(showMallocOption);
    parser.addOption(showPrivateDirtyOption);
    parser.addOption(showPrivateCleanOption);
    parser.addOption(showSharedOption);

    QCommandLineOption hideUnmanagedStackPartsOption(QStringLiteral("hide-unmanaged-stacks"), QStringLiteral("Hide unmanaged parts of call stacks"));
    parser.addOption(hideUnmanagedStackPartsOption);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    bool isShowMalloc = parser.isSet(showMallocOption);
    bool isShowPrivateDirty = parser.isSet(showPrivateDirtyOption);
    bool isShowPrivateClean = parser.isSet(showPrivateCleanOption);
    bool isShowShared = parser.isSet(showSharedOption);
    bool isHideUnmanagedStackParts = parser.isSet(hideUnmanagedStackPartsOption);

    if ((isShowMalloc ? 1 : 0)
        + (isShowPrivateDirty ? 1 : 0)
        + (isShowPrivateClean ? 1 : 0)
        + (isShowShared ? 1 : 0) != 1) {

        qFatal("One of --malloc, --private_dirty, --private_clean or --shared options is necessary. Please, use exactly only one of the options for each start of GUI.");

        return 1;
    } else if (isShowMalloc)
    {
        AllocationData::display = AllocationData::DisplayId::malloc;
    } else if (isShowPrivateDirty) {
        AllocationData::display = AllocationData::DisplayId::privateDirty;
    } else if (isShowPrivateClean) {
        AllocationData::display = AllocationData::DisplayId::privateClean;
    } else {
        assert (isShowShared);

        AllocationData::display = AllocationData::DisplayId::shared;
    }

    if (isHideUnmanagedStackParts) {
        AccumulatedTraceData::isHideUnmanagedStackParts = true;
    }

    auto createWindow = []() -> MainWindow* {
        auto window = new MainWindow;
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
        return window;
    };

    foreach (const QString& file, parser.positionalArguments()) {
        createWindow()->loadFile(file, parser.value(diffOption));
    }

    if (parser.positionalArguments().isEmpty()) {
        createWindow();
    }

    return app.exec();
}
