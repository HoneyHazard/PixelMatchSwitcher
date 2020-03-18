#pragma once

#include <stdint.h>
#include <QString>

enum class PmColorMode : int {
    GreenMode=0,
    MagentaMode=1,
    BlackMode=2,
    AlphaMode=3,
    CustomClrMode=4
};

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
    PmColorMode colorMode = PmColorMode::GreenMode;
    // bool visualize; // TODO
    // Color customColor; // TODO
};
