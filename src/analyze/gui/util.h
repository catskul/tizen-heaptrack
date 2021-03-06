/*
 * Copyright 2017 Milian Wolff <mail@milianw.de>
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

#ifndef UTIL_H
#define UTIL_H

#include "gui_config.h"

#include <qglobal.h>
#include <QString>

#ifdef QWT_FOUND
#include <QWidget>
#endif

namespace Util {

QString formatTime(qint64 ms);

QString formatByteSize(double size, int precision = 1);

QString wrapLabel(QString label, int maxLineLength, int lastLineExtra = 0,
                  const QString &delimiter = QString("<br>"));

#ifdef QWT_FOUND
bool isUnresolvedFunction(const QString &functionName);

bool exportChart(QWidget *parent, QWidget &chartWidget, const QString &chartName);
#endif
}

#endif // UTIL_H
