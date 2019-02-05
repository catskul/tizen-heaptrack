#ifndef OUTSTREAM_H
#define OUTSTREAM_H

// Note, since heaptrack IO was designed in C-style, make sure,
// that fprintf(), fputc() and fputs() don't throw exceptions
// and return same value behavior as stdoi's fprintf(), fputc()
// and fputs() respectively.

// Make sure, that errno provide proper error code in Putc() and Puts(),
// since heaptrack use it in writeError().

class outStream {
public:
    outStream() = default;
    virtual ~outStream() = default;
    outStream(const outStream &) = delete;
    outStream &operator = (const outStream &) = delete;

    virtual int Putc(int Char) noexcept = 0;
    virtual int Puts(const char *String) noexcept = 0;
    virtual bool Flush() noexcept = 0;
};

template <class Implementation, class Initialization>
outStream *OpenStream(Initialization InitValue) noexcept
{
    outStream *Result = nullptr;
    try {
        Result = new Implementation(InitValue);
    } catch (...) {
        Result = nullptr;
    }
    return Result;
}

int fprintf(outStream *stream, const char* format, ...) noexcept;

inline int fputc(int ch, outStream *stream) noexcept
{
    return stream->Putc(ch);
}

inline int fputs(const char *str, outStream *stream) noexcept
{
    return stream->Puts(str);
}

#endif // OUTSTREAM_H
