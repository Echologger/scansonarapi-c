// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#include <cstdint>
#include <memory>

#include "B64Encode.h"

namespace
{
    constexpr uint8_t base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    void encodeblock(const uint8_t in[3], uint8_t out[4])
    {
        out[0] = base64chars[((in[0] & 0xFC) >> 2)];
        out[1] = base64chars[((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4)];
        out[2] = base64chars[((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6)];
        out[3] = base64chars[(in[2] & 0x3F)];
    }

    void encodelastblock(const uint8_t in[3], uint8_t out[4], uint16_t len)
    {
        uint8_t in1 = (len > 1U) ? in[1] : 0U;
        uint8_t in2 = (len > 2U) ? in[2] : 0U;

        out[0] = base64chars[((in[0] & 0xFC) >> 2)];
        out[1] = base64chars[((in[0] & 0x03) << 4) | ((in1 & 0xF0) >> 4)];
        out[2] = (len > 1U) ? base64chars[((in1 & 0x0F) << 2) | ((in2 & 0xC0) >> 6)] : '=';
        out[3] = (len > 2U) ? base64chars[ (in2 & 0x3F)] : '=';
    }

    void b64encode(const uint8_t *const inbuf, uint16_t len, uint8_t *outbuf)
    {
        int nblocks = len / 3;
        int lastlen = len - nblocks * 3;
        int i;

        for (i = 0; i < nblocks; i++)
        {
            encodeblock(&inbuf[i * 3], &outbuf[i * 4]);
        }

        if (lastlen > 0)
        {
            encodelastblock(&inbuf[i * 3], &outbuf[i * 4], lastlen);
            outbuf[(i + 1) * 4] = 0; // Make sure out string is NULL-terminated
        }
        else
        {
            outbuf[i * 4] = 0; // Make sure out string is NULL-terminated
        }
    }
}

B64Encode::B64Encode(const void *data, std::size_t datasize, const std::string &extra)
{
    auto encodeddata = std::make_unique<uint8_t[]>(datasize * 2);
    b64encode(reinterpret_cast<const uint8_t *>(data), static_cast<uint16_t>(datasize), encodeddata.get());
    b64data = std::string(reinterpret_cast<const char *>(encodeddata.get()));
    b64data = b64data + extra;
}

B64Encode::~B64Encode()
{
}

const std::string &B64Encode::GetEncodedData() const
{
    return b64data;
}
