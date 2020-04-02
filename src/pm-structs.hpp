#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
#include <unordered_map>
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
    int baseWidth = 0, baseHeight = 0;
    int matchImgWidth = 0, matchImgHeight = 0;
    uint32_t numCompared = 0;
    uint32_t numMatched = 0;
    float percentageMatched = 0;
    bool isMatched = false;
};

struct PmMatchConfig
{
    std::string matchImgFilename;
    int roiLeft = 0, roiBottom = 0;
    float perPixelErrThresh = 25.f;
    float totalMatchThresh = 90.f;
    PmMaskMode maskMode = PmMaskMode::GreenMode;
    uint32_t customColor = 0xff00ff00;
    PmPreviewMode previewMode = PmPreviewMode::Video;
    float previewVideoScale = 0.5f;
    float previewRegionScale = 1.f;
    float previewMatchImageScale = 1.f;

    bool operator!= (const PmMatchConfig &other) const
        { return memcmp(this, &other, sizeof(PmMatchConfig)); }
    bool operator== (const PmMatchConfig &other) const
        { return operator!=(other); }
};

typedef std::unordered_map<std::string, PmMatchConfig> PmMatchPresets;

static uint qHash(const OBSWeakSource &ws)
{
    obs_source_t* source = obs_weak_source_get_source(ws);
    return qHash(source);
}

struct PmSwitchConfig
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
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchResults>("PmMatchPresets");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmSwitchConfig>("PmSwitchConfig");
    qRegisterMetaType<PmScenes>("PmScenes");
}
