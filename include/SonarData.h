// Copyright (c) EofE Ultrasonics Co., Ltd., 2024
#pragma once

#include <cstdint>
#include <iterator>
#include <memory>

class SonarData final
{
    std::unique_ptr<uint16_t[]> sonardata;

    int samplesperline;
    int linesperfullturn;

public:
    SonarData(int samplesperline = 20400, int linesperfullturn = 3200);
    ~SonarData();

    uint16_t *GetSample(int line, int position) const;

    int GetSamplesPerLine() const;
    int GetLinesPerFullTurn() const;

    void CleanSonarData();

    uint16_t *GetRawSonarData() const;
};
