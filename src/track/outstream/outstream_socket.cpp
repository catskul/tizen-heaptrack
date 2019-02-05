#include <arpa/inet.h>
#include <cassert>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdexcept>
#include <string.h>
#include <unistd.h>

#include "outstream_socket.h"

outStreamSOCKET::outStreamSOCKET(uint16_t Port) :
    Socket_(-1),
    BufferUsedSize_(0),
    Buffer_(new char[BufferCapacity_])
{
    int tmpSocketID = -1;
    auto HandleError = [&tmpSocketID] (const char *ErrorText, int Error) {
        if (tmpSocketID != -1) {
            close(tmpSocketID);
        }
        fprintf(stderr, "WARNING! %s: %s\n", ErrorText, strerror(Error));
        throw std::runtime_error(ErrorText);
    };

    tmpSocketID = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tmpSocketID == -1) {
        HandleError("socket()", errno);
    }

    int on = 1;
    if (setsockopt(tmpSocketID, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1
        || setsockopt(tmpSocketID, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1) {
        HandleError("setsockopt()", errno);
    }

    struct sockaddr_in tmpServerAddr;
    memset((char*)&tmpServerAddr, 0, sizeof(tmpServerAddr));
    tmpServerAddr.sin_family = AF_INET;
    tmpServerAddr.sin_port = htons(Port);
    tmpServerAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(tmpSocketID, (struct sockaddr *) &tmpServerAddr, sizeof(tmpServerAddr)) == -1) {
        HandleError("bind()", errno);
    }

    if (listen(tmpSocketID, 1) == -1) {
        HandleError("listen()", errno);
    }

    struct sockaddr_storage tmpServerStorage;
    socklen_t addr_size = sizeof tmpServerStorage;
    Socket_ = accept(tmpSocketID, (struct sockaddr*)&tmpServerStorage, &addr_size);
    if (Socket_ == -1) {
        HandleError("accept()", errno);
    }

    close(tmpSocketID);
}

outStreamSOCKET::~outStreamSOCKET()
{
    if (Socket_ == -1) {
        return;
    }

    FlushBuffer();
    close(Socket_);
}

bool outStreamSOCKET::SocketErrorDetected() noexcept
{
    int SocketErrno;
    unsigned int ErrnoSize = sizeof(SocketErrno);
    if (getsockopt(Socket_, SOL_SOCKET, SO_ERROR, &SocketErrno, &ErrnoSize) == -1) {
        return true;
    }

    if (SocketErrno) {
        close(Socket_);
        Socket_ = -1;
        fprintf(stderr, "WARNING! Unable to use socket: %s", strerror(SocketErrno));
        errno = SocketErrno;
        return true;
    }

    return false;
}

bool outStreamSOCKET::BufferedWriteToSocket(const void *Data, size_t Count) noexcept
{
    if (Count > BufferCapacity_) {
        if (!FlushBuffer()) {
            return false;
        }
        return SendToSocket(Data, Count);
    }

    if (Count > AvailableSpace()) {
        if (!FlushBuffer()) {
            return false;
        }
    }

    CopyToBuffer(Data, Count);
    return true;
}

bool outStreamSOCKET::SendToSocket(const void *Data, size_t Count) noexcept
{
    if (SocketErrorDetected()) {
        return false;
    }
    if (Count == 0) {
        return true;
    }
    return send(Socket_, Data, Count, MSG_NOSIGNAL) != -1;
}

void outStreamSOCKET::CopyToBuffer(const void *Data, size_t Count) noexcept
{
    memcpy(BufferPos(), Data, Count);
    BufferUsedSize_ += Count;
}

bool outStreamSOCKET::FlushBuffer() noexcept
{
    if (Socket_ == -1) {
        errno = EIO;
        return false;
    }
    bool ret = SendToSocket(Buffer_.get(), BufferUsedSize_);
    BufferUsedSize_ = 0;
    return ret;
}

int outStreamSOCKET::Putc(int Char) noexcept
{
    if (Socket_ == -1) {
        errno = EIO;
        return EOF;
    }

    // same behavior as for fputc()
    unsigned char tmpChar = static_cast<unsigned char>(Char);
    if (BufferedWriteToSocket(&tmpChar, sizeof(unsigned char)))
        return Char;

    return EOF;
}

int outStreamSOCKET::Puts(const char *String) noexcept
{
    if (Socket_ == -1) {
        errno = EIO;
        return EOF;
    } else if (!String) {
        errno = EINVAL;
        return EOF;
    }

    // same behavior as for fputs()
    if (BufferedWriteToSocket(String, strlen(String)))
        return 1; // return a nonnegative number on success

    return EOF;
}

bool outStreamSOCKET::Flush() noexcept
{
    if (Socket_ == -1) {
        errno = EIO;
        return false;
    }
    return FlushBuffer();
}
