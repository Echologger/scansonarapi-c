# scansonarapi-c Echologger Scanning Sonar API for C/C++/C\#

## Prerequisites

- `Git` version control system

Git can be downloaded from [Git Webpage](https://git-scm.com/downloads)

- `CMake` build system

Git can be downloaded from [CMake download webpage](https://cmake.org/download/)

- optional `Ninja` utility

Can be downloaded from [Ninja webpage](https://ninja-build.org/)

## Build tools

- `ARM GCC` GCC GNU toolchain > 7.0.0 must be used for Linux build
- `Visual Studio` > 2013 or `MSYS64` for Windows build
- optional `Ninja` utility

Build command for Linux:

    git clone --recursive https://github.com/Echologger/scansonarapi-c.git
    cd echosounderapi-c
    mkdir build
    cd build
    cmake ..
    make all
    make install

    // or build using Ninja
    
    git clone --recursive https://github.com/Echologger/scansonarapi-c.git
    cd echosounderapi-c
    mkdir build
    cd build
    cmake -GNinja ..
    ninja    

Build command for Windows:

    git clone --recursive https://github.com/Echologger/scansonarapi-c.git
    cd echosounderapi-c
    mkdir build
    cd build
    cmake ..
    // in case of Visual Studio toolchain cmake produce a solution file, which can be used to make binaries later on
    // in case of MSYS cmake produce a 'Makefile' file, which can be used by `make` utility

Binary files can be found at the /exe or build folder

Using example (Windows):

    #include <windows.h>
    #include <stdio.h>
    #include <conio.h>
    #include "ScansonarCWrapper.h"
    #include "SonarStructures.h"

    static const uint16_t uncompand8to12b[256] =
    {
           0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15, //   0 ~ 15
          16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,   31, //  16 ~ 31
          32,   33,   34,   35,   36,   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47, //  32 ~ 47
          48,   49,   50,   51,   52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   62,   63, //  48 ~ 63
          65,   67,   69,   71,   73,   75,   77,   79,   81,   83,   85,   87,   89,   91,   93,   95, //  64 ~ 79
          97,   99,  101,  103,  105,  107,  109,  111,  113,  115,  117,  119,  121,  123,  125,  127, //  80 ~ 95
         131,  135,  139,  143,  147,  151,  155,  159,  163,  167,  171,  175,  179,  183,  187,  191, //  96 ~ 111
         195,  199,  203,  207,  211,  215,  219,  223,  227,  231,  235,  239,  243,  247,  251,  255, // 112 ~ 127
         263,  271,  279,  287,  295,  303,  311,  319,  327,  335,  343,  351,  359,  367,  375,  383, // 128 ~ 143
         391,  399,  407,  415,  423,  431,  439,  447,  455,  463,  471,  479,  487,  495,  503,  511, // 144 ~ 159
         527,  543,  559,  575,  591,  607,  623,  639,  655,  671,  687,  703,  719,  735,  751,  767, // 160 ~ 175
         783,  799,  815,  831,  847,  863,  879,  895,  911,  927,  943,  959,  975,  991, 1007, 1023, // 176 ~ 191
        1055, 1087, 1119, 1151, 1183, 1215, 1247, 1279, 1311, 1343, 1375, 1407, 1439, 1471, 1503, 1535, // 192 ~ 207
        1567, 1599, 1631, 1663, 1695, 1727, 1759, 1791, 1823, 1855, 1887, 1919, 1951, 1983, 2015, 2047, // 208 ~ 223
        2111, 2175, 2239, 2303, 2367, 2431, 2495, 2559, 2623, 2687, 2751, 2815, 2879, 2943, 3007, 3071, // 224 ~ 239
        3135, 3199, 3263, 3327, 3391, 3455, 3519, 3583, 3647, 3711, 3775, 3839, 3903, 3967, 4031, 4095  // 240 ~ 255
    };

    // Callback for each received line
    // This callback should be as short as possible!
    static void linecallback(char* linebuffer, int size)
    {
        PDATAHEADER pdh = (PDATAHEADER>)(&linebuffer[0]);
        PDATAFOOTER pdf = (PDATAFOOTER)(&linebuffer[pdh->samples - sizeof(DATAFOOTER)]);
        PCOMMANDID  recvid = (PCOMMANDID)(&pdh->commandid);

        // Do some preprocess here (amplify, calculate distance, etc...)
        for (int i = 0; i < pdh->samples - pdh->dataoffset - sizeof(DATAFOOTER); i++)
        {
            uint16_t sample = linebuffer[i + pdh->dataoffset];
            sample = uncompand8to12b[sample];
            // Do sample manipulation
        }
    }

    int main(void)
    {
        pSnrCtx sctx = ScansonarOpen("\\\\.\\COM27", 115200U, L"scandata.bin", linecallback); // Windows
        //pSnrCtx sctx = ScansonarOpen("/dev/ttyUSB0", 115200U, "scandata.bin", linecallback); // Linux

        // At this point Scanning Sonar start to work with default parameters if sctx is not NULL pointer
        // Status of sonar (Running/Error) can be obtainer by calling ScansonarIsDetected(sctx) function
        // ScansonarIsDetected(sctx) return false than sonar should be restarted by calling Close/Open functions

        if (NULL != sctx)
        {
            EchosounderValue txlength_value;
            EchosounderValue numofsamples_value;

            LongToScansonarValue(40, &txlength_value); // Convert pulse length integer to sonar value
            ScansonarSetValue(sctx, IdTxLength, &txlength_value); // Set new pulse length

            LongToScansonarValue(2048, &numofsamples_value); // Convert number of samples integer to sonar value
            ScansonarSetValue(sctx, IdSamples, &numofsamples_value);

            ScansonarStart(sctx); // Apply new settings to the Scanning sonar

            // Data from the sonar are temporary saved at the buffer sizeof 20400 samples X 3200 lines in the memory
            // Each line represent a received samples with sampling rate of 100kHz
            // As this "Image" contains 3200 lines, the angle resolution is 0.1125 deg.
            // This data can be obtained by calling GetRawSonarData(sctx)
            // How make picture from this data is desctibed in the document "RS900 communication protocolfor application developer.doc"

            for(;;)
            {
                Sleep(20);
                // Do other stuff
                if(0 != _kbhit()) { break; }
            }

            ScansonarClose(sctx);
        }

        return 0;
    }

Using with Python 3+

    import time
    import ctypes
    import itertools

    libscan = ctypes.cdll.LoadLibrary(".\\scansonar_api.dll")

    libscan.ScansonarOpen.argtypes = [ctypes.c_char_p, ctypes.c_uint32, ctypes.c_wchar_p, ctypes.c_void_p]
    libscan.ScansonarOpen.restype = ctypes.c_void_p

    libscan.GetRawSonarData.argtypes = [ctypes.c_void_p]
    libscan.GetRawSonarData.restype = ctypes.POINTER(ctypes.c_uint16)

    scanhandle = libscan.ScansonarOpen(b"\\\\.\\COM6", 921600, "", 0)

    if None != scanhandle:
        rawdata = libscan.GetRawSonarData(scanhandle)

        while True:
            #do your job here
