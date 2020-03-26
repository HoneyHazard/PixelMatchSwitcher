#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
#include <QSet>
#include <QHash>

#include <obs.h>
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

struct PmMatchResults
{
    int baseWidth, baseHeight;
    int matchImgWidth, matchImgHeight;
    uint32_t numCompared;
    uint32_t numMatched;
    float percentageMatched;
    bool isMatched;
};

struct PmMatchConfig
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

static uint qHash(const OBSWeakSource &ws)
{
    obs_source_t* source = obs_weak_source_get_source(ws);
    return qHash(source);
}

struct PmSceneConfig
{
    bool isEnabled = false;
    OBSWeakSource matchScene;
    OBSWeakSource noMatchScene;
    OBSWeakSource defaultTransition;
    QHash<QPair<OBSWeakSource, OBSWeakSource>, OBSWeakSource>
        transitions;
};

typedef QSet<OBSWeakSource> PmScenes;

inline void pmRegisterMetaTypes()
{
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmSceneConfig>("PmSceneConfig");
    qRegisterMetaType<PmScenes>("PmScenes");
}
