#include "pm-reaction.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QDateTime>

const std::string PmAction::k_timeMarker = "[time]";
const std::string PmAction::k_labelMarker = "[label]";
const std::string PmAction::k_defaultFileTimeFormat = "dd/MM/yy hh:mm:ss";

const char *PmAction::actionStr(PmActionType actionType)
{
    switch (actionType) {
    case PmActionType::None: return obs_module_text("none");
    case PmActionType::Scene: return obs_module_text("scene");
    case PmActionType::SceneItem: return obs_module_text("scene item");
    case PmActionType::Filter: return obs_module_text("filter");
    case PmActionType::Hotkey: return obs_module_text("hotkey");
    case PmActionType::FrontEndAction: return obs_module_text("frontend event");
    case PmActionType::ToggleMute: return obs_module_text("toggle mute");
    case PmActionType::File: return obs_module_text("file"); break;
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
    case PmActionType::FrontEndAction: return Qt::green;
    case PmActionType::ToggleMute: return QColor(255, 127, 0, 255); break;
    case PmActionType::File: return Qt::white; break;
    default : return Qt::lightGray;
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

QString PmAction::frontEndActionStr(PmFrontEndAction fea)
{
    switch (fea) {
    case PmFrontEndAction::StreamingStart:
        return obs_module_text("Streaming: Start");
    case PmFrontEndAction::StreamingStop:
        return obs_module_text("Streaming: Stop");
    case PmFrontEndAction::RecordingStart:
        return obs_module_text("Recording: Start");
    case PmFrontEndAction::RecordingStop:
        return obs_module_text("Recording: Stop");
    case PmFrontEndAction::RecordingPause:
        return obs_module_text("Recording: Pause");
    case PmFrontEndAction::RecordingUnpause:
        return obs_module_text("Recording: Unpause");
    case PmFrontEndAction::ReplayBufferStart:
        return obs_module_text("Replay Buffer: Start");
    case PmFrontEndAction::ReplayBufferSave:
        return obs_module_text("Replay Buffer: Save");
    case PmFrontEndAction::ReplayBufferStop:
        return obs_module_text("Replay Buffer: Stop");
    case PmFrontEndAction::TakeScreenshot:
        return obs_module_text("Take Screenshot");
    case PmFrontEndAction::StartVirtualCam:
        return obs_module_text("Virtual Cam: Start");
    case PmFrontEndAction::StopVirtualCam:
        return obs_module_text("Virtual Cam: Stop");
    case PmFrontEndAction::ResetVideo:
        return obs_module_text("Reset Video");
    default:
        return obs_module_text("unknown");
    }
}

QString PmAction::fileActionStr(PmFileActionType fa)
{
    switch (fa) {
    case PmFileActionType::WriteAppend:
        return obs_module_text("append");
    case PmFileActionType::WriteTruncate:
        return obs_module_text("truncate");
    }
}

PmAction::PmAction(obs_data_t *data)
{
    obs_data_set_default_int(data, "action_type", (long long)actionType);
    actionType = (PmActionType)obs_data_get_int(data, "action_type");

    obs_data_set_default_int(data, "action_code", (long long)actionCode);
    actionCode = (size_t)obs_data_get_int(data, "action_code");

    targetElement = obs_data_get_string(data, "target_element");
    targetDetails = obs_data_get_string(data, "target_details");

    if (actionType == PmActionType::Hotkey) {
        obs_data_set_default_int(data, "hotkey_key", 0L);
        obs_data_set_default_int(data, "hotkey_modifiers", 0L);
        keyCombo.key = (obs_key_t)obs_data_get_int(data, "hotkey_key");
        keyCombo.modifiers
            = (uint32_t)obs_data_get_int(data, "hotkey_modifiers");
    }
    if (actionType == PmActionType::File) {
        timeFormat = obs_data_get_string(data, "date_time_format");
    }

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
                actionType = (PmActionType)(elementText.toInt());
            } else if (name == "action_code") {
                actionCode = (size_t)elementText.toUInt();
            } else if (name == "target_element") {
                targetElement = elementText.toUtf8().data();
            } else if (name == "target_details") {
                targetDetails = elementText.toUtf8().data();
            } else if (name == "hotkey_key") {
                keyCombo.key = (obs_key_t)elementText.toInt();
            } else if (name == "hotkey_modifiers") {
                keyCombo.modifiers = (uint32_t)elementText.toUInt();
            } else if (name == "date_time_format") {
                timeFormat = elementText.toUtf8().data();
            }
        }
    }
}

obs_data_t *PmAction::saveData() const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_int(ret, "action_type", (long long)actionType);
    obs_data_set_int(ret, "action_code", (long long)actionCode);
    if (targetElement.size())
        obs_data_set_string(ret, "target_element", targetElement.data());
    if (targetDetails.size())
        obs_data_set_string(ret, "target_details", targetDetails.data());
    if (actionType == PmActionType::Hotkey) {
        obs_data_set_int(
            ret, "hotkey_modifiers", (long long)keyCombo.modifiers);
        obs_data_set_int(ret, "hotkey_key", (long long)keyCombo.key);
    }
    if (actionType == PmActionType::File) {
        if (timeFormat.size()) {
            obs_data_set_string(ret, "date_time_format", timeFormat.data());
        }
    }
    return ret;
}

void PmAction::saveXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("action");
    writer.writeTextElement("action_type", QString::number(size_t(actionType)));
    writer.writeTextElement("action_code", QString::number(size_t(actionCode)));
    if (targetElement.size())
        writer.writeTextElement("target_element", targetElement.data());
    if (targetDetails.size())
        writer.writeTextElement("target_details", targetDetails.data());
    if (actionType == PmActionType::Hotkey) {
        writer.writeTextElement(
            "hotkey_key", QString::number(int(keyCombo.key)));
        writer.writeTextElement(
            "hotkey_modifiers", QString::number(keyCombo.modifiers));
    }
    if (actionType == PmActionType::File) {
        if (timeFormat.size()) {
            writer.writeTextElement("date_time_format", timeFormat.data());
        }
    }
    writer.writeEndElement();
}

bool PmAction::renameElement(PmActionType actionType,
    const std::string &oldName, const std::string &newName)
{
    if (actionType == actionType && targetElement == oldName) {
        targetElement = newName;
        return true;
    }
    return false;
}

bool PmAction::isSet() const
{
    switch (actionType) {
    case PmActionType::None:
        return false; break;
    case PmActionType::FrontEndAction:
        return actionCode != (size_t)-1; break;
    case PmActionType::Hotkey:
        return keyCombo.key != (obs_key_t)-1
            || keyCombo.modifiers != (obs_key_t)-1;
    case PmActionType::File:
        return targetElement.size() > 0 || targetDetails.size() > 0;
    default:
        return targetElement.size() > 0; break;
    }
}

bool PmAction::operator==(const PmAction &other) const
{
    return actionType == other.actionType
        && actionCode == other.actionCode
        && targetElement == other.targetElement
        && targetDetails == other.targetDetails
        && (actionType != PmActionType::Hotkey ||
            (keyCombo.key == other.keyCombo.key
                && keyCombo.modifiers == other.keyCombo.modifiers))
        && (actionType != PmActionType::File ||
            timeFormat == other.timeFormat);
}

QString PmAction::actionColorStr() const
{
    return actionColorStr(actionType);
}

std::string PmAction::formattedFileString(const std::string &str,
    const std::string &cfgLabel, const QDateTime &time) const
{
    // TODO optimize but not super important
    std::string timeStr = time.toString(timeFormat.data()).toUtf8();
    std::string ret = str;
    size_t find;
    while ((find = ret.find(k_timeMarker)) != std::string::npos) {
        ret.replace(find, k_timeMarker.size(), timeStr);
    }
    while ((find = ret.find(k_labelMarker)) != std::string::npos) {
        ret.replace(find, k_labelMarker.size(), cfgLabel);
    }
    return ret;
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
    for (PmAction &action : unmatchActions) {
        ret |= action.renameElement(actionType, oldName, newName);
    }
    return ret;
}

bool PmReaction::hasAction(PmActionType actionType) const
{
    for (const PmAction& action : matchActions) {
        if (actionType == PmActionType::ANY
         || action.actionType == actionType) {
            return true;
        }
    }
    for (const PmAction &action : unmatchActions) {
        if (actionType == PmActionType::ANY
         || action.actionType == actionType) {
            return true;
        }
    }
    return nullptr;
}

bool PmReaction::hasMatchAction(PmActionType actionType) const
{
    for (const PmAction& action : matchActions) {
        if (actionType == PmActionType::ANY
         || action.actionType == actionType) {
            return true;
        }
    }
    return nullptr;
}

bool PmReaction::hasUnmatchAction(PmActionType actionType) const
{
    for (const PmAction& action : unmatchActions) {
        if (actionType == PmActionType::ANY
         || action.actionType == actionType) {
            return true;
        }
    }
    return nullptr;
}

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
        if (action.actionType == PmActionType::Scene) {
            sceneName = action.targetElement;
            transition = action.targetDetails;
            return;
        }
    }
}

void PmReaction::getUnmatchScene(
    std::string &sceneName, std::string &transition) const
{
    for (const PmAction& action : unmatchActions) {
        if (action.actionType == PmActionType::Scene) {
            sceneName = action.targetElement;
            transition = action.targetDetails;
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

