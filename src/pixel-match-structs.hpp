#pragma once

#include <stdint.h>

struct PmResultsPacket
{
    uint32_t baseWidth, baseHeight;
    uint32_t matchImgWidth, matchImgHeight;
    uint32_t numCompared;
    uint32_t numMatched;
    float percentageMatched;
    bool isMatched;
};

struct  PmConfigPacket
{
    int roiLeft = 0, roiBottom = 0;
    float perPixelErrThresh = 25.f;
    float totalMatchThresh = 90.f;
};
