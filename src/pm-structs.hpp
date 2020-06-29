#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
#include <unordered_map>
//#include <unordered_set>
#include <QSet>
//#include <QHash>

#include <obs.h>
#include <obs.hpp>

#include "pm-filter.h"

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
    uint32_t matchImgWidth = 0, matchImgHeight = 0;
    uint32_t numCompared = 0;
    uint32_t numMatched = 0;
    float percentageMatched = 0;
    bool isMatched = false;
    uint32_t baseWidth = 0, baseHeight = 0;
};

typedef std::vector<PmMatchResults> PmMultiMatchResults;

struct PmMatchConfig
{
    PmMatchConfig();
    PmMatchConfig(obs_data_t* data);
    obs_data_t* save() const;

    std::string label;
    std::string matchImgFilename;

    #if 0
    int roiLeft = 0, roiBottom = 0;
    float perPixelErrThresh = 25.f;
    float totalMatchThresh = 90.f;
    PmMaskMode maskMode = PmMaskMode::GreenMode;
    uint32_t customColor = 0xffffff00;
    #endif

    struct pm_match_entry_config filterCfg;
    float totalMatchThresh;

    PmMaskMode maskMode = PmMaskMode::GreenMode;
    PmPreviewMode previewMode = PmPreviewMode::Video;
    float previewVideoScale = 0.5f;
    float previewRegionScale = 1.f;
    float previewMatchImageScale = 1.f;

    //OBSWeakSource matchScene;
    //OBSWeakSource transition;
    std::string matchScene;
    std::string matchTransition;

    bool operator==(const PmMatchConfig&) const;
    bool operator!=(const PmMatchConfig& other) const
        { return !operator==(other); }
};

class PmMultiMatchConfig : public std::vector<PmMatchConfig>
{
public:
    PmMultiMatchConfig() {}
    PmMultiMatchConfig(obs_data_t* data);
    obs_data_t* save(const std::string& presetName);

    bool operator==(const PmMultiMatchConfig&) const;
    bool operator!=(const PmMultiMatchConfig& other) const
        { return !operator==(other); }

    std::string noMatchScene;
    std::string noMatchTransition;
};

typedef std::unordered_map<std::string, PmMultiMatchConfig> PmMatchPresets;

uint qHash(const OBSWeakSource &ws);
class PmScenes : public QSet<OBSWeakSource>
{
public:
    QSet<QString> sceneNames() const;
};
//typedef std::unordered_set<std::string> PmScenes;

void pmRegisterMetaTypes();

#ifdef _MSC_VER
    #define PM_LITTLE_ENDIAN (REG_DWORD == REG_DWORD_LITTLE_ENDIAN)
#else
    #define PM_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif

#if 0
struct PmSwitchConfig
{
    PmSwitchConfig() {}
    PmSwitchConfig(obs_data_t* data);
    obs_data_t* save() const;

    bool isEnabled = false;
    OBSWeakSource matchScene;
    OBSWeakSource noMatchScene;
    OBSWeakSource defaultTransition;
};
#endif