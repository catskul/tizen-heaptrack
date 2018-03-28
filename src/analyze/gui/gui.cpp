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
#include <QMessageBox>

#ifdef NO_K_LIB
#include "noklib.h"
#else
#include <KAboutData>
#include <KLocalizedString>
#endif

#include "aboutdata.h"
#include "../accumulatedtracedata.h"
#include "../allocationdata.h"
#include "mainwindow.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

#ifndef NO_K_LIB
    KLocalizedString::setApplicationDomain(AboutData::ShortName.toStdString().c_str());

    const auto LicenseType = KAboutLicense::LGPL;

    typedef AboutData A;

    KAboutData aboutData(A::ComponentName, A::DisplayName, A::Version, A::ShortDescription,
                         LicenseType, A::CopyrightStatement, QString(),
                         QString(), A::BugAddress);

    aboutData.addAuthor(i18n("Milian Wolff"), i18n("Original author, maintainer"), QStringLiteral("mail@milianw.de"),
                        QStringLiteral("http://milianw.de"));

    aboutData.setOrganizationDomain("kde.org");

    KAboutData::setApplicationData(aboutData);
#endif
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("office-chart-area")));

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
#ifndef NO_K_LIB
    aboutData.setupCommandLine(&parser);
#endif

    QCommandLineOption diffOption{{QStringLiteral("d"), QStringLiteral("diff")},
                                  i18n("Base profile data to compare other files to."),
                                  QStringLiteral("<file>")};
    parser.addOption(diffOption);
    parser.addPositionalArgument(QStringLiteral("files"), i18n("Files to load"), i18n("[FILE...]"));

    QCommandLineOption showMallocOption(QStringLiteral("malloc"), QStringLiteral("Show malloc-allocated memory consumption"));
    QCommandLineOption showManagedOption(QStringLiteral("managed"), QStringLiteral("Show managed memory consumption"));
    QCommandLineOption showPrivateDirtyOption(QStringLiteral("private_dirty"), QStringLiteral("Show Private_Dirty part of memory consumption"));
    QCommandLineOption showPrivateCleanOption(QStringLiteral("private_clean"), QStringLiteral("Show Private_Clean part of memory consumption"));
    QCommandLineOption showSharedOption(QStringLiteral("shared"), QStringLiteral("Show Shared_Clean + Shared_Dirty part of memory consumption"));

    parser.addOption(showMallocOption);
    parser.addOption(showManagedOption);
    parser.addOption(showPrivateDirtyOption);
    parser.addOption(showPrivateCleanOption);
    parser.addOption(showSharedOption);

    QCommandLineOption hideUnmanagedStackPartsOption(QStringLiteral("hide-unmanaged-stacks"), QStringLiteral("Hide unmanaged parts of call stacks"));
    parser.addOption(hideUnmanagedStackPartsOption);

    QCommandLineOption showCoreCLRPartOption(QStringLiteral("show-coreclr"), QStringLiteral("Show CoreCLR/non-CoreCLR memory distribution"));
    parser.addOption(showCoreCLRPartOption);

    parser.process(app);
#ifndef NO_K_LIB
    aboutData.processCommandLine(&parser);
#endif

    bool isShowMalloc = parser.isSet(showMallocOption);
    bool isShowManaged = parser.isSet(showManagedOption);
    bool isShowPrivateDirty = parser.isSet(showPrivateDirtyOption);
    bool isShowPrivateClean = parser.isSet(showPrivateCleanOption);
    bool isShowShared = parser.isSet(showSharedOption);
    bool isHideUnmanagedStackParts = parser.isSet(hideUnmanagedStackPartsOption);
    bool isShowCoreCLRPartOption = parser.isSet(showCoreCLRPartOption);

    if ((isShowMalloc ? 1 : 0)
        + (isShowManaged ? 1 : 0)
        + (isShowPrivateDirty ? 1 : 0)
        + (isShowPrivateClean ? 1 : 0)
        + (isShowShared ? 1 : 0) != 1) {

        const auto msg = "One of --malloc, --managed, --private_dirty, --private_clean or --shared options is necessary. " \
                         "Please, use exactly only one of the options for each start of GUI.";

        QMessageBox::critical(nullptr, AboutData::DisplayName + " Error", msg, QMessageBox::Ok);

        qFatal(msg);

        return 1;
    } else if (isShowMalloc)
    {
        AllocationData::display = AllocationData::DisplayId::malloc;
    } else if (isShowManaged)
    {
        AllocationData::display = AllocationData::DisplayId::managed;
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

    if (isShowCoreCLRPartOption) {
        AccumulatedTraceData::isShowCoreCLRPartOption = true;
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
