// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#include "Scansonar.h"
#include "SonarStructures.h"

#include "serial/serial.h"

#include <iostream>

static int GetPingInterval(int samples, int steps, int comspeed)
{
    int t_acqusition = (samples - sizeof(DATAHEADER) - sizeof(DATAFOOTER)) * 11; // aquisition time us // increase on 10% (was ...TER))* 10 )
    int t_step = steps * 1200; // stepping time us
    float comm_mult = (float)((float)samples / ((float)comspeed / 10.0f));
    int t_communicate = static_cast<int>(1000000.0F * comm_mult);
    int t_communicate_extra = t_communicate + t_communicate / 10; // tcommunicate + 10%

    return (t_communicate_extra > (t_acqusition + t_step + 2000)) ? t_communicate_extra : (t_acqusition + t_step + 2000);
}

void Scansonar::SetDefaultSettings()
{
    scansonar_settings_[IdCentralFrequency] = "860000";
    scansonar_settings_[IdFrequencyBand]    = "80000";
    scansonar_settings_[IdToneChirp]        = "0";
    scansonar_settings_[IdTxLength]         = "20";
    scansonar_settings_[IdSamplFreq]        = "100000";
    scansonar_settings_[IdSamples]          = "1376";   // 10meters
    scansonar_settings_[IdInterval]         = std::to_string(GetPingInterval(1376, 1, GetSerialPort()->getBaudrate()));
    scansonar_settings_[IdGain]             = "0.0";
    scansonar_settings_[IdTVGTime]          = "80";
    scansonar_settings_[IdCommandID]        = "1024";

    scansonar_settings_[IdSectorHeading]    = "0";
    scansonar_settings_[IdRotationParam]    = "0";
    scansonar_settings_[IdSectorWidth]      = "0";
    scansonar_settings_[IdSteppingMode]     = "1";
}

int Scansonar::SendSettings()
{
    DATAGCOMMONSONARPARAM dcsp = { 0, };

    const auto &cfv = scansonar_settings_[IdCentralFrequency];

    if (false == cfv.empty())
    {
        dcsp.central_frequency = std::stoi(cfv);
    }
    else
    {
        return -1;
    }

    const auto& fbv = scansonar_settings_[IdFrequencyBand];

    if (false == fbv.empty())
    {
        dcsp.frequency_band = std::stoi(fbv);
    }
    else
    {
        return -1;
    }

    const auto& tone = scansonar_settings_[IdToneChirp];

    if (false == tone.empty())
    {
        dcsp.chirp_tone = std::stoi(tone);
    }
    else
    {
        return -1;
    }

    const auto& txlen = scansonar_settings_[IdTxLength];

    if (false == txlen.empty())
    {
        dcsp.pulse_length = std::stoi(txlen);
    }
    else
    {
        return -1;
    }

    const auto& sf = scansonar_settings_[IdSamplFreq];

    if (false == sf.empty())
    {
        dcsp.sample_frequency = std::stoi(sf);
    }
    else
    {
        return -1;
    }

    const auto& samples = scansonar_settings_[IdSamples];

    if (false == samples.empty())
    {
        dcsp.samples = std::stoi(samples);
    }
    else
    {
        return -1;
    }

    const auto& gain = scansonar_settings_[IdGain];

    if (false == gain.empty())
    {
        dcsp.gain = std::stof(gain);
    }
    else
    {
        return -1;
    }

    const auto& tvgtime = scansonar_settings_[IdTVGTime];

    if (false == tvgtime.empty())
    {
        dcsp.tvg_time = std::stoi(tvgtime);
    }
    else
    {
        return -1;
    }

    const auto& commandid = scansonar_settings_[IdCommandID];

    if (false == commandid.empty())
    {
        dcsp.commandid = std::stoi(commandid);
    }
    else
    {
        return -1;
    }

    DATAGSCANSONARPARAM dssp = { 0, };

    const auto& shv = scansonar_settings_[IdSectorHeading];

    if (false == shv.empty())
    {
        dssp.sector_heading = std::stoi(shv);
    }
    else
    {
        return -1;
    }

    const auto& swv = scansonar_settings_[IdSectorWidth];

    if (false == swv.empty())
    {
        dssp.sector_width = std::stoi(swv);
    }
    else
    {
        return -1;
    }

    const auto& rpv = scansonar_settings_[IdRotationParam];

    if (false == rpv.empty())
    {
        dssp.rotation_parameters = std::stoi(rpv);
    }
    else
    {
        return -1;
    }

    const auto& smv = scansonar_settings_[IdSteppingMode];

    if (false == smv.empty())
    {
        dssp.stepping_mode = std::stoi(smv);
    }
    else
    {
        return -1;
    }

    const auto& interval = std::to_string(GetPingInterval(dcsp.samples, dssp.stepping_mode, GetSerialPort()->getBaudrate()));
    scansonar_settings_[IdInterval] = interval; // Update interval value internally

    if (false == interval.empty())
    {
        dcsp.ping_interval = std::stoi(interval);
    }
    else
    {
        return -1;
    }

    threadsonarserial_->SetSonarParams(&dcsp, &dssp);

    return 0;
}

Scansonar::Scansonar(std::shared_ptr<serial::Serial> SerialPort, std::wstring filename, std::function<void(char*, int)> cbfunc, std::map<int, ScansonarCommandList>& CommandList) :
    serial_port_(SerialPort),
    is_detected_(false)
{
    threadsonarserial_ = std::make_unique<ThreadSonarSerial>(SerialPort, filename, cbfunc);

    SetDefaultSettings();
    SendSettings();
}

Scansonar::Scansonar(std::shared_ptr<serial::Serial> SerialPort, std::string filename, std::function<void(char*, int)> cbfunc, std::map<int, ScansonarCommandList>& CommandList) :
    serial_port_(SerialPort),
    is_detected_(false)
{
    threadsonarserial_ = std::make_unique<ThreadSonarSerial>(SerialPort, filename, cbfunc);

    SetDefaultSettings();
    SendSettings();
}


bool Scansonar::SetValue(ScansonarCommandIds Command, const std::string& SonarValue)
{
    bool retvalue = false;

    float fvalue = std::stof(SonarValue);
    int ivalue = std::stoi(SonarValue);

    switch (Command)
    {
        case IdCommandID:
        {
            scansonar_settings_[IdCommandID] = SonarValue;
            retvalue = true;
            break;
        }
        case IdSamples:
        {
            int minvalue = 240;
            int maxvalue = 13340;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdSamples] = SonarValue;
            }

            break;
        }
        case IdGain:
        {
            float minvalue = -15;
            float maxvalue = 15;
            
            retvalue = (fvalue < minvalue) ? false : (fvalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdGain] = SonarValue;
            }

            break;
        }
        case IdCentralFrequency:
        {
            int minvalue = 10000;
            int maxvalue = 1000000;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdCentralFrequency] = SonarValue;
            }

            break;
        }
        case IdFrequencyBand:
        {
            int minvalue = 1;
            int maxvalue = 100000;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdFrequencyBand] = SonarValue;
            }

            break;
        }
        case IdToneChirp:
        {
            int minvalue = 0;
            int maxvalue = 6;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdToneChirp] = SonarValue;
            }

            break;
        }
        case IdTVGTime:
        {
            int minvalue = 80;
            int maxvalue = 200;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdTVGTime] = SonarValue;
            }

            break;
        }
        case IdTxLength:
        {
            int minvalue = 10;
            int maxvalue = 100;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdTxLength] = SonarValue;
            }

            break;
        }
/*
        case IdInterval:
        Interval should be calculated depending of F(samples, steps, comspeed)
*/
        case IdRange:
        {
            int minvalue = 1;
            int maxvalue = 100;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdRange] = SonarValue;
            }

            break;
        }
        case IdSound:
        {
            float minvalue = 1000.0F;
            float maxvalue = 2000.0F;

            retvalue = (fvalue < minvalue) ? false : (fvalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdSound] = SonarValue;
            }

            break;
        }
        case IdThreshold:
        {
            int minvalue = 5;
            int maxvalue = 98;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdThreshold] = SonarValue;
            }

            break;
        }
        case IdSamplFreq:
        {
            int minvalue = 100000; // for firmware 1.04
            int maxvalue = 100000;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdSamplFreq] = SonarValue;
            }

            break;
        }
        case IdSectorHeading:
        {
            int minvalue = 0;
            int maxvalue = 28800;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdSectorHeading] = SonarValue;
            }

            break;
        }
        case IdSectorWidth:
        {
            int minvalue = 0;
            int maxvalue = 28800;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdSectorWidth] = SonarValue;
            }

            break;
        }
        case IdRotationParam:
        {
            int minvalue = 0;
            int maxvalue = 1;

            retvalue = (ivalue < minvalue) ? false : (ivalue > maxvalue) ? false : true;

            if (true == retvalue)
            {
                scansonar_settings_[IdRotationParam] = SonarValue;
            }

            break;
        }
        case IdSteppingMode:
        {
            int validvalues[] = { 0, 1, 2, 4, 8, 16 };

            for (auto &value : validvalues)
            {
                if (ivalue == value)
                {
                    retvalue = true;
                    break;
                }
            }

            if (true == retvalue)
            {
                scansonar_settings_[IdSteppingMode] = SonarValue;
            }

            break;
        }
    }

    return retvalue;
}

const std::string& Scansonar::GetValue(ScansonarCommandIds command)
{
    return scansonar_settings_[command];
}

bool Scansonar::Detect()
{
    return !threadsonarserial_->sonarfailed_;
}

bool Scansonar::IsRunning()
{
    return false;
}

bool Scansonar::IsDetected()
{
    return !threadsonarserial_->sonarfailed_;

}

std::shared_ptr<serial::Serial>& Scansonar::GetSerialPort()
{
    return serial_port_;
}

void Scansonar::GetSettings()
{
}

void Scansonar::SetSettings()
{
}

void Scansonar::Start()
{
    SendSettings();
}

void Scansonar::Stop()
{

}

uint16_t* Scansonar::GetRawSonarData() const
{
    return threadsonarserial_->GetSonarData();
}