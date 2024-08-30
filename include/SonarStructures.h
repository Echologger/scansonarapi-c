// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#pragma once

#include <cstdint>

enum BIN_COMMANDS_MRS
{
    BIN_COMMAND_COMMONSETTINGS = 0,
    BIN_COMMAND_SCANSETTINGS,
    BIN_COMMAND_COMMONANDSCANSETTINGS,
    BIN_COMMAND_HOSTSETTINGS,
    BIN_COMMAND_EPROMSETTINGS,
    BIN_COMMAND_RESET,
    BIN_COMMAND_START,
    BIN_COMMAND_STOP,
    BIN_COMMAND_ZERO,
    BIN_COMMAND_DEVICETYPE,
    BIN_COMMAND_FWVERSION,
    BIN_COMMAND_EEPROMDIRECT,
    BIN_COMMAND_GYROCALIBRATE
};

#pragma pack(2)
struct _datag_common_sonar_param
{
    uint32_t start_node;
    uint32_t data_format;
    uint32_t commandid;
    uint32_t central_frequency;
    uint32_t frequency_band;
    uint32_t chirp_tone;
    uint32_t pulse_length;
    uint32_t ping_interval;
    uint32_t samples;
    uint32_t sample_frequency;
    float gain;
    float tvg_slope;
    uint16_t tvg_mode;
    uint16_t tvg_time;
    uint16_t sync;
    uint16_t sync_timeout;
    float tx_power;
    float rms_tx_power;
};
#pragma pack()

typedef struct _datag_common_sonar_param   DATAGCOMMONSONARPARAM;
typedef struct _datag_common_sonar_param *PDATAGCOMMONSONARPARAM;

#pragma pack(2)
struct _datag_scan_sonar_param
{
    uint16_t sector_heading;
    uint16_t sector_width;
    uint16_t rotation_parameters;
    uint16_t stepping_mode;
    uint32_t  stepping_time;
    uint32_t  stepping_angle;
};
#pragma pack()

typedef struct _datag_scan_sonar_param   DATAGSCANSONARPARAM;
typedef struct _datag_scan_sonar_param *PDATAGSCANSONARPARAM;

#pragma pack(2)
struct _datag_commonandscan_param
{
    DATAGCOMMONSONARPARAM dcsp;
    DATAGSCANSONARPARAM   dssp;
};
#pragma pack()

typedef struct _datag_commonandscan_param   DATAGCOMMONANDSCANPARAM;
typedef struct _datag_commonandscan_param *PDATAGCOMMONANDSCANPARAM;

#pragma pack(2)
struct _datag_host_param
{
    uint16_t host_latency;
    uint16_t host_timeout;
    uint32_t  host_time;
};
#pragma pack()

typedef struct _datag_host_param   DATAGHOSTPARAM;
typedef struct _datag_host_param *PDATAGHOSTPARAM;

#pragma pack(2)
struct _eeprom_direct
{
    uint16_t page;
    unsigned char data[16];
};
#pragma pack()

typedef struct _eeprom_direct   EEPROMDIRECT;
typedef struct _eeprom_direct *PEEPROMDIRECT;

enum BIN_COMMANDS
{
    BIN_COMMAND_SETTINGS = 0,
    BIN_COMMAND_DEVICEID,
    BIN_COMMAND_TILTOFFSETS
};

struct deviceCommand
{
    int32_t magic;
    int32_t command;
    int32_t checksum;
    int32_t size;
    int32_t data[28];
};

typedef struct deviceCommand   DEVICECOMMAND;
typedef struct deviceCommand *PDEVICECOMMAND;

struct _dataheader_v1
{
    uint32_t magic;
    uint32_t dataoffset;
    uint32_t datasize; // 1,2,4 bytes
    uint32_t samples;
    uint32_t deviceid;
    uint32_t angle;
    uint32_t commandid;
};

//typedef struct _dataheader_v1   DATAHEADER ;
//typedef struct _dataheader_v1 *PDATAHEADER ;
typedef struct _dataheader_v1   DATAHEADERV1;
typedef struct _dataheader_v1 *PDATAHEADERV1;

struct _dataheader_v2
{
    uint32_t magic;
    uint32_t dataoffset;
    uint32_t datasize; // 1,2,4 bytes
    uint32_t samples;
    uint32_t deviceid;
    uint32_t angle;
    uint32_t commandid;
    uint32_t gyro;
    uint32_t compass;
};

typedef struct _dataheader_v2   DATAHEADER;
typedef struct _dataheader_v2 *PDATAHEADER;
typedef struct _dataheader_v2   DATAHEADERV2;
typedef struct _dataheader_v2 *PDATAHEADERV2;

struct _dataheader_v3
{
    uint32_t magic;
    uint32_t dataoffset;
    uint32_t datasize; // 1,2,4 bytes
    uint32_t samples;
    uint32_t deviceid;
    uint32_t angle;
    uint32_t commandid;
    uint32_t gyro;
    uint32_t compass;
    float         latitude;
    float         longitude;
};

typedef struct _dataheader_v3   DATAHEADERV3;
typedef struct _dataheader_v3 *PDATAHEADERV3;

struct _datafooter
{
    uint32_t timestamp;
    uint32_t magic;
};

typedef struct _datafooter   DATAFOOTER;
typedef struct _datafooter *PDATAFOOTER;

struct _olddataheader
{
    int32_t magic;
    int32_t angle;
    int32_t deviceid;
    int32_t stepping_mode;
    int32_t commandid;
    int32_t datasize;
    int32_t samples;
};

typedef struct _olddataheader   OLDDATAHEADER;
typedef struct _olddataheader *POLDDATAHEADER;

struct _olddatafooter
{
    int32_t commandid;
    int32_t magic;
};

typedef struct _olddatafooter   OLDDATAFOOTER;
typedef struct _olddatafooter *POLDDATAFOOTER;

struct _commandid
{
    unsigned version    : 2; // 0
    unsigned chirp_tone : 2; // 2
    unsigned headup     : 1; // 4
    unsigned pulse      : 10; // 5
    unsigned gain       : 4; // 15
    unsigned gain_sign  : 1; // 19
    unsigned step       : 4; // 20
    unsigned heading    : 3; // 24
    unsigned width      : 2; // 27
    unsigned width0     : 1; // 28
    unsigned reserved1  : 2; // 30
};

typedef struct _commandid   COMMANDID;
typedef struct _commandid *PCOMMANDID;
