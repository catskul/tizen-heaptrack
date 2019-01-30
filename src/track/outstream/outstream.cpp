#include <cstdarg>
#include <cstdio>
#include <memory>
#include <cassert>

#include "outstream.h"

int fprintf(outStream *stream, const char* format, ...) noexcept
{
    assert(stream && format);

    // heaptrack creates only one outStream instance and provide its own locking, we are safe here
    static std::unique_ptr<char[]> Buf;
    static size_t BufSize = 0;
    int tmpStrSize = 0;
    va_list argptr;

    for (;;) {
        va_start(argptr, format);
        tmpStrSize = std::vsnprintf(Buf.get(), BufSize, format, argptr);
        va_end(argptr);
        if (tmpStrSize > -1
            && static_cast<size_t>(tmpStrSize) < BufSize) {
            break;
        } else if (tmpStrSize > -1) {
            size_t needBufSize = tmpStrSize + sizeof('\0');
            Buf.reset(new (std::nothrow) char[needBufSize]);
            if (!Buf.get()) {
                BufSize = 0;
                errno = ENOMEM;
                return -1;
            }
            BufSize = needBufSize;
        } else {
            errno = EIO;
            return -1;
        }
    }

    int ret = stream->Puts(Buf.get());
    if (ret > 0) {
        // make proper return code, since it different from fputs()
        ret = tmpStrSize;
    }

    return ret;
}
