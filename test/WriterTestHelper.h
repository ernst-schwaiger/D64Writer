#ifndef WRITER_TEST_HELPER_H
#define WRITER_TEST_HELPER_H

#include "Writer.h"
#include <array>

namespace d64
{
    using D64ImgBuf = std::array<uint8_t, Writer::BYTES_PER_SECTOR * Writer::NUM_SECTORS>;

    // helps the test getting the bytes of the image back
    class D64ImageStreamBuf : public std::streambuf
    {
    public:
        D64ImageStreamBuf(D64ImgBuf &buf) : std::streambuf()
        {
            buf.fill(0x00);
            char *pBuf = reinterpret_cast<char *>(&buf[0]);
            this->setp(pBuf, &pBuf[buf.size()]);
        }
    };

    void assertProgOnImage(std::vector<uint8_t> const &prog, std::string const &d64ProgName, D64ImgBuf &imageBuf);
    void writeImageToBuf(D64ImgBuf &dest, Writer &src);
}

#endif