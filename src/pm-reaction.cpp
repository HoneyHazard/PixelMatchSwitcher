#include "pm-reaction.hpp"

void PmAction::execute()
{
    switch (m_actionType) {
    case ActionType::None: break;
    case ActionType::Scene: switchScene(); break;
    case ActionType::SceneItem: toggleSceneItem(); break;
    case ActionType::Filter: toggleFilter(); break;
    case ActionType::Hotkey: triggerHotkey(); break;
    case ActionType::FrontEndEvent: triggerFrontEndEvent(); break;
    }
}
