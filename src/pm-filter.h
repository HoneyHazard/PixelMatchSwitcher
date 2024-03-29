#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include "pm-module.h"

struct pm_match_entry_config
{
    // params
    int roi_left;
    int roi_bottom;
    float per_pixel_err_thresh;
    bool is_enabled;
    bool mask_alpha;
    struct vec3 mask_color;
};

struct pm_match_entry_data
{
    struct pm_match_entry_config cfg;

    // match image data
    void* match_img_data;
    uint32_t match_img_width, match_img_height;
    gs_texture_t* match_img_tex;

    // results
    uint32_t num_compared;
    uint32_t num_matched;
};

enum pm_filter_mode { 
    PM_MATCH = 0, PM_MATCH_VISUALIZE = 1, 
    PM_MASK_BEGIN = 2, PM_MASK = 3, PM_MASK_END = 4, PM_MASK_VISUALIZE = 5, 
    PM_SELECT_REGION_VISUALIZE = 6, PM_SNAPSHOT = 7
};

struct pm_filter_data
{
    // plugin basics
    obs_source_t *context;
    gs_effect_t *effect;
    //obs_data_t *settings;

    // shader parameters and results
    gs_eparam_t *param_image;
    gs_eparam_t *param_show_color_indicator;
    gs_eparam_t *param_show_border;
    gs_eparam_t *param_border_px_width;
    gs_eparam_t *param_border_px_height;

    gs_eparam_t *param_roi_left;
    gs_eparam_t *param_roi_bottom;
    gs_eparam_t *param_roi_right;
    gs_eparam_t *param_roi_top;
    gs_eparam_t *param_per_pixel_err_thresh;
    gs_eparam_t *param_mask_color;
    gs_eparam_t *param_mask_alpha;
    gs_eparam_t* param_store_match_alpha;
    gs_eparam_t *param_match_img;
    gs_eparam_t *param_compare_counter;
    gs_eparam_t *param_match_counter;
    gs_eresult_t *result_compare_counter;
    gs_eresult_t *result_match_counter;

    // match data
    size_t num_match_entries;
    struct pm_match_entry_data* match_entries;

    // dynamic data
    pthread_mutex_t mutex;
    size_t selected_match_index;
    uint32_t base_width;
    uint32_t base_height;
    enum pm_filter_mode filter_mode;

    // selection mode and snapshot
    uint32_t select_left, select_bottom, select_right, select_top;
    uint8_t* captured_region_data;

    gs_texrender_t* snapshot_texrender;
    gs_stagesurf_t* snapshot_stagesurface;
    
    gs_texrender_t* mask_texrender;
    gs_stagesurf_t* mask_stagesurface;
    gs_texture_t* mask_region_texture;

    // callbacks for fast reactions
    void (*on_match_image_captured)(struct pm_filter_data *data);
    void (*on_frame_processed)(struct pm_filter_data* filter_data);

    // callback for showing dialog upon settings button in filter UI
    void (*on_settings_button_released)();
};

void pm_destroy_match_gfx(struct gs_texture *tex, void *img_data);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" void pm_supply_match_entry_config(struct pm_filter_data *filter,
                size_t match_idx, const struct pm_match_entry_config *cfg);

extern "C" void pm_resize_match_entries(
                struct pm_filter_data *filter, size_t new_size);
#else
void pm_supply_match_entry_config(struct pm_filter_data *filter,
     size_t match_idx, const struct pm_match_entry_config *cfg);

void pm_resize_match_entries(struct pm_filter_data *filter, size_t new_size);
#endif
