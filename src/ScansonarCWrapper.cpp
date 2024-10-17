// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#include <cstdio>
#include <cstdarg>
#include <string>

#include "Scansonar.h"
#include "ScansonarCWrapper.h"
#include "serial/serial.h"

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#endif

#if defined (__linux__)
pSnrCtx ScansonarOpen(const char* portpath, uint32_t baudrate, const char* filename, void(* const line_cb)(char*, int))
#else
pSnrCtx ScansonarOpen(const char* portpath, uint32_t baudrate, const wchar_t* filename, void(*const line_cb)(char*, int))
#endif
{
    pSnrCtx ctx = nullptr;

    try
    {
        std::shared_ptr<serial::Serial> serialPort(new serial::Serial(portpath, baudrate, serial::Timeout::simpleTimeout(SERIALPORT_TIMEOUT_MS)));
        if (nullptr == line_cb)
        {
            ctx = reinterpret_cast<pSnrCtx>(new Scansonar(serialPort, filename));
        }
        else
        {
            ctx = reinterpret_cast<pSnrCtx>(new Scansonar(serialPort, filename, line_cb));
        }
    }
    catch(...)
    {
        // In case of any exception this function returns nullptr
    }

    return ctx;
}

void ScansonarClose(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    delete ss;
    snrctx = nullptr;
}

size_t ScansonarReadData(pSnrCtx snrctx, uint8_t *buffer, size_t size)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    return ss->GetSerialPort()->read(buffer, size);
}

long ScansonarValueToLong(pcEchosounderValue value)
{
    return std::stol(value->value_text);
}

float ScansonarValueToFloat(pcEchosounderValue value)
{
    return std::stof(value->value_text);
}

const char * ScansonarValueToText(pcEchosounderValue value)
{
    return value->value_text;
}

bool IsValidScansonarValue(pcEchosounderValue value)
{
    return !((0 == value->value_len) || (0 == strlen(value->value_text)));
}

void LongToScansonarValue(long num, pEchosounderValue value)
{
    value->value_len = snprintf(value->value_text, sizeof(value->value_text), "%ld", num);
}

void FloatToScansonarValue(float num, pEchosounderValue value)
{
    value->value_len = snprintf(value->value_text, sizeof(value->value_text), "%f", num);
}

int ScansonarGetValue(pSnrCtx snrctx, ScansonarCommandIds_t command, pEchosounderValue value)
{
    int result = -1;

    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    std::string ssvalue = ss->GetValue(command);

    (void)snprintf(value->value_text, sizeof(value->value_text), "%s", ssvalue.c_str());
    value->value_len = (int)ssvalue.length();

    return (ssvalue.length() > 0) ? 0 : -1;
}

int ScansonarSetValue(pSnrCtx snrctx, ScansonarCommandIds_t command, pcEchosounderValue value)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    bool result = ss->SetValue(command, value->value_text);

    return (false != result) ? 0 : -1;
}

bool ScansonarDetect(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    return ss->Detect();
}

void ScansonarStart(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    ss->Start();
}

void ScansonarStop(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    ss->Stop();
}

bool ScansonarIsRunning(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    return ss->IsRunning();
}

bool ScansonarIsDetected(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    return ss->IsDetected();
}

uint16_t* GetRawSonarData(pSnrCtx snrctx)
{
    auto ss = reinterpret_cast<Scansonar*>(snrctx);
    return ss->GetRawSonarData();
}
