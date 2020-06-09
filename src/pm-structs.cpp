#include "pm-structs.hpp"

uint qHash(const OBSWeakSource &ws)
{
    obs_source_t* source = obs_weak_source_get_source(ws);
    return qHash(source);
}

bool PmMatchConfig::operator==(const PmMatchConfig &other) const
{
    return matchImgFilename == other.matchImgFilename
        && roiLeft == other.roiLeft
        && roiBottom == other.roiBottom
        && perPixelErrThresh == other.perPixelErrThresh
        && totalMatchThresh == other.totalMatchThresh
        && maskMode == other.maskMode
        && customColor == other.customColor
        && previewMode == other.previewMode
        && previewVideoScale == other.previewVideoScale
        && previewMatchImageScale == other.previewMatchImageScale
        && matchScene == other.matchScene
        && transition == other.transition;
}

PmMatchConfig::PmMatchConfig(obs_data_t *data)
{
    obs_data_set_default_string(
        data, "match_image_filename", matchImgFilename.data());
    matchImgFilename = obs_data_get_string(data, "match_image_filename");

    obs_data_set_default_int(data, "roi_left", roiLeft);
    roiLeft = int(obs_data_get_int(data, "roi_left"));

    obs_data_set_default_int(data, "roi_bottom", roiBottom);
    roiBottom = int(obs_data_get_int(data, "roi_bottom"));

    obs_data_set_default_double(
        data, "per_pixel_allowed_error", double(perPixelErrThresh));
    perPixelErrThresh
        = float(obs_data_get_double(data, "per_pixel_allowed_error"));

    obs_data_set_default_double(
        data, "total_match_threshold", double(totalMatchThresh));
    totalMatchThresh =float(obs_data_get_double(data, "total_match_threshold"));

    obs_data_set_default_int(data, "mask_mode", int(maskMode));
    maskMode = PmMaskMode(obs_data_get_int(data, "mask_mode"));

    obs_data_set_default_int(data, "custom_color", customColor);
    customColor = uint32_t(obs_data_get_int(data, "custom_color"));

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

    obs_data_set_default_bool(data, "is_enabled", isEnabled);
    isEnabled = obs_data_get_bool(data, "is_enabled");
    
    matchScene = obs_data_get_string(data, "match_scene");
    transition = obs_data_get_string(data, "transition");
}

obs_data_t* PmMatchConfig::save() const
{
    obs_data_t *ret = obs_data_create();
    //obs_data_set_string(ret, "name", presetName.data());
    obs_data_set_string(ret, "match_image_filename", matchImgFilename.data());
    obs_data_set_int(ret, "roi_left", roiLeft);
    obs_data_set_int(ret, "roi_bottom", roiBottom);
    obs_data_set_double(
        ret, "per_pixel_allowed_error", double(perPixelErrThresh));
    obs_data_set_double(
        ret, "total_match_threshold", double(totalMatchThresh));
    obs_data_set_int(ret, "mask_mode", int(maskMode));
    obs_data_set_int(ret, "custom_color", customColor);
    obs_data_set_int(ret, "preview_mode", int(previewMode));
    obs_data_set_double(
        ret, "preview_video_scale", double(previewVideoScale));
    obs_data_set_double(
        ret, "preview_region_scale", double(previewRegionScale));
    obs_data_set_double(
        ret, "preview_match_image_scale", double(previewMatchImageScale));

    obs_data_set_bool(ret, "is_enabled", isEnabled);
    obs_data_set_string(ret, "match_scene", matchScene.data());
        //obs_source_get_name(obs_weak_source_get_source(matchScene)));
    obs_data_set_string(ret, "transition", transition.data());
        //obs_source_get_name(obs_weak_source_get_source(transition)));

    return ret;
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
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<PmMultiMatchResults>("PmMatchResults");
    qRegisterMetaType<PmMatchPresets>("PmMatchPresets");
    qRegisterMetaType<PmMultiMatchConfig>("PmMatchConfig");
    qRegisterMetaType<PmScenes>("PmScenes");
}

obs_data_t* PmMultiMatchConfig::save(const std::string& presetName)
{
    //obs_data_set_string(ret, "name", presetName.data());
}
