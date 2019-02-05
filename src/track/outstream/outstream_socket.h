#ifndef OUTSTREAMSOCKET_H
#define OUTSTREAMSOCKET_H

#include <memory>
#include <cassert>
#include "outstream.h"

class outStreamSOCKET final : public outStream
{
public:
    outStreamSOCKET() = delete;
    explicit outStreamSOCKET(uint16_t Port);
    ~outStreamSOCKET();

    outStreamSOCKET(const outStreamSOCKET &) = delete;
    outStreamSOCKET &operator = (const outStreamSOCKET &) = delete;

    outStreamSOCKET(outStreamSOCKET &&other) :
        Socket_(other.Socket_),
        BufferUsedSize_(other.BufferUsedSize_),
        Buffer_(std::move(other.Buffer_))
    {
        other.Socket_ = -1;
        other.BufferUsedSize_ = 0;
    }
    outStreamSOCKET &operator = (outStreamSOCKET &&other) {
        Socket_ = other.Socket_;
        other.Socket_ = -1;
        BufferUsedSize_ = other.BufferUsedSize_;
        other.BufferUsedSize_ = 0;
        Buffer_ = std::move(other.Buffer_);
        return *this;
    }

    int Putc(int Char) noexcept override;
    int Puts(const char *String) noexcept override;
    bool Flush() noexcept override;

    static constexpr int MinAllowedSocketPort = 1;
    static constexpr int MaxAllowedSocketPort = 65535;
    static constexpr uint16_t DefaultSocketPort = 5050;

private:
    bool SocketErrorDetected() noexcept;
    bool BufferedWriteToSocket(const void *Data, size_t Count) noexcept;
    bool SendToSocket(const void *Data, size_t Count) noexcept;
    void CopyToBuffer(const void *Data, size_t Count) noexcept;
    bool FlushBuffer() noexcept;

    size_t AvailableSpace() const noexcept
    {
        assert(BufferCapacity_ >= BufferUsedSize_);
        return BufferCapacity_ - BufferUsedSize_;
    }

    char *BufferPos() const noexcept
    {
        assert(BufferCapacity_ >= BufferUsedSize_);
        return Buffer_.get() + BufferUsedSize_;
    }

    int Socket_;
    // mainly, heaptrack send small blocks (about 1-50 bytes each)
    static constexpr size_t BufferCapacity_ = 4096;
    size_t BufferUsedSize_;
    std::unique_ptr<char[]> Buffer_;
};

#endif // OUTSTREAMSOCKET_H
