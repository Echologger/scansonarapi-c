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

    #include <stdio.h>
    #include <conio.h>
    #include "ScansonarCWrapper.h"
    #include "SonarStructures.h"

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
        //pSnrCtx sctx = ScansonarOpen("/dev/ttyUSB0", 115200U, L"scandata.bin", linecallback); // Linux

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
