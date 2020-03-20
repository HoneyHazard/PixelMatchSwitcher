#pragma once

#include <stdint.h>
#include <QString>

enum class PmMaskMode : int {
    GreenMode=0,
    MagentaMode=1,
    BlackMode=2,
    AlphaMode=3,
    CustomClrMode=4
};

struct PmResultsPacket
{
    int baseWidth, baseHeight;
    uint32_t matchImgWidth, matchImgHeight;
    uint32_t numCompared;
    uint32_t numMatched;
    float percentageMatched;
    bool isMatched;
};

struct PmConfigPacket
{
    int roiLeft = 0, roiBottom = 0;
    float perPixelErrThresh = 25.f;
    float totalMatchThresh = 90.f;
    PmMaskMode maskMode = PmMaskMode::GreenMode;
    uint32_t customColor = 0xff00ff00;
    float previewVideoScale = 0.5f;
    // bool visualize; // TODO
    // Color customColor; // TODO
};
