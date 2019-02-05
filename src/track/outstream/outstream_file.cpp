#include <cassert>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <stdio_ext.h>

#include "outstream_file.h"

outStreamFILE::outStreamFILE(const char *FileName) :
    Stream_(nullptr),
    Owner_(false)
{
    assert(FileName);

    Stream_ = fopen(FileName, "w");
    if (!Stream_) {
        fprintf(stderr, "WARNING! Failed to open file %s: %s\n", FileName, strerror(errno));
        throw std::runtime_error("Unable to open stream");
    }

    Owner_ = true;
    // from heaptrack code: we do our own locking, this speeds up the writing significantly
    __fsetlocking(Stream_, FSETLOCKING_BYCALLER);
}

outStreamFILE::outStreamFILE(FILE *FileStream) :
    Stream_(nullptr),
    Owner_(false)
{
    assert(FileStream);
    Stream_ = FileStream;
}

outStreamFILE::~outStreamFILE()
{
    if (Owner_ && Stream_) {
        fclose(Stream_);
    }
}

int outStreamFILE::Putc(int Char) noexcept
{
    if (!Stream_) {
        errno = EIO;
        return EOF;
    }
    return fputc(Char, Stream_);
}

int outStreamFILE::Puts(const char *String) noexcept
{
    if (!Stream_) {
        errno = EIO;
        return EOF;
    } else if (!String) {
        errno = EINVAL;
        return EOF;
    }
    return fputs(String, Stream_);
}
