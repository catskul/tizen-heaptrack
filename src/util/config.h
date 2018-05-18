/*
 * Copyright 2014-2017 Milian Wolff <mail@milianw.de>
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

#ifndef HEAPTRACK_CONFIG_H
#define HEAPTRACK_CONFIG_H

#define HEAPTRACK_VERSION_STRING "1.1.0-0.1"
#define HEAPTRACK_VERSION_MAJOR 1
#define HEAPTRACK_VERSION_MINOR 1
#define HEAPTRACK_VERSION_PATCH 0
#define HEAPTRACK_VERSION ((HEAPTRACK_VERSION_MAJOR<<16)|(HEAPTRACK_VERSION_MINOR<<8)|(HEAPTRACK_VERSION_PATCH))

#define HEAPTRACK_FILE_FORMAT_VERSION 2

#define HEAPTRACK_DEBUG_BUILD 1

// cfree() does not exist in glibc 2.26+.
// See: https://bugs.kde.org/show_bug.cgi?id=383889
#define HAVE_CFREE 0

#endif // HEAPTRACK_CONFIG_H
