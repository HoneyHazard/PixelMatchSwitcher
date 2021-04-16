#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <obs-data.h>
#include <QXmlStreamReader>

class PmAction
{
public:
    enum class ActionType : unsigned char {
        None=0, Scene=1, SceneItem=2, Filter=3, Hotkey=4, FrontEndEvent=5 };

    enum class ToggleCode : int { Show=0, Hide=1 };

    PmAction() {}
    PmAction(obs_data_t *data);
    PmAction(QXmlStreamReader &reader);

    void execute();

    obs_data_t *saveData() const;
    void saveXml(QXmlStreamWriter &writer);

    bool operator==(const PmAction &) const;
    bool operator!=(const PmAction &other) const
        { return !operator==(other); }

protected:
    void switchScene();
    void toggleSceneItem();
    void toggleFilter();
    void triggerHotkey();
    void triggerFrontEndEvent();

    ActionType m_actionType = ActionType::None;
    int m_targetCode = 0;
    std::string m_targetElement;
    std::string m_targetDetails;
};

struct PmReaction
{
    PmReaction() {}
    PmReaction(obs_data_t *data);
    PmReaction(QXmlStreamReader &reader);

    obs_data_t *saveData() const;
    void saveXml(QXmlStreamWriter &writer);

    uint32_t lingerMs;
    uint32_t cooldownMs;

    std::vector<PmAction> matchActions;
    std::vector<PmAction> unmatchActions;
};
