#include <cstdarg>
#include <cstdio>
#include <memory>
#include <cassert>

#include "outstream.h"

int fprintf(outStream *stream, const char* format, ...) noexcept
{
    assert(stream && format);

    int tmpStrSize = 0;
    va_list argptr;

    std::unique_lock<std::mutex> lock(stream->m_fprintfBuf);
    for (;;) {
        va_start(argptr, format);
        tmpStrSize = std::vsnprintf(stream->fprintfBuf, stream->fprintfBufSize, format, argptr);
        va_end(argptr);
        if (tmpStrSize > -1 && static_cast<size_t>(tmpStrSize) < stream->fprintfBufSize) {
            break;
        } else if (tmpStrSize > -1) {
            size_t needBufSize = tmpStrSize + sizeof('\0');
            if (stream->fprintfBuf)
                delete [] stream->fprintfBuf;
            stream->fprintfBuf = new (std::nothrow) char[needBufSize];
            if (!stream->fprintfBuf) {
                stream->fprintfBufSize = 0;
                errno = ENOMEM;
                return -1;
            }
            stream->fprintfBufSize = needBufSize;
        } else {
            errno = EIO;
            return -1;
        }
    }

    int ret = stream->Puts(stream->fprintfBuf);
    lock.unlock();
    if (ret >= 0) {
        // make proper return code, since it different from fputs()
        ret = tmpStrSize;
    }

    return ret;
}
