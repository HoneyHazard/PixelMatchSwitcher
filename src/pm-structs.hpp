#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
#include <unordered_map>
#include <QSet>
#include <QHash>

#include <obs.h>
#include <obs.hpp>

// TODO: organize and add comments

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
    uint32_t baseWidth = 0, baseHeight = 0;
    uint32_t matchImgWidth = 0, matchImgHeight = 0;
    uint32_t numCompared = 0;
    uint32_t numMatched = 0;
    float percentageMatched = 0;
    bool isMatched = false;
};

struct PmMatchConfig
{
    PmMatchConfig() {}
    PmMatchConfig(obs_data_t* data);
    obs_data_t* save(const std::string &presetName) const;

    std::string matchImgFilename;
    int roiLeft = 0, roiBottom = 0;
    float perPixelErrThresh = 25.f;
    float totalMatchThresh = 90.f;
    PmMaskMode maskMode = PmMaskMode::GreenMode;
    uint32_t customColor = 0xffff0000;
    PmPreviewMode previewMode = PmPreviewMode::Video;
    float previewVideoScale = 0.5f;
    float previewRegionScale = 1.f;
    float previewMatchImageScale = 1.f;

    bool operator!= (const PmMatchConfig &other) const;
    bool operator== (const PmMatchConfig &other) const
        { return operator!=(other); }

};

typedef std::unordered_map<std::string, PmMatchConfig> PmMatchPresets;

struct PmSwitchConfig
{
    PmSwitchConfig() {}
    PmSwitchConfig(obs_data_t *data);
    obs_data_t* save() const;

    bool isEnabled = false;
    OBSWeakSource matchScene;
    OBSWeakSource noMatchScene;
    OBSWeakSource defaultTransition;
    //QHash<QPair<OBSWeakSource, OBSWeakSource>, OBSWeakSource>
    //    transitions;
};

uint qHash(const OBSWeakSource &ws);
typedef QSet<OBSWeakSource> PmScenes;

void pmRegisterMetaTypes();
