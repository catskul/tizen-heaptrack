#ifndef HELPWIDGET_H
#define HELPWIDGET_H

#include <QMainWindow>
#include <QWidget>

class ChartHelpWindow: public QMainWindow
{
    Q_OBJECT

public:
    explicit ChartHelpWindow(QWidget *parent = 0);
    ~ChartHelpWindow();

protected:
    virtual void closeEvent(QCloseEvent *event) override;
};

#endif // HELPWIDGET_H
