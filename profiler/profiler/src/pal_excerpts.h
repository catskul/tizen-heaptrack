#ifndef PAL_EXCERPTS_H
#define PAL_EXCERPTS_H

#include "pal.h"

HRESULT __stdcall StringCopyWorkerW(WCHAR* pszDest, size_t cchDest, const WCHAR* pszSrc);

HRESULT __stdcall StringCchCopyW(WCHAR* pszDest, size_t cchDest, const WCHAR* pszSrc);

#endif// PAL_EXCERPTS_H