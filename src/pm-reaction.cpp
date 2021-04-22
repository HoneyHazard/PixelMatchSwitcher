#include "pm-reaction.hpp"
#include <obs-frontend-api.h>

PmAction::PmAction(obs_data_t *data)
{
    obs_data_set_default_int(data, "action_type", (long long)actionType);
    actionType = (ActionType)obs_data_get_int(data, "action_type");

    obs_data_set_default_int(data, "action_code", m_actionCode);
    m_actionCode = (int)obs_data_get_int(data, "action_code");

    m_targetElement = obs_data_get_string(data, "target_element");
    m_targetDetails = obs_data_get_string(data, "target_details");

    obs_data_release(data);
}

PmAction::PmAction(QXmlStreamReader &reader)
{
    while (true) {
        reader.readNext();
        if (reader.atEnd() || reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QStringRef name = reader.name();
        if (reader.isEndElement()) {
            if (name == "action") {
                return;
            }
        } else if (reader.isStartElement()) {
            QString elementText = reader.readElementText();
            if (name == "action_type") {
                actionType = (ActionType)(elementText.toInt());
            } else if (name == "action_code") {
                m_actionCode = int(elementText.toInt());
            } else if (name == "target_element") {
                m_targetElement = elementText.toUtf8().data();
            } else if (name == "target_details") {
                m_targetDetails = elementText.toUtf8().data();
            }
        }
    }
}

obs_data_t *PmAction::saveData() const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_int(ret, "action_type", (long long)actionType);
    obs_data_set_int(ret, "action_code", (long long)m_actionCode);
    obs_data_set_string(ret, "target_element", m_targetElement.data());
    obs_data_set_string(ret, "target_details", m_targetDetails.data());
    return ret;
}

void PmAction::saveXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("action");
    writer.writeTextElement("action_type", QString::number(int(actionType)));
    writer.writeTextElement("action_code", QString::number(int(m_actionCode)));
    writer.writeTextElement("target_element", m_targetElement.data());
    writer.writeTextElement("target_details", m_targetDetails.data());
    writer.writeEndElement();
}

/*
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
*/

bool PmAction::isSet() const
{
    // TODO: revisit
    switch (actionType) {
    case ActionType::None:
        return false; break;
    default:
        return m_targetElement.size() > 0; break;
    }
}

bool PmAction::operator==(const PmAction &other) const
{
    return actionType == other.actionType
        && m_actionCode == other.m_actionCode
        && m_targetElement == other.m_targetElement
        && m_targetDetails == other.m_targetDetails;
}


//---------------------------------------------------------

PmReaction::PmReaction(obs_data_t *data)
{
    obs_data_set_default_int(data, "linger_ms", (long long)lingerMs);
    lingerMs = (uint32_t)(obs_data_get_int(data, "linger_ms"));

    obs_data_set_default_int(data, "cooldown_ms", (long long)cooldownMs);
    cooldownMs = (uint32_t)(obs_data_get_int(data, "cooldown_ms"));

    obs_data_array_t* matchArray = obs_data_get_array(data, "match_actions");
    if (matchArray) {
        obs_data_array_enum(matchArray, readActionArray, &matchActions);
        obs_data_array_release(matchArray);
    }

    obs_data_array_t *unmatchArray =
        obs_data_get_array(data, "unmatch_actions");
    if (unmatchArray) {
        obs_data_array_enum(unmatchArray, readActionArray, &unmatchActions);
        obs_data_array_release(unmatchArray);
    }

    obs_data_release(data);
}

PmReaction::PmReaction(QXmlStreamReader &reader)
{
    while (true) {
        reader.readNext();
        if (reader.atEnd() ||
            reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QStringRef name = reader.name();
        if (reader.isEndElement()) {
            if (name == "reaction") {
                return;
            }
        } else if (reader.isStartElement()) {
            if (name == "match_actions") {
                readActionsXml(reader, "match_actions", matchActions);
            } else if (name == "unmatch_actions") {
                readActionsXml(reader, "unmatch_actions", unmatchActions);
            } else {
                QString elemText = reader.readElementText();
                if (name == "linger_ms") {
                    lingerMs = (uint32_t)elemText.toInt();
                } else if (name == "cooldown_ms") {
                    cooldownMs = (uint32_t)elemText.toInt();
                }
            }
        }
    }
}

obs_data_t *PmReaction::saveData() const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_int(ret, "linger_ms", (long long)lingerMs);
    obs_data_set_int(ret, "cooldown_ms", (long long)cooldownMs);
    if (matchActions.size() > 0) {
        obs_data_array_t *matchArray = writeActionArray(matchActions);
        obs_data_set_array(ret, "match_actions", matchArray);
        obs_data_array_release(matchArray);
    }
    if (unmatchActions.size() > 0) {
        obs_data_array_t *unmatchArray = writeActionArray(unmatchActions);
        obs_data_set_array(ret, "unmatch_actions", unmatchArray);
        obs_data_array_release(unmatchArray);
    }
    return ret;
}

void PmReaction::saveXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("reaction");
    writer.writeTextElement("linger_ms", QString::number(lingerMs));
    writer.writeTextElement("cooldown_ms", QString::number(cooldownMs));
    writeActionsXml(writer, "match_actions", matchActions);
    writeActionsXml(writer, "unmatch_actions", unmatchActions);
    writer.writeEndElement();
}

void PmReaction::readActionArray(obs_data_t *aData, void *param)
{
    auto *actions = static_cast< std::vector<PmAction>* >(param);
    PmAction action(aData);
    actions->push_back(action);
    obs_data_release(aData);
}

obs_data_array_t *PmReaction::writeActionArray(const std::vector<PmAction> &vec)
{
    obs_data_array_t *ret = obs_data_array_create();
    for (const auto &action : vec) {
        obs_data_t *aData = action.saveData();
        obs_data_array_push_back(ret, aData);
        obs_data_release(aData);
    }
    return ret;
}

void PmReaction::readActionsXml(QXmlStreamReader &reader,
    const std::string &vecName, std::vector<PmAction> &vec)
{
    while (true) {
        reader.readNext();
        if (reader.atEnd() ||
            reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QStringRef name = reader.name();
        if (reader.isEndElement()) {
            if (name == vecName.data()) {
                return;
            }
        } else if (reader.isStartElement() && name == "action") {
            PmAction action(reader);
            vec.push_back(action);
        }
    }
}

void PmReaction::writeActionsXml(QXmlStreamWriter &writer,
    const std::string &vecName, const std::vector<PmAction> &vec)
{
    if (vec.empty()) return;

    writer.writeStartElement(vecName.data());
    for (const auto &action : vec) {
        action.saveXml(writer);
    }
    writer.writeEndElement();
}
