// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#if !defined(SCANSONAR_H)
#define SCANSONAR_H

#include "ISonar.h"
#include <cstdint>
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <map>

#include "serial/serial.h"
#include "ScansonarCommands.h"
#include "ThreadSonarSerial.h"

namespace
{
    std::map<int, ScansonarCommandList> ScansonarCommands =
    {
        { ScansonarCommandIds::IdInfo,            { "#info",       "",      ""}},
        { ScansonarCommandIds::IdRange,           { "#range",      "50000", " - #range[ ]{0,}\\[[ ]{0,}([0-9]{1,}) mm[ ]{0,}\\].*"}},
        { ScansonarCommandIds::IdInterval,        { "#interval",   "0.1",   " - #interval[ ]{0,}\\[[ ]{0,}(([0-9]*[.])?[0-9]+) sec[ ]{0,}\\].*"}},
        { ScansonarCommandIds::IdTxLength,        { "#txlength",   "50",    " - #txlength[ ]{0,}\\[[ ]{0,}([0-9]{1,}) uks[ ]{0,}\\].*" }},
        { ScansonarCommandIds::IdGain,            { "#gain",       "0.0",   " - #gain[ ]{0,}\\[[ ]{0,}([+-]?([0-9]*[.])?[0-9]+) dB[ ]{0,}\\].*" }},
        { ScansonarCommandIds::IdSound,           { "#sound",      "1500",  " - #sound[ ]{0,}\\[[ ]{0,}([0-9]{1,}) mps[ ]{0,}\\].*" }},
        { ScansonarCommandIds::IdDeadzone,        { "#deadzone",   "300",   " - #deadzone[ ]{0,}\\[[ ]{0,}([0-9]{1,}) mm[ ]{0,}\\].*" }},
        { ScansonarCommandIds::IdThreshold,       { "#threshold",  "10",    " - #threshold[ ]{0,}\\[[ ]{0,}([0-9]{1,}) %[ ]{0,}\\].*" }},
        { ScansonarCommandIds::IdOutput,          { "#output",     "3",     " - #output[ ]{0,}\\[[ ]{0,}([0-9]{1,})[ ]{0,}\\].*" }},
        { ScansonarCommandIds::IdVersion,         { "#version",    "",      " S\\/W Ver: ([0-9]{1,}[.][0-9]{1,}) .*" }},
        { ScansonarCommandIds::IdGo,              { "#go",         "",      "" }}
    };
}

class Scansonar : public ISonar
{
    /**
    *   Scaning Sonar Thread class used by echosounder
    */
    std::unique_ptr<ThreadSonarSerial>threadsonarserial_;

    /**
    *   Serial port class used by echosounder
    */
    std::shared_ptr<serial::Serial>serial_port_;

    /**
    *   Data return by the unit after host issued command to it
    */
    //std::string command_result_;

    /**
    *   #info command result line by line
    */
    //std::vector<std::string> info_lines_;

    /**
    *   This map contains all available command for the echosounder
    */
    std::map<int, ScansonarCommandList> scansonar_commands_;

    /**
    *   This map contains all current settings of the echosounder
    */
    std::map<ScansonarCommandIds_t, std::string> scansonar_settings_;

    /**
    *   Current running status of the echosounder
    */
    bool is_running_;

    /**
    *   Current detected status of the echosounder
    */
    bool is_detected_;

    /**
     *   @brief Send command to the echosounder
     *   @param command - command to send
     *   @return 1 - command successfuly execute, 2 - invalid argument, 3 - invalid command, -2 - timeout occured
     */
    //int SendCommand(ScansonarCommandIds command);
    int SendSettings(); // Send scansonar_settings_ to sonar

    /**
     *   @brief Receive responce for command sent to the echosounder
     *   @return 1 - command successfuly execute, 2 - invalid argument, 3 - invalid command, -2 - timeout occured
     */
    //int SendCommandResponseCheck();

    /**
     *   @brief Waiting until echosounder send back "command prompt" character
     *   @param timeoutms - timeout in milliseconds
     *   @return 1 - command prompt received, -2 - timeout occured
     */
    //int WaitCommandPrompt(int64_t timeoutms) const;

    //void GetAllValues();
    //void SetAllValues();

    /**
     *   @brief Send #info command and parse it to echosounder_settings_
     *   @return 1 - command successfuly execute, 2 - invalid argument, 3 - invalid command, -2 - timeout occured
     */
    //int GetSonarInfo();

public:

    /**
        Constructor
    */
    //Scansonar(std::shared_ptr<serial::Serial> SerialPort, std::map<int, ScansonarCommandList>& CommandList = ScansonarCommands);
    Scansonar(std::shared_ptr<serial::Serial> SerialPort, std::wstring filename = L"", const std::function<void(char*, int)> cbfunc = [](char* line, int num){}, std::map<int, ScansonarCommandList>& CommandList = ScansonarCommands);
    Scansonar(std::shared_ptr<serial::Serial> SerialPort, std::string filename = "", const std::function<void(char*, int)> cbfunc = [](char* line, int num){}, std::map<int, ScansonarCommandList>& CommandList = ScansonarCommands);

    /**
    *   @brief Set default scanning sonar settings
    */
    void SetDefaultSettings();

    /**
    *   @brief Set echosounder's value
    */
    bool SetValue(ScansonarCommandIds Command, const std::string& SonarValue);

    /**
    *   @brief Get echosounder's value. Value is stored internally in the class.
    *   @return std::string reference to Value
    */
    const std::string& GetValue(ScansonarCommandIds command);

    /**
    *   @brief Get serial port used for access to echosounder.
    *   @return std::shared_ptr<serial::Serial> reference
    */
    std::shared_ptr<serial::Serial>& GetSerialPort();

    uint16_t* GetRawSonarData() const;

    virtual void GetSettings() override;
    virtual void SetSettings() override;
    virtual void Start() override;
    virtual void Stop() override;

    virtual bool Detect() override;

    virtual bool IsRunning() override;
    virtual bool IsDetected() override;
};

#endif // SINGLESONAR_H
