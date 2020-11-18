#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

#include <QXmlStreamWriter>
#include <QFile>
#include <sstream>

bool operator== (const struct pm_match_entry_config& l, 
                 const struct pm_match_entry_config& r)
{
    return l.is_enabled == r.is_enabled
        && l.roi_left == r.roi_left
        && l.roi_bottom == r.roi_bottom
        && l.per_pixel_err_thresh == r.per_pixel_err_thresh
        && l.mask_alpha == r.mask_alpha
        && l.mask_color.x == r.mask_color.x
        && l.mask_color.y == r.mask_color.y
        && l.mask_color.z == r.mask_color.z;
}

PmMatchConfig::PmMatchConfig()
{
    memset(&filterCfg, 0, sizeof(filterCfg));
    filterCfg.is_enabled = true;
    filterCfg.per_pixel_err_thresh = 10.f;
    switch (maskMode) {
    case PmMaskMode::AlphaMode:
        filterCfg.mask_alpha = true;
        filterCfg.mask_color.x = 0.f;
        filterCfg.mask_color.y = 0.f;
        filterCfg.mask_color.z = 0.f;
        break;
    case PmMaskMode::GreenMode:
        filterCfg.mask_alpha = false;
        filterCfg.mask_color.x = 0.f;
        filterCfg.mask_color.y = 1.f;
        filterCfg.mask_color.z = 0.f;
        break;
    case PmMaskMode::MagentaMode:
        filterCfg.mask_alpha = false;
        filterCfg.mask_color.x = 1.f;
        filterCfg.mask_color.y = 0.f;
        filterCfg.mask_color.z = 1.f;
        break;
    case PmMaskMode::BlackMode:
    case PmMaskMode::CustomClrMode:
        filterCfg.mask_alpha = false;
        filterCfg.mask_color.x = 0.f;
        filterCfg.mask_color.y = 0.f;
        filterCfg.mask_color.z = 0.f;
    }
}

bool PmMatchConfig::operator==(const PmMatchConfig &other) const
{
    return matchImgFilename == other.matchImgFilename
        && label == other.label
        && totalMatchThresh == other.totalMatchThresh
        && maskMode == other.maskMode
        && filterCfg == other.filterCfg
        && targetScene == other.targetScene
        && targetTransition == other.targetTransition
        && lingerMs == other.lingerMs;
}

PmMatchConfig::PmMatchConfig(obs_data_t *data)
{
    obs_data_set_default_string(
        data, "match_image_filename", matchImgFilename.data());
    matchImgFilename = obs_data_get_string(data, "match_image_filename");

    obs_data_set_default_string(data, "label", label.data());
    label = obs_data_get_string(data, "label");

    obs_data_set_default_int(data, "roi_left", filterCfg.roi_left);
    filterCfg.roi_left = int(obs_data_get_int(data, "roi_left"));

    obs_data_set_default_int(data, "roi_bottom", filterCfg.roi_bottom);
    filterCfg.roi_bottom = int(obs_data_get_int(data, "roi_bottom"));

    obs_data_set_default_double(
        data, "per_pixel_allowed_error", double(filterCfg.per_pixel_err_thresh));
    filterCfg.per_pixel_err_thresh
        = float(obs_data_get_double(data, "per_pixel_allowed_error"));

    obs_data_set_default_double(
        data, "total_match_threshold", double(totalMatchThresh));
    totalMatchThresh 
        = float(obs_data_get_double(data, "total_match_threshold"));

    obs_data_set_default_int(data, "mask_mode", (int)maskMode);
    maskMode = PmMaskMode(obs_data_get_int(data, "mask_mode"));

    obs_data_set_default_bool(data, "mask_alpha", filterCfg.mask_alpha);
    filterCfg.mask_alpha = obs_data_get_bool(data, "mask_alpha");

    obs_data_set_default_vec3(data, "mask_color", &filterCfg.mask_color);
    obs_data_get_vec3(data, "mask_color", &filterCfg.mask_color);

    obs_data_set_default_bool(data, "is_enabled", filterCfg.is_enabled);
    filterCfg.is_enabled = obs_data_get_bool(data, "is_enabled");
    
    targetScene = obs_data_get_string(data, "target_scene");

    obs_data_set_default_string(data, "target_transition", targetTransition.data());
    targetTransition = obs_data_get_string(data, "target_transition");

    obs_data_set_default_int(data, "linger_ms", (int)lingerMs);
    lingerMs = obs_data_get_int(data, "linger_ms");
}

PmMatchConfig::PmMatchConfig(QXmlStreamReader &reader)
{
    while (true) {
        reader.readNext();
        if (reader.atEnd() || reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QStringRef name = reader.name();
        if (reader.isEndElement()) {
            if (name == "match_config") {
                return;
            }
        } else if (reader.isStartElement()) {
            if (name == "label") {
                label = reader.readElementText().toUtf8();
            } else if (name == "match_image_filename") {
                matchImgFilename =
                    reader.readElementText().toUtf8();
            } else if (name == "roi_left") {
                filterCfg.roi_left =
                    reader.readElementText().toInt();
            } else if (name == "roi_bottom") {
                filterCfg.roi_bottom =
                    reader.readElementText().toInt();
            } else if (name == "per_pixel_allowed_error") {
                filterCfg.per_pixel_err_thresh =
                    reader.readElementText().toFloat();
            } else if (name == "total_match_threshold") {
                totalMatchThresh =
                    reader.readElementText().toFloat();
            } else if (name == "mask_mode") {
                maskMode = (PmMaskMode)reader.readElementText()
                           .toInt();
            } else if (name == "mask_alpha") {
                filterCfg.mask_alpha =
                    reader.readElementText() == "true" ? true : false;
            } else if (name == "mask_color_r") {
                filterCfg.mask_color.x =
                    reader.readElementText().toFloat();
            } else if (name == "mask_color_g") {
                filterCfg.mask_color.y =
                    reader.readElementText().toFloat();
            } else if (name == "mask_color_b") {
                filterCfg.mask_color.z =
                    reader.readElementText().toFloat();
            } else if (name == "is_enabled") {
                filterCfg.mask_alpha =
                    reader.readElementText() == "true" ? true : false;
            } else if (name == "target_scene") {
                targetScene =
                    reader.readElementText().toUtf8();
            } else if (name == "target_transition") {
                targetTransition =
                    reader.readElementText().toUtf8();
            } else if (name == "linger_ms") {
                lingerMs = reader.readElementText().toInt();
            }
        }
    }
}

obs_data_t* PmMatchConfig::save() const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_string(ret, "match_image_filename", matchImgFilename.data());
    obs_data_set_string(ret, "label", label.data());
    obs_data_set_int(ret, "roi_left", filterCfg.roi_left);
    obs_data_set_int(ret, "roi_bottom", filterCfg.roi_bottom);
    obs_data_set_double(
        ret, "per_pixel_allowed_error", double(filterCfg.per_pixel_err_thresh));
    obs_data_set_double(
        ret, "total_match_threshold", double(totalMatchThresh));
    obs_data_set_int(ret, "mask_mode", (int)maskMode);
    obs_data_set_bool(ret, "mask_alpha", filterCfg.mask_alpha);
    obs_data_set_vec3(ret, "mask_color", &filterCfg.mask_color);

    obs_data_set_bool(ret, "is_enabled", filterCfg.is_enabled);
    obs_data_set_string(ret, "target_scene", targetScene.data());
    obs_data_set_string(ret, "target_transition", targetTransition.data());
    obs_data_set_int(ret, "linger_ms", (int)lingerMs);

    return ret;
}

void PmMatchConfig::saveXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("match_config");
    writer.writeTextElement("label", label.data());
    writer.writeTextElement("match_image_filename", matchImgFilename.data());
    writer.writeTextElement("roi_left",
        QString::number(filterCfg.roi_left));
    writer.writeTextElement("roi_bottom",
        QString::number(filterCfg.roi_bottom));
    writer.writeTextElement("per_pixel_allowed_error",
        QString::number(filterCfg.per_pixel_err_thresh));
    writer.writeTextElement("total_match_threshold",
        QString::number(totalMatchThresh));
    writer.writeTextElement("mask_mode", QString::number((int)maskMode));
    writer.writeTextElement("mask_alpha",
        filterCfg.mask_alpha ? "true" : "false");
    writer.writeTextElement("mask_color_r",
        QString::number(filterCfg.mask_color.x));
    writer.writeTextElement("mask_color_g",
        QString::number(filterCfg.mask_color.y));
    writer.writeTextElement("mask_color_b",
        QString::number(filterCfg.mask_color.z));
    writer.writeTextElement("is_enabled",
        filterCfg.is_enabled ? "true" : "false" );
    writer.writeTextElement("target_scene", targetScene.data());
    writer.writeTextElement("target_transition", targetTransition.data());
    writer.writeTextElement("linger_ms", QString::number((int)lingerMs));
    writer.writeEndElement();
}

const std::string PmMultiMatchConfig::k_defaultNoMatchScene = "";
const std::string PmMultiMatchConfig::k_defaultNoMatchTransition = "Cut";

PmMultiMatchConfig::PmMultiMatchConfig(obs_data_t* data)
{
    obs_data_array_t* matchEntriesArray
        = obs_data_get_array(data, "entries");
    size_t count = obs_data_array_count(matchEntriesArray);
    for (size_t i = 0; i < count; ++i) {
        obs_data_t* entryObj = obs_data_array_item(matchEntriesArray, i);
        push_back(PmMatchConfig(entryObj));
        obs_data_release(entryObj);
    }
    obs_data_array_release(matchEntriesArray);

    noMatchScene = obs_data_get_string(data, "no_match_scene");

    obs_data_set_default_string(
        data, "no_match_transition", noMatchTransition.data());
    std::string str = obs_data_get_string(data, "no_match_transition");
    if (str.size())
        noMatchTransition = str;
}

PmMultiMatchConfig::PmMultiMatchConfig(
    QXmlStreamReader &reader, std::string &presetName)
{
    while (true) {
        reader.readNext();
        if (reader.atEnd() || reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QStringRef name = reader.name();
        if (reader.isEndElement()) {
            if (name == "preset") {
                return;
            }
        } else {
            if (name == "name") {
                presetName = reader.readElementText().toUtf8();
            } else if (name == "no_match_scene") {
                noMatchScene = reader.readElementText().toUtf8();
            } else if (name == "no_match_transition") {
                noMatchTransition = reader.readElementText().toUtf8();
            } else if (name == "match_config") {
                PmMatchConfig cfg(reader);
                push_back(cfg);
            }
        }
    }
}

obs_data_t* PmMultiMatchConfig::save(const std::string& presetName)
{
    obs_data_t* ret = obs_data_create();
    obs_data_set_string(ret, "name", presetName.data());
    obs_data_set_int(ret, "num_entries", int(size()));

    obs_data_array_t* matchEntriesArray = obs_data_array_create();
    for (const auto& entry : *this) {
        obs_data_t* entryObj = entry.save();
        obs_data_array_push_back(matchEntriesArray, entryObj);
        obs_data_release(entryObj);
    }
    obs_data_set_array(ret, "entries", matchEntriesArray);
    obs_data_array_release(matchEntriesArray);
    
    obs_data_set_string(ret, "no_match_scene", noMatchScene.data());
    obs_data_set_string(ret, "no_match_transition", noMatchTransition.data());
    return ret;
}

void PmMultiMatchConfig::saveXml(
    QXmlStreamWriter &writer, const std::string &presetName) const
{
    writer.writeStartElement("preset");
    writer.writeTextElement("name", presetName.data());
    writer.writeTextElement("no_match_scene", noMatchScene.data());
    writer.writeTextElement("no_match_transition", noMatchTransition.data());
    for (const auto &cfg : *this) {
        cfg.saveXml(writer);
    }
    writer.writeEndElement();
}

bool PmMultiMatchConfig::operator==(const PmMultiMatchConfig& other) const
{
    if (size() != other.size()) {
        return false;
    }
    else {
        for (size_t i = 0; i < size(); ++i) {
            if (at(i) != other.at(i)) {
                return false;
            }
        }
        return noMatchScene == other.noMatchScene
            && noMatchTransition == other.noMatchTransition;
    }
}

QSet<std::string> PmScenes::sceneNames() const 
{
    QSet<std::string> ret;
    for (const auto &name : values()) {
        ret.insert(name);
    }
    return ret;
}

PmScenes::PmScenes(const PmScenes &other)
    : QHash<OBSWeakSource, std::string>(other)
{
}

PmPreviewConfig::PmPreviewConfig(obs_data_t* data)
{
    obs_data_set_default_int(data, "preview_mode", int(previewMode));
    previewMode = PmPreviewMode(obs_data_get_int(data, "preview_mode"));
}

obs_data_t* PmPreviewConfig::save() const
{
    obs_data_t* ret = obs_data_create();

    obs_data_set_int(ret, "preview_mode", int(previewMode));

    return ret;
}

bool PmPreviewConfig::operator==(const PmPreviewConfig& other) const
{
    return previewMode == other.previewMode;
}

void pmRegisterMetaTypes()
{
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmMultiMatchConfig>("PmMultiMatchConfig");
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchPresets>("PmMatchPresets");
    qRegisterMetaType<PmPreviewConfig>("PmPreviewConfig");
    qRegisterMetaType<PmScenes>("PmScenes");
    qRegisterMetaType<PmFilterRef>("PmFilterRef");
    qRegisterMetaType<PmCaptureState>("PmCaptureState");
    qRegisterMetaType<PmMultiMatchResults>("PmMultiMatchResults");
    qRegisterMetaType<QList<std::string>>("QList<std::string>");
}

PmMatchPresets PmMatchPresets::importXml(const std::string &filename)
{
    PmMatchPresets ret;
    QFile file(filename.data());
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        std::stringstream oss;
        oss << "Unable to open file: " << filename;
        throw std::exception(oss.str().data());
    }

    QXmlStreamReader xml(&file);
    bool readingPresets = false;
    while (true) {
        xml.readNext();
        if (xml.atEnd() || xml.error() != QXmlStreamReader::NoError) {
            break;
        }

        auto name = xml.name();
        if (name == "presets") {
            if (xml.isStartElement()) {
                readingPresets = true;
            } else {
                readingPresets = false;
            }
        } else if (readingPresets) {
            if (name == "preset" && xml.isStartElement()) {
                std::string presetName;
                PmMultiMatchConfig preset(xml, presetName);
                ret.insert(presetName, preset);
            }
        }
    }
    return ret;
}

void PmMatchPresets::exportXml(const std::string &filename,
                               const QList<std::string> &selectedPresets) const
{
    QFile file(filename.data());
    file.open(QIODevice::WriteOnly);
    if (!file.isOpen()) {
        std::stringstream oss;
        oss << "Unable to open file: " << filename;
        throw std::exception(oss.str().data());
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE pixel-match-switcher>");
    xml.writeStartElement("presets");
    xml.writeAttribute("obs_version", OBS_VERSION); // TODO: plugin version as well

    for (const auto &presetName : selectedPresets) {
        auto f = find(presetName);
        if (f != end()) {
            const auto &preset = *f;
            preset.saveXml(xml, presetName);
        }
    }
    xml.writeEndDocument();
}
