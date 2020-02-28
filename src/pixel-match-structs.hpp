#pragma once

#include <stdint.h>

struct PmResultsPacket
{
    uint32_t baseWidth, baseHeight;
    uint32_t matchImgWidth, matchImgHeight;
    // TODO: more.
};

struct  PmConfigPacket
{
    int roiLeft, roiBottom;
};
