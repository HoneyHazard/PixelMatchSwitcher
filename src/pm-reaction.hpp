#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <obs-data.h>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QColor>

#include <obs-module.h>
#include <obs-hotkey.h>

/**
 * @brief Types of reactions provided by the plugin
 */
enum class PmActionType : char {
    None = 0,
    Scene = 1,
    SceneItem = 2,
    Filter = 3,
    Hotkey = 4,
    FrontEndAction = 5,
    ToggleMute = 6,
    ANY = 7
};

/**
 * @brief Scene items and filters can be shown or hidden
 */
enum class PmToggleCode : size_t { On = 0, Off = 1 };

/**
 * @brief Match entry actions vs global match/unmatch
 */
enum class PmReactionTarget { Entry, Global }; 

enum class PmReactionType { Match, Unmatch };

enum class PmFrontEndAction : size_t {
    StreamingStart = 0, StreamingStop = 1,
    RecordingStart = 2, RecordingStop = 3,
	RecordingPause = 4, RecordingUnpause = 5,
    ReplayBufferStart = 6, ReplayBufferSave = 7, ReplayBufferStop = 8,
    TakeScreenshot,
    StartVirtualCam, StopVirtualCam,
    ResetVideo
};

/**
 * @brief Individual unit of action that is part of a reaction
 */
struct PmAction
{
public:
    static const char *actionStr(PmActionType actionType);
	static QColor actionColor(PmActionType actionType);
    static QColor dimmedColor(PmActionType actionType,
                              PmActionType active = PmActionType::None);
	static QString actionColorStr(PmActionType actionType);
    static QString frontEndActionStr(PmFrontEndAction fea);

    PmAction() {}
    PmAction(obs_data_t *data);
    PmAction(QXmlStreamReader &reader);

    obs_data_t *saveData() const;
    void saveXml(QXmlStreamWriter &writer) const;
    bool renameElement(PmActionType actionType,
        const std::string &oldName, const std::string &newName);

    bool isSet() const;
    bool operator==(const PmAction &) const;
    bool operator!=(const PmAction &other) const { return !operator==(other); }
    QString actionColorStr() const;

    PmActionType actionType = PmActionType::None;
    std::string targetElement;
    std::string targetDetails;

    size_t actionCode = (size_t)-1;

    obs_key_combination_t keyCombo = {(uint32_t)-1, (obs_key_t)-1};
};

/**
 * @brief Describes what should happen when an entry is matched or unmatched
 */
struct PmReaction
{
    PmReaction() {}
    PmReaction(obs_data_t *data);
    PmReaction(QXmlStreamReader &reader);

    obs_data_t *saveData() const;
    void saveXml(QXmlStreamWriter &writer) const;
    bool renameElement(PmActionType actionType,
        const std::string &oldName, const std::string &newName);

    bool hasAction(PmActionType actionType) const;
    bool hasMatchAction(PmActionType actionType) const;
    bool hasUnmatchAction(PmActionType actionType) const;
    bool hasSceneAction() const { return hasAction(PmActionType::Scene); }
    void getMatchScene(std::string &sceneName, std::string &transition) const;
    void getUnmatchScene(std::string &scenename, std::string &transition) const;

    bool isSet() const;
    bool operator==(const PmReaction &) const;
    bool operator!=(const PmReaction &other) const
        { return !operator==(other); }
    size_t matchSz() const { return matchActions.size(); }
    size_t unmatchSz() const { return unmatchActions.size(); }

    uint32_t lingerMs = 0;
    uint32_t cooldownMs = 0;

    std::vector<PmAction> matchActions;
    std::vector<PmAction> unmatchActions;

protected:
    static void readActionArray(obs_data_t *a, void *param);
    static obs_data_array_t *writeActionArray(const std::vector<PmAction> &vec);
    static void readActionsXml(QXmlStreamReader &reader,
        const std::string &vecName, std::vector<PmAction> &vec);
    static void writeActionsXml(QXmlStreamWriter &writer,
        const std::string &vecName, const std::vector<PmAction> &vec);
};

