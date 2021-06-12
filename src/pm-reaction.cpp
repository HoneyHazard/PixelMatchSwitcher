#include "pm-reaction.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>

const char *PmAction::actionStr(PmActionType actionType)
{
    switch (actionType) {
    case PmActionType::None: return obs_module_text("none");
    case PmActionType::Scene: return obs_module_text("scene");
    case PmActionType::SceneItem: return obs_module_text("scene item");
    case PmActionType::Filter: return obs_module_text("filter");
    case PmActionType::Hotkey: return obs_module_text("hotkey");
    case PmActionType::FrontEndEvent: return obs_module_text("front end event");
    default: return obs_module_text("unknown");
    }
}

QColor PmAction::actionColor(PmActionType actionType)
{
	switch (actionType) {
	case PmActionType::None: return Qt::white; 
	case PmActionType::Scene: return Qt::cyan;
	case PmActionType::SceneItem: return Qt::yellow;
	case PmActionType::Filter: return QColor(255, 0, 255, 255);
	case PmActionType::Hotkey: return Qt::red;
	case PmActionType::FrontEndEvent: return Qt::green;
	default: return Qt::white;
    }
}

QColor PmAction::dimmedColor(PmActionType actionType, PmActionType active)
{
	QColor color = actionColor(actionType);
	if (active != actionType) {
		color.setRed(color.red() / 2);
		color.setGreen(color.green() / 2);
		color.setBlue(color.blue() / 2);
	}
	return color;
}

QString PmAction::actionColorStr(PmActionType actionType)
{
	return actionColor(actionType).name();
}


PmAction::PmAction(obs_data_t *data)
{
    obs_data_set_default_int(data, "action_type", (long long)m_actionType);
    m_actionType = (PmActionType)obs_data_get_int(data, "action_type");

    obs_data_set_default_int(data, "action_code", (long long)m_actionCode);
    m_actionCode = (size_t)obs_data_get_int(data, "action_code");

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
                m_actionType = (PmActionType)(elementText.toInt());
            } else if (name == "action_code") {
                m_actionCode = (size_t)elementText.toUInt();
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
    obs_data_set_int(ret, "action_type", (long long)m_actionType);
    obs_data_set_int(ret, "action_code", (long long)m_actionCode);
    obs_data_set_string(ret, "target_element", m_targetElement.data());
    obs_data_set_string(ret, "target_details", m_targetDetails.data());
    return ret;
}

void PmAction::saveXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("action");
    writer.writeTextElement("action_type", QString::number(size_t(m_actionType)));
    writer.writeTextElement("action_code", QString::number(size_t(m_actionCode)));
    writer.writeTextElement("target_element", m_targetElement.data());
    writer.writeTextElement("target_details", m_targetDetails.data());
    writer.writeEndElement();
}

bool PmAction::renameElement(PmActionType actionType,
    const std::string &oldName, const std::string &newName)
{
    if (m_actionType == actionType && m_targetElement == oldName) {
        m_targetElement = newName;
        return true;
    }
    return false;
}

bool PmAction::isSet() const
{
    // TODO: revisit
    switch (m_actionType) {
    case PmActionType::None:
        return false; break;
    case PmActionType::Hotkey:
	    return m_actionCode != (size_t)-1; break;
    default:
        return m_targetElement.size() > 0; break;
    }
}

bool PmAction::operator==(const PmAction &other) const
{
    return m_actionType == other.m_actionType
        && m_actionCode == other.m_actionCode
        && m_targetElement == other.m_targetElement
        && m_targetDetails == other.m_targetDetails;
}

QString PmAction::actionColorStr() const
{
	return actionColorStr(m_actionType);
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

bool PmReaction::renameElement(PmActionType actionType,
    const std::string &oldName, const std::string &newName)
{
    bool ret = false;
    for (PmAction &action : matchActions) {
        ret |= action.renameElement(actionType, oldName, newName); 
    }
    for (PmAction &action : matchActions) {
        ret |= action.renameElement(actionType, oldName, newName);
    }
    return ret;
}

bool PmReaction::hasAction(PmActionType actionType) const
{
    for (const PmAction& action : matchActions) {
        if (actionType == PmActionType::ANY
         || action.m_actionType == actionType) {
            return true;
        }
    }
    for (const PmAction &action : matchActions) {
        if (actionType == PmActionType::ANY
         || action.m_actionType == actionType) {
            return true;
        }
    }
    return nullptr;
}

bool PmReaction::hasMatchAction(PmActionType actionType) const
{
    for (const PmAction& action : matchActions) {
		if (actionType == PmActionType::ANY
         || action.m_actionType == actionType) {
            return true;
        }
    }
    return nullptr;
}

bool PmReaction::hasUnmatchAction(PmActionType actionType) const
{
    for (const PmAction& action : unmatchActions) {
        if (actionType == PmActionType::ANY
         || action.m_actionType == actionType) {
            return true;
        }
    }
    return nullptr;
}


#if 0
const char *PmReaction::targetScene() const
{
    for (const PmAction& action : matchActions) {
        const char *ts = action.targetScene();
        if (ts)
            return ts;
    }
    for (const PmAction &action : unmatchActions) {
        const char *ts = action.targetScene();
        if (ts)
            return ts;
    }
    return nullptr;
}
#endif

bool PmReaction::isSet() const
{
    for (const auto &action : matchActions) {
        if (action.isSet()) {
            return true;
        }
    }
    for (const auto &action : unmatchActions) {
        if (action.isSet()) {
            return true;
        }
    }
    return false;
}

bool PmReaction::operator==(const PmReaction &other) const
{
    return lingerMs == other.lingerMs
        && cooldownMs == other.cooldownMs
        && matchActions == other.matchActions
        && unmatchActions == other.unmatchActions;
}

void PmReaction::getMatchScene(
    std::string &sceneName, std::string &transition) const
{
    for (const PmAction& action : matchActions) {
        if (action.m_actionType == PmActionType::Scene) {
            sceneName = action.m_targetElement;
            transition = action.m_targetDetails;
            return;
        }
    }
}

void PmReaction::getUnmatchScene(
    std::string &sceneName, std::string &transition) const
{
    for (const PmAction& action : unmatchActions) {
        if (action.m_actionType == PmActionType::Scene) {
            sceneName = action.m_targetElement;
            transition = action.m_targetDetails;
            return;
        }
    }
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
