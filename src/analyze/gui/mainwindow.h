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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "gui_config.h"

#include <QMainWindow>

#ifndef NO_K_LIB
#include <KSharedConfig>
#endif

namespace Ui {
class MainWindow;
}

class TreeModel;
class ChartModel;
class Parser;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

public slots:
    void loadFile(const QString& path, const QString& diffBase = {});
    void openNewFile();
    void closeFile();
    void about();

signals:
    void clearData();

#ifdef NO_K_LIB
protected:
    virtual void closeEvent(QCloseEvent *event) override;
public slots:
    void selectOpenFile();
    void selectCompareToFile();
#endif
#ifdef QWT_FOUND
protected:
    virtual void moveEvent(QMoveEvent *event) override;
#endif
private:
    void showError(const QString& message);
    void setupStacks();

    QScopedPointer<Ui::MainWindow> m_ui;
    Parser* m_parser;
#ifndef NO_K_LIB
    KSharedConfig::Ptr m_config;
#endif
    bool m_diffMode = false;

    QAction* m_openAction = nullptr;
    QAction* m_openNewAction = nullptr;
    QAction* m_closeAction = nullptr;
    QAction* m_quitAction = nullptr;
};

#endif // MAINWINDOW_H
