#ifndef OUTSTREAMFILE_H
#define OUTSTREAMFILE_H

#include "outstream.h"

class outStreamFILE final : public outStream
{
public:
    outStreamFILE() = delete;
    explicit outStreamFILE(const char *FileName);
    explicit outStreamFILE(FILE *FileStream);
    ~outStreamFILE();

    outStreamFILE(const outStreamFILE &) = delete;
    outStreamFILE &operator = (const outStreamFILE &) = delete;

    outStreamFILE(outStreamFILE &&other) :
        Stream_(other.Stream_),
        Owner_(other.Owner_)
    {
        other.Stream_ = nullptr;
        other.Owner_ = false;
    }
    outStreamFILE &operator = (outStreamFILE &&other) {
        Stream_ = other.Stream_;
        Owner_ = other.Owner_;
        other.Stream_ = nullptr;
        other.Owner_ = false;
        return *this;
    }

    int Putc(int Char) noexcept override;
    int Puts(const char *String) noexcept override;
    bool Flush() noexcept override;

private:
    FILE *Stream_;
    bool Owner_;
};

#endif // OUTSTREAMFILE_H
