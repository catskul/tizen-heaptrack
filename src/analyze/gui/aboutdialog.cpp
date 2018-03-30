#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "aboutdata.h"

#include <math.h>
#include <QFontMetrics>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setWindowTitle("About " + AboutData::DisplayName);

    ui->textBrowser->viewport()->setAutoFillBackground(false);

    ui->textBrowser->setHtml(QString(
        "<h2>%1 v.%2</h2>" \
        "<p>%3.</p>")
        .arg(AboutData::DisplayName).arg(AboutData::Version)
        .arg(AboutData::ShortDescription) +
        QString(
        "<p>%1.</p>")
        .arg(AboutData::CopyrightStatement) +
#ifdef SAMSUNG_TIZEN_BRANCH
        QString(
        "<p>Based on <a href=http://milianw.de/tag/heaptrack>Heaptrack memory profiler</a> " \
        "created by Milian Wolff on terms of " \
        "<a href=https://www.gnu.org/licenses/lgpl-3.0.en.html>LGPL</a>.</p>") +
#endif
        QString(
        "<p>Uses <a href=https://www.qt.io>Qt framework</a> v.%1 libraries on terms of " \
        "<a href=https://www.gnu.org/licenses/lgpl-3.0.en.html>LGPL</a>.</p>" \
        "<p>Uses <a href=https://cgit.kde.org/threadweaver.git>ThreadWeaver library</a> " \
        "on terms of <a href=https://www.gnu.org/licenses/lgpl-3.0.en.html>LGPL</a>.</p>" \
        "<p>The application is based in part on the work of the " \
        "<a href=http://qwt.sf.net>Qwt project</a> on terms of " \
        "<a href=http://qwt.sourceforge.net/qwtlicense.html>Qwt License</a>.</p>")
        .arg(QT_VERSION_STR)
#ifdef WINDOWS
        + QString(
        "<p>Application icon (free for commercial use): Jack Cai " \
        "(<a href=http://www.doublejdesign.co.uk>www.doublejdesign.co.uk</a>).</p>")
#endif
    );

    QFontMetrics fm(ui->textBrowser->font());
    QRect rect = fm.boundingRect("The application is based in part on the work of the Qwt project on terms of");
    int m = ui->verticalLayout->margin();
    int h = ui->buttonBox->height();
    int textWidth = (int)round(rect.width() * 1.05);
    int textHeight = (int)round(rect.height() * 1.03 * 17);
    resize(std::max(420, 2 * m + textWidth), std::max(252, 2 * m + h + textHeight));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
