// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#if !defined(SINGLESONARCOMMANDS_H)
#define SINGLESONARCOMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

enum ScansonarCommandIds
{
    IdInfo = 0, 
    IdGo,
    IdRange,
    IdInterval,
    IdTxLength,
    IdCommandID,
    IdGain,

    IdSound,
    IdDeadzone,
    IdThreshold,
    IdOutput,
    IdSamplFreq,
    IdSamples,
    IdVersion,

    IdToneChirp,  // 0 - Tone, 1 - Chirp
    IdTVGTime,
    IdCentralFrequency,
    IdFrequencyBand,
    IdTxPower,
    IdRMSTxPower,

    IdSetHighFreq,
    IdSetLowFreq,

    IdGetHighFreq,
    IdGetLowFreq,
    IdGetWorkFreq,

    IdSectorHeading,
    IdSectorWidth,
    IdSteppingMode,
    IdSteppingTime,
    IdSteppingAngle
};

typedef enum ScansonarCommandIds ScansonarCommandIds_t;

struct ScansonarCommandList
{
    const char* command_text;
    const char* default_value;
    const char* regex_match_text;
};

#ifdef __cplusplus
}
#endif

#endif //!SINGLESONARCOMMANDS_H
