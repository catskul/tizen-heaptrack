#ifndef NOKLIB_H
#define NOKLIB_H

#include <QtCore/QString>

inline QString i18n(const char *text)
{
    return text;
}

template <typename A1>
inline QString i18n(const char *text, const A1 &a1)
{
    return QString(text).arg(a1);
}

template <typename A1, typename A2>
inline QString i18n(const char *text, const A1 &a1, const A2 &a2)
{
    return QString(text).arg(a1).arg(a2);
}

template <typename A1, typename A2, typename A3>
inline QString i18n(const char *text, const A1 &a1, const A2 &a2, const A3 &a3)
{
    return QString(text).arg(a1).arg(a2).arg(a3);
}

template <typename A1, typename A2, typename A3, typename A4>
inline QString i18n(const char *text, const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4)
{
    return QString(text).arg(a1).arg(a2).arg(a3).arg(a4);
}

template <typename A1, typename A2, typename A3, typename A4, typename A5>
inline QString i18n(const char *text, const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5)
{
    return QString(text).arg(a1).arg(a2).arg(a3).arg(a4).arg(a5);
}

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
inline QString i18n(const char *text, const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4, const A5 &a5, const A6 &a6)
{
    return QString(text).arg(a1).arg(a2).arg(a3).arg(a4).arg(a5).arg(a6);
}

template <typename A1>
inline QString i18nc(const char */*comment*/, const char *text, const A1 &a1)
{
    return QString(text).arg(a1);
}

template <typename A1, typename A2>
inline QString i18nc(const char */*comment*/, const char *text, const A1 &a1, const A2 &a2)
{
    return QString(text).arg(a1).arg(a2);
}

template <typename A1, typename A2, typename A3>
inline QString i18nc(const char */*comment*/, const char *text, const A1 &a1, const A2 &a2, const A3 &a3)
{
    return QString(text).arg(a1).arg(a2).arg(a3);
}

template <typename A1, typename A2, typename A3, typename A4>
inline QString i18nc(const char */*comment*/, const char *text, const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4)
{
    return QString(text).arg(a1).arg(a2).arg(a3).arg(a4);
}

template <typename A1, typename A2, typename A3, typename A4, typename A5>
inline QString i18nc(const char */*comment*/, const char *text, const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4,
                     const A5 &a5)
{
    return QString(text).arg(a1).arg(a2).arg(a3).arg(a4).arg(a5);
}

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
inline QString i18nc(const char */*comment*/, const char *text, const A1 &a1, const A2 &a2, const A3 &a3, const A4 &a4,
                     const A5 &a5, const A6 &a6)
{
    return QString(text).arg(a1).arg(a2).arg(a3).arg(a4).arg(a5).arg(a6);
}

template <typename A1>
inline QString i18np(const char */*singular*/, const char *plural, const A1 &a1)
{
    return QString(plural).arg(a1);
}

#endif // NOKLIB_H
