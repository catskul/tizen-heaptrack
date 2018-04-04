#include "charthelpwindow.h"
#include "chartwidget.h"

#include <QTextEdit>
#include <QToolTip>

ChartHelpWindow::ChartHelpWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle("Chart Help");
    setWindowFlags(Qt::Tool);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_DeleteOnClose);
    setPalette(QToolTip::palette());
    setWindowOpacity(0.9);
    setAutoFillBackground(true);
    auto textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setContextMenuPolicy(Qt::NoContextMenu);
    textEdit->viewport()->setAutoFillBackground(false);
    setCentralWidget(textEdit);
    const auto text =
        "<p>Use <u>Context Menu</u> (right click inside the chart to open) to control different chart options.</p>" \
        "<p>Use <u>left mouse button</u> to <b>zoom in</b> to selection: press the button, drag " \
        "to make a rectangular selection, release.</p>" \
        "<p>Use <u>left mouse button</u> with modifier keys to:</p>" \
        "<ul>" \
        "<li><b>zoom out</b> (one step back) - <b>&lt;Shift&gt;</b>+click;" \
        "<li><b>reset zoom</b> - <b>&lt;Ctrl&gt;</b>+click;" \
        "<li><b>move (pan)</b> the chart  - <b>&lt;Alt&gt;</b>+drag." \
        "</ul>";
    textEdit->setHtml(text);
    QFontMetrics fm(textEdit->font());
    QRect rect = fm.boundingRect("Use left mouse button to zoom in to selection: press the ");
    int textWidth = std::max(292, (int)round(rect.width() * 1.03));
    int textHeight = std::max(164, (int)round(rect.height() * 1.03 * 12));
    setGeometry(0, 0, textWidth, textHeight);
    setMinimumSize(120, 80);
    setMaximumSize(std::max(400, textWidth * 2), std::max(280, textHeight * 2));
}

ChartHelpWindow::~ChartHelpWindow()
{
    ChartWidget::HelpWindow = nullptr;
}

void ChartHelpWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    ChartOptions::GlobalOptions = ChartOptions::setOption(ChartOptions::GlobalOptions, ChartOptions::ShowHelp, false);
}
