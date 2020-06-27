#include "pm-structs.hpp"

uint qHash(const OBSWeakSource &ws)
{
    obs_source_t* source = obs_weak_source_get_source(ws);
    return qHash(source);
}

bool operator== (const struct pm_match_entry_config& l, 
                 const struct pm_match_entry_config& r)
{
    return l.roi_left == r.roi_left
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
}

bool PmMatchConfig::operator==(const PmMatchConfig &other) const
{
    return matchImgFilename == other.matchImgFilename
        && label == other.label
        && totalMatchThresh == other.totalMatchThresh
#if 0
        && roiLeft == other.roiLeft
        && roiBottom == other.roiBottom
        && perPixelErrThresh == other.perPixelErrThresh
        && totalMatchThresh == other.totalMatchThresh
        && maskMode == other.maskMode
        && customColor == other.customColor
#endif
        && filterCfg == other.filterCfg
        && previewMode == other.previewMode
        && previewVideoScale == other.previewVideoScale
        && previewRegionScale == other.previewRegionScale
        && previewMatchImageScale == other.previewMatchImageScale
        && matchScene == other.matchScene
        && matchTransition == other.matchTransition;
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

    obs_data_set_default_bool(data, "mask_alpha", filterCfg.mask_alpha);
    filterCfg.mask_alpha = obs_data_get_bool(data, "mask_alpha");

    obs_data_set_default_vec3(data, "mask_color", &filterCfg.mask_color);
    obs_data_get_vec3(data, "mask_color", &filterCfg.mask_color);

    obs_data_set_default_int(data, "preview_mode", int(previewMode));
    previewMode = PmPreviewMode(obs_data_get_int(data, "preview_mode"));

    obs_data_set_default_double(
        data, "preview_video_scale", double(previewVideoScale));
    previewVideoScale
        = float(obs_data_get_double(data, "preview_video_scale"));

    obs_data_set_default_double(
        data, "preview_region_scale", double(previewRegionScale));
    previewRegionScale
        = float(obs_data_get_double(data, "preview_region_scale"));

    obs_data_set_default_double(
        data, "preview_match_image_scale", double(previewMatchImageScale));
    previewMatchImageScale
        = float(obs_data_get_double(data, "preview_match_image_scale"));

    obs_data_set_default_bool(data, "is_enabled", filterCfg.is_enabled);
    filterCfg.is_enabled = obs_data_get_bool(data, "is_enabled");
    
    matchScene = obs_data_get_string(data, "match_scene");
    matchTransition = obs_data_get_string(data, "match_transition");
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
    obs_data_set_bool(ret, "mask_alpha", filterCfg.mask_alpha);
    obs_data_set_vec3(ret, "mask_color", &filterCfg.mask_color);
    obs_data_set_int(ret, "preview_mode", int(previewMode));
    obs_data_set_double(
        ret, "preview_video_scale", double(previewVideoScale));
    obs_data_set_double(
        ret, "preview_region_scale", double(previewRegionScale));
    obs_data_set_double(
        ret, "preview_match_image_scale", double(previewMatchImageScale));

    obs_data_set_bool(ret, "is_enabled", filterCfg.is_enabled);
    obs_data_set_string(ret, "match_scene", matchScene.data());
        //obs_source_get_name(obs_weak_source_get_source(matchScene)));
    obs_data_set_string(ret, "match_transition", matchTransition.data());
        //obs_source_get_name(obs_weak_source_get_source(transition)));

    return ret;
}

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
    noMatchTransition = obs_data_get_string(data, "no_match_transition");
}

obs_data_t* PmMultiMatchConfig::save(const std::string& presetName)
{
    obs_data_t* ret = obs_data_create();
    obs_data_set_string(ret, "name", presetName.data());
    obs_data_set_int(ret, "num_entries", size());

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
        return true;
    }
}

#if 0
PmSwitchConfig::PmSwitchConfig(obs_data_t *data)
{
    // TODO
}

obs_data_t *PmSwitchConfig::save() const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_bool(ret, "enabled", isEnabled);
    obs_data_set_string(ret, "match_scene",
        obs_source_get_name(obs_weak_source_get_source(matchScene)));
    obs_data_set_string(ret, "no_match_scene",
        obs_source_get_name(obs_weak_source_get_source(noMatchScene)));
    obs_data_set_string(ret, "transition",
        obs_source_get_name(obs_weak_source_get_source(defaultTransition)));
    return ret;
}
#endif

void pmRegisterMetaTypes()
{
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchPresets>("PmMatchPresets");
    qRegisterMetaType<PmScenes>("PmScenes");
}

