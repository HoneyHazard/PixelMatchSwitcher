#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
#include <QHash>
#include <QSet>

#include <obs.h>
#include <obs.hpp>

#include "pm-filter.h"

/**
 * @brief Describes strategies for how regions of the match image will be masked 
 *        out based on color or alpha channels of the image.
 */
enum class PmMaskMode : int {
    AlphaMode=0,
    GreenMode=1,
    MagentaMode=2,
    BlackMode=3,
    CustomClrMode=4
};

/**
 * @brief Preview region of the dialog will have these preview modes
 */
enum class PmPreviewMode : int {
    Video=0,
    Region=1,
    MatchImage=2
};

/**
 * @brief Describes states of the state machine for capturing match images from
 *        video.
 */
enum class PmCaptureState : unsigned char {
    Inactive=0,       /**< No capture is active                               */
    Activated=1,      /**< Capture activated and waiting for user action      */
    SelectBegin=2,    /**< Mouse pressed to begin selection of a region       */
    SelectMoved=3,    /**< Mouse movign as the region is being selected       */
    SelectFinished=4, /**< Mouse released to end selection of a region        */
    Accepted=5,       /**< Image extracted and being reviewed by user         */
    Automask=6,       /**< Automask mode is active and dynamic portions of 
                           the image are becoming part of the generated mask  */
};

/**
 * @brief Information describing results of processing an indivisual match
 *        entry. Constructed from the filter's output by PmCore and sent to 
 *        other modules.
 */
struct PmMatchResults
{
    uint32_t matchImgWidth = 0, matchImgHeight = 0;
    uint32_t numCompared = 0;
    uint32_t numMatched = 0;
    float percentageMatched = 0;
    bool isMatched = false;
    uint32_t baseWidth = 0, baseHeight = 0;
};

/**
 * @briefs Contains results for several match entries
 */
typedef std::vector<PmMatchResults> PmMultiMatchResults;

/**
 * @brief Describes matching configuration of an individual match entry, 
 *        as well as switching behavior in case of a match
 */
struct PmMatchConfig
{
    PmMatchConfig();
    PmMatchConfig(obs_data_t* data);
    obs_data_t* save() const;

    std::string label = obs_module_text("new config");
    std::string matchImgFilename;

    struct pm_match_entry_config filterCfg;
    float totalMatchThresh = 90.f;

    PmMaskMode maskMode = PmMaskMode::AlphaMode;

    std::string matchScene;
    std::string matchTransition = "Cut";

    bool operator==(const PmMatchConfig&) const;
    bool operator!=(const PmMatchConfig& other) const
        { return !operator==(other); }
};

/**
 * @brief Describes matching and switching configuration of several match 
 *        entries, as well as switching behavior for when there is no match
 */
class PmMultiMatchConfig : public std::vector<PmMatchConfig>
{
public:
    static const std::string k_defaultNoMatchScene;
    static const std::string k_defaultNoMatchTransition;

    PmMultiMatchConfig() {}
    PmMultiMatchConfig(obs_data_t* data);
    obs_data_t* save(const std::string& presetName);

    bool operator==(const PmMultiMatchConfig&) const;
    bool operator!=(const PmMultiMatchConfig& other) const
        { return !operator==(other); }

    std::string noMatchScene = k_defaultNoMatchScene;
    std::string noMatchTransition = k_defaultNoMatchTransition;
};

/**
 * @bries Stores multiple PmMultiMatchConfig that are easily referenced by a
 *        preset name
 */
typedef QHash<std::string, PmMultiMatchConfig> PmMatchPresets;

/*
 * @brief Represents a set of weak source references to available scenes, that
 *        also allows matching each source to a scene name
 */
class PmScenes : public QHash<OBSWeakSource, std::string>
{
public:
    PmScenes() {}
    PmScenes(const PmScenes& other);

    QSet<std::string> sceneNames() const;
};

/**
 * @brief Configuration for the preview state of the dialog.
 * 
 * Note the class used to have more members in the past, and may have more in
 * the future. Hence this wrapper class is retained.
 */
struct PmPreviewConfig
{
    PmPreviewConfig() {}
    PmPreviewConfig(obs_data_t* data);
    obs_data_t* save() const;

    bool operator==(const PmPreviewConfig&) const;
    bool operator!=(const PmPreviewConfig& other) const
        { return !operator==(other); }

    PmPreviewMode previewMode = PmPreviewMode::Video;
};

/**
 * @brief A namespace where commonly used constants can be placed
 */
namespace PmConstants
{
    const QString k_imageFilenameFilter 
        = "PNG (*.png);; JPEG (*.jpg *.jpeg);; BMP (*.bmp);; All files (*.*)";
};

/**
 * @brief Allows our data structures to be propagated through signals and slots.
 */
void pmRegisterMetaTypes();

/** @brief Allow OBSWeakSource be a key index into QHash or QSet */
inline uint qHash(const OBSWeakSource& ws)
{
    obs_source_t* source = obs_weak_source_get_source(ws);
    uint ret = qHash(source);
    obs_source_release(source);
    return ret;
}

/** @brief Allow obs_weak_source_t* be a key index into QHash or QSet */
inline uint qHash(obs_weak_source_t* ws)
{
	obs_source_t *source = obs_weak_source_get_source(ws);
	uint ret = qHash(source);
	obs_source_release(source);
	return ret;
}

namespace std {
    /** @brief Allow std::string to be a key index into QHash or QSet */
    inline uint qHash(const std::string& str)
    {
        return (uint)std::hash<std::string>()(str);
    }
}

#ifdef _MSC_VER
    #define PM_LITTLE_ENDIAN (REG_DWORD == REG_DWORD_LITTLE_ENDIAN)
#else
    #define PM_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif
