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

#include "util.h"

#include <QRegularExpression>
#include <QString>

#ifndef NO_K_LIB
#include <KFormat>
#endif

#ifdef QWT_FOUND
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#endif

namespace Util {

QString formatTime(qint64 ms)
{
    if (ms > 60000) {
        // minutes
        return QString::number(double(ms) / 60000, 'g', 3) + QLatin1String("min");
    } else {
        // seconds
        return QString::number(double(ms) / 1000, 'g', 3) + QLatin1Char('s');
    }
}

QString formatByteSize(double size, int precision)
{
#ifndef NO_K_LIB
    static KFormat format;
    return format.formatByteSize(size, precision, KFormat::JEDECBinaryDialect);
#else
    int32_t divider = 0;
    QString suffix;
    if (size < 1024)
    {
        suffix = "B";
    }
    else
    {
        const int32_t LimitMB = 1024 * 1024;
        if (size < LimitMB)
        {
            divider = 1024;
            suffix = "KB";
        }
        else
        {
            const int32_t LimitGB = LimitMB * 1024;
            if (size < LimitGB)
            {
                divider = LimitMB;
                suffix = "MB";
            }
            else
            {
                divider = LimitGB;
                suffix = "GB";
            }
        }
    }
    QString result;
    if (divider > 1)
    {
        size /= divider;
    }
    else if ((int32_t)size == size)
    {
        precision = 0;
    }
    result = QString::number(size, 'f', precision);
    result += ' ';
    result += suffix;
    return result;
#endif
}

QString wrapLabel(QString label, int maxLineLength, int lastLineExtra,
    const QString& delimiter)
{
    int labelLength = label.size();
    if (labelLength + lastLineExtra <= maxLineLength)
    {
        return label.toHtmlEscaped();
    }
    static QRegularExpression delimBefore("[(<]");
    static QRegularExpression delimAfter("[- .,)>\\/]");
    QString result;
    do
    {
        int i = -1;
        int wrapAfter = 0;
        int i1 = label.indexOf(delimBefore, maxLineLength);
        int i2 = label.indexOf(delimAfter, maxLineLength - 1);
        if (i1 >= 0)
        {
            if (i2 >= 0)
            {
                if (i2 < i1)
                {
                    i = i2;
                    wrapAfter = 1;
                }
                else
                {
                    i = i1;
                }
            }
            else
            {
                i = i1;
            }
        }
        else
        {
            i = i2;
            wrapAfter = 1;
        }
        if (i < 0)
        {
            break;
        }
        i += wrapAfter;
        result += label.left(i).toHtmlEscaped();
        label.remove(0, i);
        if (label.isEmpty()) // special: avoid <br> at the end
        {
            return result;
        }
        result += delimiter;
        labelLength -= i;
    }
    while (labelLength + lastLineExtra > maxLineLength);
    result += label.toHtmlEscaped();
    return result;
}

#ifdef QWT_FOUND
bool isUnresolvedFunction(const QString &functionName)
{
    return functionName.startsWith("<unresolved function>");
}

bool exportChart(QWidget *parent, QWidget &chartWidget, const QString &chartName)
{
    QString selectedFilter;
    QString saveFilename = QFileDialog::getSaveFileName(parent, "Save Chart As",
        chartName, "PNG (*.png);; BMP (*.bmp);; JPEG (*.jpg *.jpeg)", &selectedFilter);
    if (!saveFilename.isEmpty())
    {
        QFileInfo fi(saveFilename);
        if (fi.suffix().isEmpty()) // can be on some platforms
        {
            int i = selectedFilter.indexOf("*.");
            if (i >= 0)
            {
                static QRegularExpression delimiters("[ )]");
                i += 2;
                int j = selectedFilter.indexOf(delimiters, i);
                if (j > i)
                {
                    --i;
                    QString suffix = selectedFilter.mid(i, j - i);
                    saveFilename += suffix;
                }
            }
        }
        if (chartWidget.grab().save(saveFilename))
        {
            return true;
        }
        QMessageBox::warning(parent, "Error",
            QString("Cannot save the chart to \"%1\".").arg(saveFilename), QMessageBox::Ok);
    }
    return false;
}
#endif

} // namespace Util
