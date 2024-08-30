// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#include "SonarData.h"
#include <cstring>

SonarData::SonarData(int samplesperline, int linesperfullturn) :
    samplesperline(samplesperline),
    linesperfullturn(linesperfullturn)
{
    sonardata = std::make_unique<uint16_t[]>(samplesperline * linesperfullturn);
    CleanSonarData();
}

SonarData::~SonarData()
{
}

uint16_t *SonarData::GetSample(int line, int position) const
{
    return &sonardata[line * samplesperline + position];
    //return std::next(sonardata, line * samplesperline + position);
}

int SonarData::GetSamplesPerLine() const
{
    return samplesperline;
}

int SonarData::GetLinesPerFullTurn() const
{
    return linesperfullturn;
}

void SonarData::CleanSonarData()
{
    std::fill(sonardata.get(), sonardata.get() + linesperfullturn * samplesperline, 0);
}

uint16_t *SonarData::GetRawSonarData() const
{
    return sonardata.get();
}
