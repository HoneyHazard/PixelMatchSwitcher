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

enum class PmToggleCode : int { Show = 0, Hide = 1 };

struct PmAction
{
public:
    static const char *actionStr(PmActionType actionType);
    static QColor actionColor(PmActionType color);

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

    PmActionType m_actionType = PmActionType::None;
    int m_actionCode = 0;
    std::string m_targetElement;
    std::string m_targetDetails;
};

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
    bool hasSceneAction() const { return hasAction(PmActionType::Scene); }
    bool isSet() const;
    bool operator==(const PmReaction &) const;
    bool operator!=(const PmReaction &other) const
        { return !operator==(other); }
    void getMatchScene(std::string &sceneName, std::string &transition);
    void getUnmatchScene(std::string &scenename, std::string &transition);

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
