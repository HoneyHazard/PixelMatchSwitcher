#pragma once

#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <qmetatype.h>

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

namespace std
{
    template<> struct hash<OBSWeakSource>
    {
        std::size_t operator()(OBSWeakSource const& s)
        {
            obs_source_t* source = obs_weak_source_get_source(s);
            return std::hash<obs_source_t*>{}(source);
        }
    };
    template<> struct hash<pair<OBSWeakSource,OBSWeakSource>>
    {
        std::size_t operator()(pair<OBSWeakSource,OBSWeakSource> p)
        {
            size_t first = std::hash<OBSWeakSource>{}(p.first);
            size_t second = std::hash<OBSWeakSource>{}(p.second);
            return first + (second >> 16) | ((second & 0xFFFF) << 48);
        }
    };
}

struct PmSceneConfig
{
    OBSWeakSource matchScene;
    OBSWeakSource noMatchScene;
    std::unordered_map<std::pair<OBSWeakSource, OBSWeakSource>, OBSWeakSource>
        transitions;
};

typedef std::unordered_set<OBSWeakSource> PmScenes;

inline void pmRegisterMetaTypes()
{
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmSceneConfig>("PmSceneConfig");
    qRegisterMetaType<PmScenes>("PmScenes");
}
