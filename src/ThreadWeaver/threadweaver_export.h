#ifndef THREADWEAVER_GLOBAL_H
#define THREADWEAVER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(THREADWEAVER_LIBRARY)
#  define THREADWEAVER_EXPORT Q_DECL_EXPORT
#else
#  define THREADWEAVER_EXPORT Q_DECL_IMPORT
#endif

#endif // THREADWEAVER_GLOBAL_H