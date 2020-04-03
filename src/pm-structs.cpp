#include "pm-structs.hpp"

uint qHash(const OBSWeakSource &ws)
{
    obs_source_t* source = obs_weak_source_get_source(ws);
    return qHash(source);
}

bool PmMatchConfig::operator!=(const PmMatchConfig &other) const
{
    // TODO: hacky! clean up.
    if (this->matchImgFilename != other.matchImgFilename) {
        return true;
    } else {
        return !!memcmp(&this->roiLeft, &other.roiLeft,
                        sizeof(PmMatchConfig)-sizeof(std::string));
    }
}

PmMatchConfig::PmMatchConfig(obs_data_t *data)
{
    // TODO
}

obs_data_t* PmMatchConfig::save(const std::string &presetName) const
{
    obs_data_t *ret = obs_data_create();
    obs_data_set_string(ret, "name", presetName.data());
    obs_data_set_string(ret, "match_image_filename", matchImgFilename.data());
    obs_data_set_int(ret, "roi_left", roiLeft);
    obs_data_set_int(ret, "roi_bottom", roiBottom);
    obs_data_set_double(ret, "per_pixel_allowed_error", double(perPixelErrThresh));
    obs_data_set_double(ret, "total_match_threshold", double(totalMatchThresh));
    obs_data_set_int(ret, "mask_mode", int(maskMode));
    obs_data_set_int(ret, "custom_color", customColor);
    obs_data_set_int(ret, "preview_mode", int(previewMode));
    obs_data_set_double(ret, "preview_video_scale", double(previewVideoScale));
    obs_data_set_double(ret, "preview_region_scale", double(previewRegionScale));
    obs_data_set_double(ret, "preview_match_image_scale", double(previewMatchImageScale));
    return ret;
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

void pmRegisterMetaTypes()
{
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<PmMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchResults>("PmMatchPresets");
    qRegisterMetaType<PmMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmSwitchConfig>("PmSwitchConfig");
    qRegisterMetaType<PmScenes>("PmScenes");
}
