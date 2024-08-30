// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#include <chrono>
#include <algorithm>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>

#include "ThreadSonarSerial.h"
#include "SonarStructures.h"
#include "Crc32.h"
#include "B64Encode.h"
#include "AppLists.h"

namespace
{
    // *INDENT-OFF*

    const uint16_t uncompand8to12b[256] =
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

    // *INDENT-ON*
}

//wxThread::ExitCode ThreadSonarSerial::Entry()
static void SonarSerialThreadFunc(void* arg)
{
    bool isfailed = false;

    ThreadSonarSerial* tss = reinterpret_cast<ThreadSonarSerial*>(arg);

    while (false == tss->threadkilled)
    {
        if (false == isfailed)
        {
            try
            {
                switch (tss->state)
                {
                case ThreadSSState::TSSState_Init:
                {
                    tss->state = tss->ThreadInit();
                    break;
                }

                case ThreadSSState::TSSState_Connecting:
                {
                    tss->state = tss->ThreadConnecting();
                    break;
                }

                case ThreadSSState::TSSState_Connected:
                    tss->state = tss->ThreadConnected();
                    break;

                case ThreadSSState::TSSState_Working:
                    //std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    tss->state = tss->ThreadWorking();
                    break;

                case ThreadSSState::TSSState_SetSettings:
                    tss->state = tss->ThreadSetSettings();
                    break;

                case ThreadSSState::TSSState_Disconnected:
                default:
                {
                    tss->sonarfailed_ = true;
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    break;
                }
                }
            }
            catch (serial::IOException& exstr)
            {
                tss->sonarfailed_ = true;
                tss->state = ThreadSSState::TSSState_Disconnected;
                tss->serialport.reset();
                isfailed = true;
                std::string exeptiontxt = std::string(exstr.what());
                std::cout << "ThreadSonarSerial::Entry : IOException" << "\n";
                std::cout << exstr.what() << "\n";
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

ThreadSonarSerial::ThreadSonarSerial(std::shared_ptr<serial::Serial> SerialPort, std::wstring filename, std::function<void(char*, int)> cbfunc) :
    cb_dataready(cbfunc),
    serialport(SerialPort),
    threadkilled(false),
    params_updated(true),
    sonarfailed_(false)
{
    //dcsp = { 1, 0, 1000, 0, 0, 2, 50, 50000, 368, 100000, 0.0F, 0.0F, 0, 80, 0, 0, 0.0F, 0.0F };
    //dssp = { 0, 0, 0, 1, 35138, 0 };
    dcsp = { 0, };
    dssp = { 0, };

    keep_alive_counter = std::chrono::steady_clock::now();

    sonarData = std::make_unique<SonarData>();
    outputfile = std::make_unique<std::ofstream>(filename, std::ofstream::binary);

    state = ThreadSSState::TSSState_Init;
    thread = std::make_unique<std::thread>(SonarSerialThreadFunc, this);
}

ThreadSonarSerial::ThreadSonarSerial(std::shared_ptr<serial::Serial> SerialPort, std::string filename, std::function<void(char*, int)> cbfunc) :
    cb_dataready(cbfunc),
    serialport(SerialPort),
    threadkilled(false),
    params_updated(true),
    sonarfailed_(false)
{
    //dcsp = { 1, 0, 1000, 0, 0, 2, 50, 50000, 368, 100000, 0.0F, 0.0F, 0, 80, 0, 0, 0.0F, 0.0F };
    //dssp = { 0, 0, 0, 1, 35138, 0 };
    dcsp = { 0, };
    dssp = { 0, };

    keep_alive_counter = std::chrono::steady_clock::now();

    sonarData = std::make_unique<SonarData>();
    outputfile = std::make_unique<std::ofstream>(filename, std::ofstream::binary);

    state = ThreadSSState::TSSState_Init;
    thread = std::make_unique<std::thread>(SonarSerialThreadFunc, this);
}

ThreadSonarSerial::~ThreadSonarSerial()
{
    KillThread();
}

ThreadSSState ThreadSonarSerial::GetThreadState() const
{
    return state;
}

ThreadSSState ThreadSonarSerial::ThreadInit()
{
    //wxLogDebug(wxT("ThreadSonarSerial::ThreadInit: Start"));

    // DO some preinit
    return ThreadSSState::TSSState_Connecting;
}

ThreadSSState ThreadSonarSerial::ThreadConnecting()
{
    //wxLogDebug(wxT("ThreadSonarSerial::ThreadConnecting: Start"));

    ThreadSSState retvalue = ThreadSSState::TSSState_Disconnected;

    int result = 0;

    for (int i = 0; i < 5; i++)
    {
        result = MRS900_Autobaud();

        if (result < 0)
        {
            //this->Sleep(3000);
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            continue;
        }
        else
        {
            break;
        }
    }

    if (0 == result)
    {
        retvalue = ThreadSSState::TSSState_Connected;
    }

    return retvalue;
}

ThreadSSState ThreadSonarSerial::ThreadConnected()
{
    //wxLogDebug(wxT("ThreadSonarSerial::ThreadConnected: Start"));

    ThreadSSState retvalue = ThreadSSState::TSSState_Disconnected;
    int result;

    // check command mode
    for(int i = 0; i < 10; i++)
    {
        //this->Sleep(100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        result = MRS900_IsCommandMode();

        if(0 == result)
        {
            break;
        }
    }

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::ThreadConnected: Go to Command mode failed"));
        return ThreadSSState::TSSState_Disconnected;
    }

    return ThreadSSState::TSSState_SetSettings;
}

//////////////////////////////////////////////
ThreadSSState ThreadSonarSerial::ThreadWorking()
{
    //wxLogDebug(wxT("ThreadSonarSerial::ThreadWorking: Start"));

    ThreadSSState retvalue = ThreadSSState::TSSState_Disconnected;
    int result = 0;

    auto linebuffer = std::make_unique<uint8_t[]>(sonarData->GetSamplesPerLine());

    int gAngle = 0;
    //float gGyroAngle;

    //int gTraceCompass = 0;
    //int gTraceGyro = 0;
    //int gTraceHead = 0;

    static int prev_angle = -1;
    static int curr_angle = -1;

    int in_angle = 0;
    bool bResult = true;

    //keep_alive_counter = std::chrono::steady_clock::now();

    //for (;;)
    {
        if ((result = MRS900_GetLine(&linebuffer[0])) < 0)        
        {
            //std::cout << "GetLine() result => " << result << "\n";

            if (-5 == result)
            {
                return ThreadSSState::TSSState_Disconnected;
            }

            // Continue until timeout
            return ThreadSSState::TSSState_Working;
            //if (result == -7)
            //{
            //    return ThreadSSState::TSSState_Working;
            //}

            //return ThreadSSState::TSSState_Disconnected;
        }

        //std::cout << "GetLine() result => " << result << "\n";

        PDATAHEADER pdh = reinterpret_cast<PDATAHEADER>(&linebuffer[0]);
        PDATAFOOTER pdf = reinterpret_cast<PDATAFOOTER>(&linebuffer[pdh->samples - sizeof(DATAFOOTER)]);
        PCOMMANDID  recvid = reinterpret_cast<PCOMMANDID>(&pdh->commandid);

        if (0xFFFFFFFF == pdh->angle)
        {
            return ThreadSSState::TSSState_Working;
        }

        if (pdf->magic == 826560069) // END1 case
        {
            auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - keep_alive_counter);

            if (period.count() > 1000L)
            {
                keep_alive_counter = std::chrono::steady_clock::now();

                // Send Keep-alive datagram once per second
                MRS900_SendCommand(BIN_COMMAND_START, nullptr);
            }
        }

        COMMANDID cid = *recvid;

        //uint32_t icid = *((uint32_t*)&cid);
        //std::string tracebuf = "cid before = " + std::to_string(icid);
        //wxLogDebug(wxString(tracebuf));

        in_angle = pdh->angle / 9;
        in_angle = (0 == in_angle) ? 0 : (1 == cid.headup) ? sonarData->GetLinesPerFullTurn() - in_angle : in_angle;
        in_angle = std::abs(in_angle);
        in_angle %= sonarData->GetLinesPerFullTurn();


        //pappdata->recvcid = *reinterpret_cast<PCOMMANDID>(&pdh->commandid);
        //pappdata->filerange = GetRangeFromHeader(pdh);
        //pappdata->deviceId = pdh->deviceid;

        // Conver Dataheader to v3
        {
            bResult = true;

            cb_dataready(reinterpret_cast<char*>(linebuffer.get()), static_cast<int>(pdh->samples));

            if (sizeof(DATAHEADERV1) == pdh->dataoffset)
            {
                // v1 DATAHEADER
                DATAHEADERV3 dhv3;
                memcpy(&dhv3, pdh, sizeof(DATAHEADERV3));

                dhv3.dataoffset = sizeof(DATAHEADERV3);
                dhv3.gyro = 0;
                dhv3.compass = 0;
                dhv3.latitude = 0;
                dhv3.longitude = 0;
                dhv3.samples += 16;

                if(false != outputfile->is_open())
                {
                    outputfile->write(reinterpret_cast<char *>(&dhv3), sizeof(DATAHEADERV3));
                    outputfile->write(reinterpret_cast<char *>(&linebuffer[sizeof(DATAHEADERV1)]), pdh->samples - sizeof(DATAHEADERV1));
                }
            }
            else if (sizeof(DATAHEADERV2) == pdh->dataoffset)
            {
                // v2 DATAHEADER
                DATAHEADERV3 dhv3;
                memcpy(&dhv3, pdh, sizeof(DATAHEADERV3));

                dhv3.dataoffset = sizeof(DATAHEADERV3);
                dhv3.latitude = 0;
                dhv3.longitude = 0;
                dhv3.samples += 8;

                if (false != outputfile->is_open())
                {
                    outputfile->write(reinterpret_cast<char *>(&dhv3), sizeof(DATAHEADERV3));
                    outputfile->write(reinterpret_cast<char *>(&linebuffer[sizeof(DATAHEADERV2)]), pdh->samples - sizeof(DATAHEADERV2));
                }
            }
            else
            {
                // v3 DATAHEADER
                DATAHEADERV3 dhv3;
                memcpy(&dhv3, pdh, sizeof(DATAHEADERV3));

                dhv3.dataoffset = sizeof(DATAHEADERV3);
                dhv3.gyro = 0;
                dhv3.compass = 0;
                dhv3.latitude = 0;
                dhv3.longitude = 0;

                if (false != outputfile->is_open())
                {
                    outputfile->write(reinterpret_cast<char *>(&dhv3), sizeof(DATAHEADERV3));
                    outputfile->write(reinterpret_cast<char *>(&linebuffer[sizeof(DATAHEADERV3)]), pdh->samples - sizeof(DATAHEADERV3));
                }
            }
        }

        // TODO: Exception should be managed
        if (false == bResult)
        {
            ThreadSSState::TSSState_Disconnected;
        }

        //if (0 != pappdata->viewdialogsettings.stopview)
        //{
        //    return ThreadSSState::TSSState_Working;
        //}

        uint16_t *sonardata = sonarData->GetRawSonarData();

        std::memset(&sonardata[sonarData->GetSamplesPerLine() * in_angle], 0, sonarData->GetSamplesPerLine() * sizeof(uint16_t));

        for (int i = 0; i < pdh->samples - pdh->dataoffset - sizeof(DATAFOOTER); i++)
        {
            auto dz = 0;// *std::next(applists::guideadzone.begin(), pappdata->viewdialogsettings.deadzoneidx);

            if ((i + sizeof(DATAHEADER)) <= (static_cast<int>(dz * 100.0) + sizeof(DATAHEADER)))
            {
                continue;
            }

            uint16_t sample = linebuffer[i + pdh->dataoffset];
            sample = uncompand8to12b[sample];

            double gain = 0.0;// *std::next(applists::guigain.begin(), pappdata->viewdialogsettings.gainidx);
            double dsample = gain * static_cast<double>(sample);

            sample = static_cast<uint16_t>(dsample);
            sample = (sample > 4095) ? 4095 : sample;

            sonardata[(sonarData->GetSamplesPerLine() * in_angle) + i] = sample;
        }

        //std::cout << "prev_angle => " << prev_angle << " in_angle=> " << in_angle << "\n";
        /// Fill memory between 2 consecutive received data lines
        ///

        curr_angle = (in_angle == 0 && prev_angle > 1599) ? 3199 : in_angle;
        prev_angle = (in_angle > 1599 && prev_angle == 0) ? 3199 : prev_angle;

#ifdef SECTOR32TEST
        if ((prev_angle != -1) && (labs(prev_angle - (int)curr_angle) < 200))
#else
        if ((prev_angle != -1) && (std::abs(prev_angle - curr_angle) < 20))
#endif
        {
            int sign = ((prev_angle - curr_angle) > 0) ? 1 : -1;

            if (sign == -1)
            {
                for (int i = prev_angle; i < curr_angle; i++)
                {
                    int begin = sonarData->GetSamplesPerLine() * i;
                    int end = sonarData->GetSamplesPerLine() * i + sonarData->GetSamplesPerLine();

                    int begin_inangle = sonarData->GetSamplesPerLine() * in_angle;
                    int end_inangle = sonarData->GetSamplesPerLine() * in_angle + sonarData->GetSamplesPerLine();

                    std::fill(sonardata + begin, sonardata + end, 0);
                    std::copy(sonardata + begin_inangle, sonardata + end_inangle, sonardata + begin);
                }
            }
            else
            {
                for (int i = prev_angle; i > curr_angle; i--)
                {
                    int begin = sonarData->GetSamplesPerLine() * i;
                    int end = sonarData->GetSamplesPerLine() * i + sonarData->GetSamplesPerLine();

                    int begin_inangle = sonarData->GetSamplesPerLine() * in_angle;
                    int end_inangle = sonarData->GetSamplesPerLine() * in_angle + sonarData->GetSamplesPerLine();

                    std::fill(sonardata + begin, sonardata + end, 0);
                    std::copy(sonardata + begin_inangle, sonardata + end_inangle, sonardata + begin);
                }
            }
        }

        prev_angle = in_angle;
        gAngle = in_angle;

//#if 1
//        gGyroAngle = 0;
//#else
//#endif
    }

    if (true == params_updated)
    {
        for (int i = 0; i < 40; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (0 == MRS900_Work2Command())
            {
                break;
            }
        }

        return ThreadSSState::TSSState_Connected;
    }

    //pappdata->pointerPosition = in_angle; // Current angle
    //pappdata->newsonardata = true;

    return ThreadSSState::TSSState_Working;
}

ThreadSSState ThreadSonarSerial::ThreadSetSettings()
{
    //wxLogDebug(wxT("ThreadSonarSerial::ThreadSetSettings: Start"));

    int result;

    if (true == GetSonarParams())
    {
        //wxLogDebug(wxT("ThreadSonarSerial::ThreadSetSettings: true == GetSonarParams()"));

        //DATAGCOMMONSONARPARAM dcsp = { 0, }; //pappdata->dcsp;
        //DATAGSCANSONARPARAM dssp = { 0, }; //pappdata->dssp;

        DATAGCOMMONSONARPARAM dcsp_local = dcsp.load();
        DATAGSCANSONARPARAM dssp_local = dssp.load();

        result = MRS900_SetParams(&dcsp_local, &dssp_local);

        //this->Sleep(10);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (result < 0)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::ThreadSetSettings: MRS900_SetParams failed"));
            return ThreadSSState::TSSState_Disconnected;
        }
    }

    result = MRS900_Command2Work();
    //this->Sleep(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::ThreadSetSettings: MRS900_Command2Work failed\n"));
        return ThreadSSState::TSSState_Disconnected;
    }

    return ThreadSSState::TSSState_Working;
}

int ThreadSonarSerial::MRS900_Responsecheck()
{
    int result = 0;

    uint8_t magicidbuffer[4] = { '0', '0', '0', '0' };

    const uint8_t oktoken[4] = { '#', 'O', 'K', '\n' };
    const uint8_t ertoken[4] = { '#', 'E', 'R', '\n' };

    auto time_begin = std::chrono::steady_clock::now();

    for (;;)
    {
        uint8_t ch;
        std::size_t br = serialport->read(&ch, 1);

        if (br > 0)
        {
            std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
            magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

            if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), oktoken))
            {
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Responsecheck: Response: #OK"));
                result = 0;
                break;
            }
            else if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), ertoken))
            {
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Responsecheck: Response: #ER"));
                result = 1;
                break;
            }
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > 2000LL)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Responsecheck: timeout"));
            result = -2;
            break;
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_Responsecheck(char *responsedata)
{
    int result = 0;

    uint8_t magicidbuffer[4] = { '0', '0', '0', '0' };

    const uint8_t oktoken[4] = { '#', 'O', 'K', '\n' };
    const uint8_t ertoken[4] = { '#', 'E', 'R', '\n' };

    auto time_begin = std::chrono::steady_clock::now();

    bool responsefind = true;
    int responsecnt = 0;

    for (;;)
    {
        uint8_t ch;
        std::size_t br = serialport->read(&ch, 1);

        if (br > 0)
        {
            std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
            magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

            if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), oktoken))
            {
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Responsecheck: Response: #OK"));
                result = 0;
                break;
            }
            else if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), ertoken))
            {
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Responsecheck: Response: #ER"));
                result = 1;
                break;
            }

            if (ch == '<')
            {
                responsefind = true;
                responsecnt = 0;
            }
            else if (ch == '>')
            {
                if (nullptr != responsedata)
                {
                    responsedata[responsecnt] = 0;
                    responsefind = false;
                }
            }
            else if ((false != responsefind) && (responsecnt < 63) && (nullptr != responsedata))
            {
                if (nullptr != responsedata)
                {
                    responsedata[responsecnt++] = ch;
                }
            }
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > 2000LL)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Responsecheck: timeout"));
            result = -2;
            break;
        }
    }

    if ((false != responsefind) && (responsecnt < 63) && (nullptr != responsedata))
    {
        responsedata[responsecnt] = 0;
    }

    return result;
}

void ThreadSonarSerial::SetSonarParams(const PDATAGCOMMONSONARPARAM pdcsp, const PDATAGSCANSONARPARAM pdssp)
{
    dcsp = *pdcsp;
    dssp = *pdssp;

    params_updated = true;
}

void ThreadSonarSerial::SetSonarParams()
{
    params_updated = true;
}

bool ThreadSonarSerial::GetSonarParams()
{
    bool retvalue = params_updated;
    params_updated = false;

    return retvalue;
}

int ThreadSonarSerial::MRS900_Synccheck()
{
    //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Synccheck: Start"));
    int result = 0;

    uint8_t magicidbuffer[6] = { '0', '0', '0', '0', '0', '0' };
    const uint8_t synctoken[6] = { '#', 'S', 'Y', 'N', 'C', '\n'};

    auto time_begin = std::chrono::steady_clock::now();

    for (;;)
    {
        uint8_t ch;
        std::size_t br = serialport->read(&ch, 1);

        if (br > 0)
        {
            std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
            magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

            if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), synctoken))
            {
                result = 0;
                break;
            }
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > 2000LL)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Synccheck: timeout"));
            result = -2;
            break;
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_Workmodecheck()
{
    int result = 0;

    uint8_t magicidbuffer[6] = { '0', '0', '0', '0', '0', '0' };
    const uint8_t worktoken[6] = { 'W', 'O', 'R', 'K', '\r', '\n' };

    auto time_begin = std::chrono::steady_clock::now();

    for (;;)
    {
        uint8_t ch;
        std::size_t br = serialport->read(&ch, 1);

        if (br > 0)
        {
            std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
            magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

            if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), worktoken))
            {
                result = 0;
                break;
            }
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > 5000LL)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Workmodecheck: timeout"));
            result = -2;
            break;
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_Command2Work()
{
    int result;

    MRS900_SendCommand(BIN_COMMAND_START, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    result = MRS900_Responsecheck();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Command2Work: BIN_COMMAND_START failed"));
        return result;
    }

    result = MRS900_Workmodecheck();

    return result;
}

int ThreadSonarSerial::MRS900_Work2Command()
{
    int result = 0;

    auto time_begin = std::chrono::steady_clock::now();

    for (;;)
    {
        result = MRS900_GetEndOrCmnd();

        if (1 == result)
        {
            MRS900_SendCommand(BIN_COMMAND_STOP, nullptr);
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Work2Command: Workmodecheck END1"));
            // not put continue; here -> check timeout later
        }
        else if (2 == result)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Work2Command: Workmodecheck CMND"));
            result = 0;
            break;
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > 2000L)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Work2Command: Timeout"));
            result = -6;
            break;
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_Commandmodecheck(int timeout)
{
    int result = 0;

    uint8_t magicidbuffer[6] = { '0', '0', '0', '0', '0', '0' };
    const uint8_t cmndtoken[6] = { 'C', 'M', 'N', 'D', '\r', '\n' };

    auto time_begin = std::chrono::steady_clock::now();

    for (;;)
    {
        uint8_t ch;
        std::size_t br = serialport->read(&ch, 1);

        if (br > 0)
        {
            //wxString str;
            //str += ch;
            //wxLogDebug(str);

            std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
            magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

            if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), cmndtoken))
            {
                result = 0;
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Commandmodecheck: success"));
                break;
            }
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > timeout)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Commandmodecheck: timeout"));
            result = -2;
            break;
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_IsCommandMode()
{
    int result = 0;

    for (int i = 0; i < 3; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::size_t bw = serialport->write(reinterpret_cast <const uint8_t *>("\r"), 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (0 == result)
        {
            result = MRS900_Responsecheck();

            if (0 == result || 1 == result)
            {
                result = 0;
                break;
            }
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_GetEndOrCmnd()
{
    int result = 0;

    uint8_t magicidbuffer[6] = { '0', '0', '0', '0', '0', '0' };
    const uint8_t cmndtoken[6] = { 'C', 'M', 'N', 'D', '\r', '\n' };
    const uint8_t end1token[4] = { 'E', 'N', 'D', '1' };

    auto time_begin = std::chrono::steady_clock::now();

    for (;;)
    {
        uint8_t ch;
        std::size_t br = serialport->read(&ch, 1);

        if (br > 0)
        {
            std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
            magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

            if (std::equal(magicidbuffer + 2, magicidbuffer + sizeof(magicidbuffer), end1token))
            {
                result = 1;
                break;
            }
            else if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), cmndtoken))
            {
                result = 2;
                break;
            }
        }

        auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

        if (period.count() > 4000LL)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetEndOrCmnd: timeout"));
            result = -2;
            break;
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_Autobaud()
{
    //wxLogDebug(wxT("ThreadSonarSerial::MRS900_Autobaud: Start"));

    int result = -1;
    std::size_t byteswritten;

    constexpr uint8_t autobaud_symbol = '@';

    byteswritten = serialport->write(&autobaud_symbol, 1);

    result = MRS900_Synccheck();

    if (result < 0)
    {
        return result;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint32_t baudrate = serialport->getBaudrate();

    std::string autobaudstr = std::string("    <") + std::to_string(baudrate) + std::string(">\r");
    byteswritten = serialport->write(autobaudstr);

    result = MRS900_Responsecheck();

    if (0 != result)
    {
        return result;
    }

    //serialport->setBaudrate(baudrate);

    result = MRS900_Commandmodecheck(10000);

    if (0 != result)
    {
        return result;
    }

    return 0;
}

int ThreadSonarSerial::MRS900_GetLine(uint8_t *linebuf)
{
    //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine: Start"));

    int retvalue = 0;

    uint8_t magicidbuffer[4] = { '0', '0', '0', '0' };
    const uint8_t datatoken[4] = { 'D', 'A', 'T', 'A' };
    const uint8_t end0token[4] = { 'E', 'N', 'D', '0' };
    const uint8_t end1token[4] = { 'E', 'N', 'D', '1' };

    enum _states { STATE_GETHEADER, STATE_GETFOOTER, STATE_PROCESSDATA } state;

    int bytesread = 0;

    state = STATE_GETHEADER;

    auto time_begin = std::chrono::steady_clock::now();

    if (STATE_GETHEADER == state)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine: STATE_GETHEADER"));
        //int repeattimes = 0;

        for (;;)
        {
            uint8_t ch;

            std::size_t br = serialport->read(&ch, 1);

            if (br > 0)
            {
                bytesread++;

                std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
                magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

                if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), datatoken))
                {
                    std::copy(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), linebuf);
                    bytesread = sizeof(magicidbuffer);

                    state = STATE_GETFOOTER;
                    break;
                }
                else if (bytesread == sonarData->GetSamplesPerLine())
                {
                    retvalue = -4;
                    break;
                }
                else
                {
                    //
                }
            }

            auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

            if (period.count() > 1000LL)
            {
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine Error: DATA timeout"));
                retvalue = -6;
                break;
            }
        }
    }

    time_begin = std::chrono::steady_clock::now();

    if ((STATE_GETFOOTER == state) && (0 == retvalue))
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine: STATE_GETFOOTER"));
        //int repeattimes = 0;

        for (;;)
        {
            uint8_t ch;
            std::size_t br = serialport->read(&ch, 1);

            if (br > 0)
            {
                linebuf[bytesread++] = ch;

                std::rotate(magicidbuffer, magicidbuffer + 1, magicidbuffer + sizeof(magicidbuffer));
                magicidbuffer[sizeof(magicidbuffer) - 1] = ch;

                if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), end0token) || 
                    std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), end1token))
                {
                    state = STATE_PROCESSDATA;
                    break;
                }
                else if (std::equal(magicidbuffer, magicidbuffer + sizeof(magicidbuffer), datatoken))
                {
                    //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine Error: DATA detected when ENDx expected"));
                    std::cout << "ThreadSonarSerial::MRS900_GetLine Error: DATA detected when ENDx expected" << "\n";

                    bytesread = 4;
                    continue;
                }
                else if (bytesread == sonarData->GetSamplesPerLine())
                {
                    retvalue = -4;
                    break;
                }
                else
                {
                    //
                }
            }

            auto period = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);

            if (period.count() > 1000LL)
            {
                std::string tracebuf = "period.count() > 1000LL bytesread = " + std::to_string(bytesread);
                std::cout << tracebuf << "\n";
                //wxLogDebug(wxString(tracebuf));
                //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine Error: ENDx timeout"));
                retvalue = -6;
                break;
            }
        }
    }

    if ((STATE_PROCESSDATA == state) && (0 == retvalue))
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine: STATE_PROCESSDATA"));
        // At this point we assume that DATA and ENDx received correctly
        // Check only number of samples is equal bytesread

        PDATAHEADER pdh = reinterpret_cast<PDATAHEADER>(&linebuf[0]);

        //std::string tracebuf = "pdh->samples > bytesread: pdh->samples= " + std::to_string(pdh->samples) + " bytesread= " + std::to_string(bytesread);
        //std::cout << tracebuf << "\n";

        if (pdh->samples > static_cast<uint32_t>(bytesread))
        {
            std::string tracebuf = "pdh->samples > bytesread: pdh->samples= " + std::to_string(pdh->samples) + " bytesread= " + std::to_string(bytesread);
            std::cout << tracebuf << "\n";
            //wxLogDebug(wxString(tracebuf));
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine Error: Wrong number of samples"));
            retvalue = -7;
        }
    }
    else
    {
        retvalue = -1;
    }

    //wxLogDebug(wxT("ThreadSonarSerial::MRS900_GetLine: Finish"));

    return retvalue;
}

int ThreadSonarSerial::MRS900_GetFWVersion(char *version)
{
    int result;

    MRS900_SendCommand(BIN_COMMAND_FWVERSION, nullptr);

    //this->Sleep(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    result = MRS900_Responsecheck(version);

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_SendCommand: BIN_COMMAND_FWVERSION failed"));
    }

    return result;
}

int ThreadSonarSerial::MRS900_GetDeviceType(char *type)
{
    int result;

    MRS900_SendCommand(BIN_COMMAND_DEVICETYPE, nullptr);

    //this->Sleep(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    result = MRS900_Responsecheck(type);

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_SendCommand: BIN_COMMAND_DEVICETYPE failed"));
    }

    return result;
}

int ThreadSonarSerial::MRS900_Reset()
{
    int result;

    MRS900_SendCommand(BIN_COMMAND_RESET, nullptr);

    //this->Sleep(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    result = MRS900_Responsecheck();

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_SendCommand: BIN_COMMAND_RESET failed"));
    }

    return result;
}

int ThreadSonarSerial::MRS900_SetParams(const PDATAGCOMMONSONARPARAM pdcsp, const PDATAGSCANSONARPARAM pdssp)
{
    int result = -2;

    // Clean RS/MRS input buffer
    std::size_t bw = serialport->write(std::string("         \r"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    MRS900_Responsecheck();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    MRS900_SendCommand(BIN_COMMAND_COMMONSETTINGS, (void *)pdcsp);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    result = MRS900_Responsecheck();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (result < 0)
    {
        //wxLogDebug(wxT("ThreadSonarSerial::MRS900_SetParams: BIN_COMMAND_COMMONSETTINGS failed"));
    }
    else
    {
        MRS900_SendCommand(BIN_COMMAND_SCANSETTINGS, (void *)pdssp);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        result = MRS900_Responsecheck();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (result < 0)
        {
            //wxLogDebug(wxT("ThreadSonarSerial::MRS900_SetParams: BIN_COMMAND_SCANSETTINGS failed"));
        }
    }

    return result;
}

int ThreadSonarSerial::MRS900_SendCommand(int command, void *param) const
{
    DEVICECOMMAND devcommand{};

    bool isvalidcommand = true;
    int retvalue = 0;

    uint32_t crc = 0UL;
    uint32_t data = 1UL;

    switch (command)
    {
        case BIN_COMMAND_COMMONSETTINGS:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(param);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(DATAGCOMMONSONARPARAM) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_COMMONSETTINGS;
            devcommand.size = sizeof(DATAGCOMMONSONARPARAM);
            devcommand.checksum = Crc32_ComputeBuf(crc, devcommand.data, sizeof(DATAGCOMMONSONARPARAM));

            //wxLogDebug(wxT("MRS900_SendCommand : Command: BIN_COMMAND_COMMONSETTINGS"));
            break;
        }

        case BIN_COMMAND_SCANSETTINGS:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(param);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(DATAGSCANSONARPARAM) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_SCANSETTINGS;
            devcommand.size = sizeof(DATAGSCANSONARPARAM);
            devcommand.checksum = Crc32_ComputeBuf(crc, devcommand.data, sizeof(DATAGSCANSONARPARAM));

            //wxLogDebug(wxT("MRS900_SendCommand : Command: BIN_COMMAND_SCANSETTINGS"));
            break;
        }

        case BIN_COMMAND_HOSTSETTINGS:
        {
            break;
        }

        case BIN_COMMAND_START:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(&data);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(int32_t) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_START;
            devcommand.size = sizeof(data);
            devcommand.checksum = Crc32_ComputeBuf(crc, &data, sizeof(data));

            //wxLogDebug(wxT("MRS900_SendCommand : Command: BIN_COMMAND_START"));
            break;
        }

        case BIN_COMMAND_STOP:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(&data);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(int32_t) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_STOP;
            devcommand.size = sizeof(data);
            devcommand.checksum = Crc32_ComputeBuf(crc, &data, sizeof(data));

            //wxLogDebug(wxT("MRS900_SendCommand : Command: BIN_COMMAND_STOP"));
            break;
        }

        case BIN_COMMAND_RESET:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(&data);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(int32_t) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_RESET;
            devcommand.size = sizeof(data);
            devcommand.checksum = Crc32_ComputeBuf(crc, &data, sizeof(data));

            //wxLogDebug(wxT("MRS900_SendCommand : Command: BIN_COMMAND_RESET"));
            break;
        }

        case BIN_COMMAND_FWVERSION:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(&data);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(int32_t) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_FWVERSION;
            devcommand.size = sizeof(data);
            devcommand.checksum = Crc32_ComputeBuf(crc, &data, sizeof(data));

            //wxLogDebug(wxT("Command: BIN_COMMAND_FWVERSION"));
            break;
        }

        case BIN_COMMAND_DEVICETYPE:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(&data);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(int32_t) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_DEVICETYPE;
            devcommand.size = sizeof(data);
            devcommand.checksum = Crc32_ComputeBuf(crc, &data, sizeof(data));

            //wxLogDebug(wxT("Command: BIN_COMMAND_DEVICETYPE"));
            break;
        }

        case BIN_COMMAND_EEPROMDIRECT:
        {
            int32_t *pdcsp = reinterpret_cast<int32_t *>(param);

            std::fill(devcommand.data, devcommand.data + sizeof(devcommand.data) / sizeof(int32_t), 0);
            std::copy(pdcsp, pdcsp + sizeof(EEPROMDIRECT) / sizeof(int32_t), devcommand.data);

            devcommand.magic = 1145982275;
            devcommand.command = BIN_COMMAND_EEPROMDIRECT;
            devcommand.size = sizeof(EEPROMDIRECT);
            devcommand.checksum = Crc32_ComputeBuf(crc, devcommand.data, sizeof(EEPROMDIRECT));

            //wxLogDebug(wxT("Command: BIN_COMMAND_EEPROMDIRECT"));
            break;
        }

        default:
            isvalidcommand = false;
            retvalue = -1;
            break;
    }

    if (false != isvalidcommand)
    {
        auto encodeddata = std::make_unique<B64Encode>(&devcommand, 4 * sizeof(int32_t) + devcommand.size, "\r");
        const std::string &b64cmd = encodeddata->GetEncodedData();
        std::size_t bw = serialport->write(b64cmd);
        serialport->flush();

        //wxLogDebug(wxString(b64cmd));
    }

    return retvalue;
}

uint16_t* ThreadSonarSerial::GetSonarData() const
{
    return sonarData->GetRawSonarData();
}
