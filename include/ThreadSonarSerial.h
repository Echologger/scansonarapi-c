// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#pragma once

#include <cstdint>
#include <atomic>
#include <initializer_list>
#include <thread>
#include <memory>
#include <chrono>
#include <functional>

#include "serial/serial.h"
#include "SonarData.h"
#include "SonarStructures.h"

enum class ThreadSSState { TSSState_Init, TSSState_Connecting, TSSState_Connected,
                           TSSState_Working, TSSState_SetSettings, TSSState_Disconnected
                         };

class ThreadSonarSerial final
{
public:

    ThreadSonarSerial(ThreadSonarSerial &other) = delete;

    ThreadSonarSerial(std::shared_ptr<serial::Serial> SerialPort, std::wstring filename = L"", std::function<void(char*, int)> cbfunc = [](char* line, int num) {});
    ThreadSonarSerial(std::shared_ptr<serial::Serial> SerialPort, std::string filename = "", std::function<void(char*, int)> cbfunc = [](char* line, int num) {});

    ~ThreadSonarSerial();

    ThreadSSState GetThreadState() const;

    uint16_t* GetSonarData() const;

    void SetSonarParams();
    void SetSonarParams(const PDATAGCOMMONSONARPARAM pdcsp, const PDATAGSCANSONARPARAM pdssp);

    ThreadSSState ThreadInit();
    ThreadSSState ThreadWorking();

    ThreadSSState ThreadConnecting();

    ThreadSSState ThreadConnected();
    ThreadSSState ThreadSetSettings();

    std::shared_ptr<serial::Serial> serialport;
    std::atomic<ThreadSSState> state;

    std::atomic<bool> threadkilled;
    std::unique_ptr<std::thread> thread;

    void KillThread() 
    { 
        threadkilled = true; 
        thread->join(); 
    }

    std::unique_ptr<std::ofstream> outputfile;
    std::chrono::steady_clock::time_point keep_alive_counter;

    std::function<void(char*, int)> cb_dataready; // Call on data arrived / for preprocess

    std::atomic<bool> sonarfailed_;

private:

    int MRS900_Synccheck();
    int MRS900_Workmodecheck();
    int MRS900_Commandmodecheck(int timeout);
    int MRS900_Responsecheck();
    int MRS900_Responsecheck(char *responseline);
    int MRS900_GetFWVersion(char *version);
    int MRS900_GetDeviceType(char *type);
    int MRS900_IsCommandMode();
    int MRS900_Reset();
    int MRS900_GetEndOrCmnd();

    int MRS900_Command2Work();
    int MRS900_Work2Command();

    int MRS900_SetParams(const PDATAGCOMMONSONARPARAM pdcsp, const PDATAGSCANSONARPARAM pdssp);
    bool GetSonarParams();

    int MRS900_Autobaud();

    int MRS900_GetLine(uint8_t *linebuf);
    int MRS900_SendCommand(int command, void *param) const;

    std::unique_ptr<SonarData> sonarData;

    std::atomic<bool> params_updated;

    std::atomic<DATAGCOMMONSONARPARAM> dcsp;
    std::atomic<DATAGSCANSONARPARAM> dssp;
};
