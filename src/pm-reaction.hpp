#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <obs-data.h>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QColor>

/**
 * @brief Types of reactions provided by the plugin
 */
enum class PmActionType : char {
    None = 0,
    Scene = 1,
    SceneItem = 2,
    Filter = 3,
    Hotkey = 4,
    FrontEndEvent = 5
};

/**
 * @brief Scene items and filters can be shown or hidden
 */
enum class PmToggleCode : int { Show = 0, Hide = 1 };

/**
 * @brief Match entry actions vs global match/unmatch
 */
enum class PmReactionTarget { Entry, Anything };

enum class PmReactionType { Match, Unmatch };

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

    PmActionType m_actionType = PmActionType::Scene;
    int m_actionCode = 0;
    std::string m_targetElement;
    std::string m_targetDetails;
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
