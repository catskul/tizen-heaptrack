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

    ui->textEdit->viewport()->setAutoFillBackground(false);

    ui->textEdit->setHtml(QString(
        "<h2>A visualizer for heaptrack data files</h2>" \
        "<p>Copyright 2015, Milian Wolff " \
        "&lt;<a href=mailto:mail@milianw.de>mail@milianw.de</a>&gt;</p>" \
        "<p>GNU LESSER GENERAL PUBLIC LICENSE v.2.1</p>" \
        "<p>Original author, maintainer: Milian Wolff</p>" \
        "<p>Copyright 2018, %1</p>" \
        "<p>The application is based in part on the work of the Qwt project " \
        "(<a href=http://qwt.sf.net>qwt.sf.net</a>)</p>" \
        "<p>Application icon (free for commercial use): Jack Cai " \
        "(<a href=http://www.doublejdesign.co.uk>www.doublejdesign.co.uk</a>)</p>")
        .arg(AboutData::Organization)
    );

    QFontMetrics fm(ui->textEdit->font());
    QRect rect = fm.boundingRect("The application is based in part on the work of the Qwt project (qwt.sf.net)");
    int m = ui->verticalLayout->margin();
    int h = ui->buttonBox->height();
    int textWidth = (int)round(rect.width() * 1.03);
    int textHeight = (int)round(rect.height() * 1.03 * 14);
    resize(std::max(420, 2 * m + textWidth), std::max(252, 2 * m + h + textHeight));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
