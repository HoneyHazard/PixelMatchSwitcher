#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"
#include "pm-version.hpp"

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
        && wasDownloaded == other.wasDownloaded
        && label == other.label
        && totalMatchThresh == other.totalMatchThresh
        && invertResult == other.invertResult
        && maskMode == other.maskMode
        && filterCfg == other.filterCfg
        && reaction == other.reaction;
}

PmMatchConfig::PmMatchConfig(obs_data_t *data)
{
    memset(&filterCfg, 0, sizeof(pm_match_entry_config));

    obs_data_set_default_string(
        data, "match_image_filename", matchImgFilename.data());
    matchImgFilename = obs_data_get_string(data, "match_image_filename");

    obs_data_set_default_bool(data, "was_downloaded", wasDownloaded);
    wasDownloaded = obs_data_get_bool(data, "was_downloaded");

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

    obs_data_set_default_bool(data, "invert_result", invertResult);
    invertResult = obs_data_get_bool(data, "invert_result");

    obs_data_set_default_int(data, "mask_mode", int(maskMode));
    maskMode = PmMaskMode(obs_data_get_int(data, "mask_mode"));

    obs_data_set_default_bool(data, "mask_alpha", filterCfg.mask_alpha);
    filterCfg.mask_alpha = obs_data_get_bool(data, "mask_alpha");

    obs_data_set_default_vec3(data, "mask_color", &filterCfg.mask_color);
    obs_data_get_vec3(data, "mask_color", &filterCfg.mask_color);

    obs_data_set_default_bool(data, "is_enabled", filterCfg.is_enabled);
    filterCfg.is_enabled = obs_data_get_bool(data, "is_enabled");

    obs_data_t *reactionObj = obs_data_get_obj(data, "reaction");
    reaction = PmReaction(reactionObj);
    obs_data_release(reactionObj);
}

PmMatchConfig::PmMatchConfig(QXmlStreamReader &reader)
{
    memset(&filterCfg, 0, sizeof(pm_match_entry_config));

    while (true) {
        reader.readNext();
        if (reader.atEnd() || reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QString name = reader.name().toString();
        if (reader.isEndElement()) {
            if (name == "match_config") {
                return;
            }
        } else if (reader.isStartElement()) {
            if (name == "reaction") {
                reaction = PmReaction(reader);
            } else {
                QString elemText = reader.readElementText();
                if (name == "label") {
                    label = elemText.toUtf8().data();
                } else if (name == "match_image_filename") {
                    matchImgFilename = elemText.toUtf8().data();
                } else if (name == "was_downloaded") {
                    wasDownloaded = (elemText == "true" ? true : false);
                } else if (name == "roi_left") {
                    filterCfg.roi_left = elemText.toInt();
                } else if (name == "roi_bottom") {
                    filterCfg.roi_bottom = elemText.toInt();
                } else if (name == "per_pixel_allowed_error") {
                    filterCfg.per_pixel_err_thresh = elemText.toFloat();
                } else if (name == "total_match_threshold") {
                    totalMatchThresh = elemText.toFloat();
                } else if (name == "invert_result") {
                    invertResult = (elemText == "true" ? true : false);
                } else if (name == "mask_mode") {
                    maskMode = PmMaskMode(elemText.toInt());
                } else if (name == "mask_alpha") {
                    filterCfg.mask_alpha = (elemText == "true" ? true : false);
                } else if (name == "mask_color_r") {
                    filterCfg.mask_color.x = elemText.toFloat();
                } else if (name == "mask_color_g") {
                    filterCfg.mask_color.y = elemText.toFloat();
                } else if (name == "mask_color_b") {
                    filterCfg.mask_color.z = elemText.toFloat();
                } else if (name == "is_enabled") {
                    filterCfg.is_enabled = (elemText == "true" ? true : false);
                }
            }
        }
    }
}

obs_data_t* PmMatchConfig::save() const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_string(ret, "match_image_filename", matchImgFilename.data());
    obs_data_set_bool(ret, "was_downloaded", wasDownloaded);
    obs_data_set_string(ret, "label", label.data());
    obs_data_set_int(ret, "roi_left", filterCfg.roi_left);
    obs_data_set_int(ret, "roi_bottom", filterCfg.roi_bottom);
    obs_data_set_double(
        ret, "per_pixel_allowed_error", double(filterCfg.per_pixel_err_thresh));
    obs_data_set_double(
        ret, "total_match_threshold", double(totalMatchThresh));
    obs_data_set_bool(ret, "invert_result", invertResult);
    obs_data_set_int(ret, "mask_mode", int(maskMode));
    obs_data_set_bool(ret, "mask_alpha", filterCfg.mask_alpha);
    obs_data_set_vec3(ret, "mask_color", &filterCfg.mask_color);

    obs_data_set_bool(ret, "is_enabled", filterCfg.is_enabled);

    obs_data_t *reactionObj = reaction.saveData();
    obs_data_set_obj(ret, "reaction", reactionObj);
    obs_data_release(reactionObj);

    return ret;
}

void PmMatchConfig::saveXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement("match_config");
    writer.writeTextElement("label", label.data());
    if (matchImgFilename.size()) {
        writer.writeTextElement(
            "match_image_filename", matchImgFilename.data());
    }
    writer.writeTextElement("was_downloaded", wasDownloaded ? "true" : "false");
    writer.writeTextElement("roi_left", QString::number(filterCfg.roi_left));
    writer.writeTextElement("roi_bottom", 
        QString::number(filterCfg.roi_bottom));
    writer.writeTextElement("per_pixel_allowed_error",
        QString::number(double(filterCfg.per_pixel_err_thresh)));
    writer.writeTextElement("total_match_threshold", 
        QString::number(double(totalMatchThresh)));
    writer.writeTextElement("invert_result", invertResult ? "true" : "false");
    writer.writeTextElement("mask_mode", QString::number(int(maskMode)));
    writer.writeTextElement("mask_alpha",
        filterCfg.mask_alpha ? "true" : "false");
    writer.writeTextElement("mask_color_r", 
        QString::number(double(filterCfg.mask_color.x)));
    writer.writeTextElement("mask_color_g",
        QString::number(double(filterCfg.mask_color.y)));
    writer.writeTextElement("mask_color_b",
        QString::number(double(filterCfg.mask_color.z)));
    writer.writeTextElement("is_enabled",
        filterCfg.is_enabled ? "true" : "false" );
    if (reaction.isSet()) {
        reaction.saveXml(writer);
    }
    writer.writeEndElement();
}

const std::string PmMultiMatchConfig::k_defaultNoMatchTransition = "Cut";

//******************************************************************************

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

    obs_data_t *noMatchReactionObj = obs_data_get_obj(data, "reaction");
    noMatchReaction = PmReaction(noMatchReactionObj);
    obs_data_release(noMatchReactionObj);
}

PmMultiMatchConfig::PmMultiMatchConfig(
    QXmlStreamReader &reader, std::string &presetName)
{
    while (true) {
        reader.readNext();
        if (reader.atEnd() || reader.error() != QXmlStreamReader::NoError) {
            return;
        }

        QString name = reader.name().toString();
        if (reader.isEndElement()) {
            if (name == "preset") {
                return;
            }
        } else {
            if (name == "name") {
                presetName = reader.readElementText().toUtf8().data();
            } else if (name == "reaction") {
                noMatchReaction = PmReaction(reader);
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

    obs_data_t *noMatchReactionObj = noMatchReaction.saveData();
    obs_data_set_obj(ret, "reaction", noMatchReactionObj);
    obs_data_release(noMatchReactionObj);

    return ret;
}

void PmMultiMatchConfig::saveXml(
    QXmlStreamWriter &writer, const std::string &presetName) const
{
    writer.writeStartElement("preset");
    writer.writeTextElement("name", presetName.data());
    if (noMatchReaction.isSet()) {
        noMatchReaction.saveXml(writer);
    }
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
        return noMatchReaction == other.noMatchReaction;
    }
}

bool PmMultiMatchConfig::containsImage(
    const std::string &imgFilename, size_t exceptIndex) const
{
    for (size_t i = 0; i < size(); ++i) {
        if (i == exceptIndex)
            continue;

        const auto &cfg = at(i);
        if (cfg.matchImgFilename == imgFilename)
            return true;
    }

    return false;
}

//******************************************************************************

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

//******************************************************************************

PmMatchPresets::PmMatchPresets(const std::string& filename)
{
    PmMatchPresets ret;
    QFile file(filename.data());
    file.open(QIODevice::ReadOnly);
    if (!file.isOpen()) {
        std::stringstream oss;
        oss << "Unable to open file: " << filename;
        throw std::runtime_error(oss.str().data());
    }

    QXmlStreamReader reader(&file);
    importXml(reader);
}

PmMatchPresets::PmMatchPresets(const QByteArray& data)
{
    QXmlStreamReader reader(data);
    importXml(reader);
}

void PmMatchPresets::importXml(QXmlStreamReader &xml)
{
    while (true) {
        auto error = xml.error();
        if (error != QXmlStreamReader::NoError) {
            throw std::runtime_error(xml.errorString().toUtf8().data());
        }

        xml.readNext();
        if (xml.atEnd()) {
            break;
        }

        if (xml.name().toString() == "preset" && xml.isStartElement()) {
            std::string presetName;
            PmMultiMatchConfig preset(xml, presetName);
            insert(presetName, preset);
        }
    }
}

void PmMatchPresets::exportXml(const std::string &filename,
                               const QList<std::string> &selectedPresets) const
{
    QFile file(filename.data());
    file.open(QIODevice::WriteOnly);
    if (!file.isOpen()) {
        std::stringstream oss;
        oss << "Unable to open file: " << filename;
        throw std::runtime_error(oss.str().data());
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE pixel-match-switcher>");
    xml.writeStartElement("pixel_match_switcher");
    xml.writeTextElement("obs_version", OBS_VERSION);
    xml.writeTextElement("plugin_version", PM_VERSION);

    for (const auto &presetName : selectedPresets) {
        auto f = find(presetName);
        if (f != end()) {
            const auto &preset = *f;
            preset.saveXml(xml, presetName);
        }
    }
    xml.writeEndDocument();
}

bool PmMatchPresets::containsImage(const std::string &imgFilename) const
{
    for (const auto &mcfg : values()) {
        if (mcfg.containsImage(imgFilename)) {
            return true;
        }
    }

    return false;
}

QSet<std::string> PmMatchPresets::orphanedImages(
    const PmMultiMatchConfig &beingRemoved, const PmMultiMatchConfig* activeCfg)
{
    QSet<std::string> ret;
    for (const auto &rcfg : beingRemoved) {
        if (rcfg.wasDownloaded && rcfg.matchImgFilename.size()
        && (!activeCfg || !activeCfg->containsImage(rcfg.matchImgFilename))
        && !this->containsImage(rcfg.matchImgFilename)) {
            ret.insert(rcfg.matchImgFilename);
        }
    }
    return ret;
}

//******************************************************************************

bool PmSourceData::operator==(const PmSourceData &other) const
{
    return wsrc == other.wsrc && childNames == other.childNames;
}

//******************************************************************************

bool PmSceneItemData::operator==(const PmSceneItemData &other) const
{
    return si == other.si && filtersNames == other.filtersNames;
}

//******************************************************************************

QSet<std::string> PmSourceHash::sourceNames() const
{
    QSet<std::string> ret;
    for (const std::string &k : keys()) {
        ret.insert(k);
    }
    return ret;
}

//******************************************************************************

QSet<std::string> PmSceneItemsHash::sceneItemNames() const
{
    QSet<std::string> ret;
    for (const std::string &k : keys()) {
        ret.insert(k);
    }
    return ret;
}

//******************************************************************************

void pmRegisterMetaTypes()
{
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmMultiMatchConfig>("PmMultiMatchConfig");
    qRegisterMetaType<PmReaction>("PmReaction");
    qRegisterMetaType<PmAction>("PmAction");
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchPresets>("PmMatchPresets");
    qRegisterMetaType<PmPreviewConfig>("PmPreviewConfig");
    qRegisterMetaType<PmSourceHash>("PmSourceHash");
    qRegisterMetaType<PmSceneItemsHash>("PmSceneItemsHash");
    qRegisterMetaType<PmFilterRef>("PmFilterRef");
    qRegisterMetaType<PmCaptureState>("PmCaptureState");
    qRegisterMetaType<PmMultiMatchResults>("PmMultiMatchResults");
    qRegisterMetaType<QList<std::string>>("QList<std::string>");
}
