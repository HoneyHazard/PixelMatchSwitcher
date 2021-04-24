#pragma once

#include <stdint.h>
#include <utility>
#include <qmetatype.h>
#include <QHash>
#include <QSet>
#include <QList>

#include <obs.h>
#include <obs.hpp>

#include "pm-filter.h"
#include "pm-reaction.hpp"

class QXmlStreamWriter;
class QXmlStreamReader;

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
 * @brief How things should be handled when there are duplicate names
 */
enum class PmDuplicateNameReaction : char {
    Undefined = 0,
    Rename = 1,
    Replace = 2,
    Skip = 3,
    Abort = 4
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
 * @brief Contains results for several match entries
 */
typedef std::vector<PmMatchResults> PmMultiMatchResults;

/**
 * @brief Types of reactions provided by the plugin
 */
enum class PmReactionType : char {
    SwitchScene = 0,
    ShowSceneItem = 1,
    HideSceneItem = 2,
    ShowFilter = 3,
    HideFilter = 4,
    ShowImage = 5, /* TBD */
    HideImage = 6  /* TBD */
};

/**
 * @brief Describes what will happen in reaction to a condition
 */
struct PmReactionOld
{
public:
    PmReactionOld() {}
    PmReactionOld(obs_data_t *data);
    PmReactionOld(QXmlStreamReader &reader);

    obs_data_t *save() const;
    void saveXml(QXmlStreamWriter &writer) const;

    bool isSet() const;

public:
    PmReactionType type = PmReactionType::SwitchScene;

    std::string targetScene;
    std::string sceneTransition = "Cut";

    std::string targetSceneItem;
    std::string targetFilter;

    uint32_t lingerMs = 0;

    bool operator==(const PmReactionOld &) const;
    bool operator!=(const PmReactionOld &other) const
        { return !operator==(other); }
};

/**
 * @brief Describes matching configuration of an individual match entry, 
 *        as well as switching behavior in case of a match
 */
struct PmMatchConfig
{
    PmMatchConfig();
    PmMatchConfig(obs_data_t* data);
    PmMatchConfig(QXmlStreamReader &reader);

    obs_data_t* save() const;
    void saveXml(QXmlStreamWriter &writer) const;

    std::string label = obs_module_text("new config");
    std::string matchImgFilename;
    bool wasDownloaded = false;

    struct pm_match_entry_config filterCfg;
    float totalMatchThresh = 90.f;
    bool invertResult = false;

    PmMaskMode maskMode = PmMaskMode::AlphaMode;

    //PmReactionOld reactionOld;
    PmReaction reaction;

    bool operator==(const PmMatchConfig&) const;
    bool operator!=(const PmMatchConfig& other) const
        { return !operator==(other); }
};

/**
 * @brief Describes matching and switching configuration of several match 
 *        entries, as well as switching behavior for when there is no match
 */
class PmMultiMatchConfig final : public std::vector<PmMatchConfig>
{
public:
    static const std::string k_defaultNoMatchTransition;

    PmMultiMatchConfig() {}
    PmMultiMatchConfig(obs_data_t* data);
    PmMultiMatchConfig(QXmlStreamReader &reader, std::string &presetName);

    obs_data_t* save(const std::string& presetName);
    void saveXml(QXmlStreamWriter &writer, const std::string &presetName) const;

    bool operator==(const PmMultiMatchConfig&) const;
    bool operator!=(const PmMultiMatchConfig& other) const
        { return !operator==(other); }

    bool containsImage(const std::string &imgFilename,
                       size_t exceptIndex = (size_t)-1) const;

    PmReactionOld noMatchReactionOld;
    PmReaction noMatchReaction;
};

/**
 * @bries Stores multiple PmMultiMatchConfig that are easily referenced by a
 *        preset name
 */
class PmMatchPresets final : public QHash<std::string, PmMultiMatchConfig>
{
public:
    PmMatchPresets() {};
    PmMatchPresets(const std::string &filename);
    PmMatchPresets(const QByteArray &data);

    void exportXml(const std::string &filename,
                   const QList<std::string> &selectedPresets) const;

    bool containsImage(const std::string &imgFilename) const;

    QSet<std::string> orphanedImages(
        const PmMultiMatchConfig &beingRemoved,
        const PmMultiMatchConfig* activeCfg = nullptr);

protected:
    void importXml(QXmlStreamReader &reader);
};

/*
 * @brief Represents a set of weak source references to available scenes or 
 */
class PmSourceHash final : public QHash<std::string, OBSWeakSource>
{
public:
    PmSourceHash() {}
    PmSourceHash(const PmSourceHash &other)
        : QHash<std::string, OBSWeakSource>(other) {}

    QSet<std::string> sourceNames() const { return keys().toSet(); };
};

/**
 * @brief A reference to a scene item and a list of its filter names
 */
struct PmSceneItemData
{
    bool operator==(const PmSceneItemData &other) const;

    OBSSceneItem si;
    QList<std::string> filtersNames;
};

/*
 * @brief Represents a set 1f scene item references to available scene items
 */
class PmSceneItemsHash final : public QHash<std::string, PmSceneItemData> {
public:
    PmSceneItemsHash() {}
    PmSceneItemsHash(const PmSceneItemsHash &other)
        : QHash<std::string, PmSceneItemData>(other) {}

    QSet<std::string> sceneItemNames() const { return keys().toSet(); };
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
    const QString k_xmlFilenameFilter
        = "XML (*.xml);; All files (*.*)";

    const QString k_problemCharacterStr
        = QString("[") + QRegExp::escape("\\/:*?\"<>|\"") + QString("]");
    const QRegExp k_problemCharacterRegex(PmConstants::k_problemCharacterStr);
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
