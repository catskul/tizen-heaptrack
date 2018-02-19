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

#include <QString>

#ifndef NO_K_LIB
#include <KFormat>
#endif

QString Util::formatTime(qint64 ms)
{
    if (ms > 60000) {
        // minutes
        return QString::number(double(ms) / 60000, 'g', 3) + QLatin1String("min");
    } else {
        // seconds
        return QString::number(double(ms) / 1000, 'g', 3) + QLatin1Char('s');
    }
}

QString Util::formatByteSize(double size, int precision)
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
