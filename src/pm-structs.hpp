#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
//#include <unordered_map>
//#include <unordered_set>
#include <QMap>
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

enum class PmCaptureState : unsigned char {
    Inactive=0,
    Activated=1,
    SelectBegin=2,
    SelectEnd=3
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

    std::string label = obs_module_text("new config");
    std::string matchImgFilename;

    struct pm_match_entry_config filterCfg;
    float totalMatchThresh = 90.f;

    PmMaskMode maskMode = PmMaskMode::GreenMode;

    //OBSWeakSource matchScene;
    //OBSWeakSource transition;
    std::string matchScene;
    std::string matchTransition = "Cut";

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
    std::string noMatchTransition = "Cut";
};

typedef QMap<std::string, PmMultiMatchConfig> PmMatchPresets;

class PmScenes : public QMap<OBSWeakSource, std::string>
{
public:
    PmScenes() {}
    PmScenes(const PmScenes& other);

    QSet<std::string> sceneNames() const;
};

struct PmPreviewConfig
{
    PmPreviewConfig() {};
    PmPreviewConfig(obs_data_t* data);
    obs_data_t* save() const;

    bool operator==(const PmPreviewConfig&) const;
    bool operator!=(const PmPreviewConfig& other) const
        { return !operator==(other); }

    PmPreviewMode previewMode = PmPreviewMode::Video;
    float previewVideoScale = 0.5f;
    float previewRegionScale = 0.5f;
    float previewMatchImageScale = 0.5f;
};


void pmRegisterMetaTypes();

uint qHash(const OBSWeakSource& ws);
uint qHash(const std::string& str);

#ifdef _MSC_VER
    #define PM_LITTLE_ENDIAN (REG_DWORD == REG_DWORD_LITTLE_ENDIAN)
#else
    #define PM_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif
