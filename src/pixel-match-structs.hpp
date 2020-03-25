#pragma once

#include <stdint.h>
#include <QHash>
#include <qmetatype.h>

#include <obs.hpp>


enum class PmMaskMode : int {
    GreenMode=0,
    MagentaMode=1,
    BlackMode=2,
    AlphaMode=3,
    CustomClrMode=4
};

enum class PmPreviewMode : int {
    Video=0,
    Region=1,
    MatchImage=2
};

struct PmMatchResultsPacket
{
    int baseWidth, baseHeight;
    int matchImgWidth, matchImgHeight;
    uint32_t numCompared;
    uint32_t numMatched;
    float percentageMatched;
    bool isMatched;
};

struct PmMatchConfigPacket
{
    int roiLeft = 0, roiBottom = 0;
    float perPixelErrThresh = 25.f;
    float totalMatchThresh = 90.f;
    PmMaskMode maskMode = PmMaskMode::GreenMode;
    uint32_t customColor = 0xff00ff00;
    PmPreviewMode previewMode = PmPreviewMode::Video;
    float previewVideoScale = 0.5f;
    float previewRegionScale = 1.f;
    float previewMatchImageScale = 1.f;
    // bool visualize; // TODO
};

struct PmSceneConfig
{
    OBSWeakSource matchScene;
    OBSWeakSource noMatchScene;
    QHash<QPair<OBSWeakSource, OBSWeakSource>, OBSWeakSource> transitions;
};

inline void pmRegisterMetaTypes()
{
    qRegisterMetaType<PmMatchResultsPacket>("PmMatchResultsPacket");
    qRegisterMetaType<PmMatchConfigPacket>("PmMatchConfigPacket");
    qRegisterMetaType<PmSceneConfig>("PmSceneConfig");
}
