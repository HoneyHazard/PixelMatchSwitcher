#pragma once

#include <pthread.h>
#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

typedef struct pm_match_entry_config
{
    // params
    int roi_left;
    int roi_bottom;
    bool mask_alpha;
    struct vec3 mask_color;
    float per_pixel_err_thresh;
};

typedef struct pm_match_entry_data
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

struct pm_filter_data
{
    // plugin basics
    obs_source_t *context;
    gs_effect_t *effect;
    obs_data_t *settings;

    // shader parameters and results
    gs_eparam_t *param_visualize;
    gs_eparam_t *param_show_border;
    gs_eparam_t *param_px_width;
    gs_eparam_t *param_px_height;

    gs_eparam_t *param_roi_left;
    gs_eparam_t *param_roi_bottom;
    gs_eparam_t *param_roi_right;
    gs_eparam_t *param_roi_top;
    gs_eparam_t *param_per_pixel_err_thresh;
    gs_eparam_t *param_mask_color;
    gs_eparam_t *param_mask_alpha;
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
    bool preview_mode;
    bool show_border;
    uint32_t base_width;
    uint32_t base_height;

    // debug and visualization
    bool visualize;

    // callbacks for fast reactions
    void (*on_frame_processed)(struct pm_filter_data *sender);
};

static void pm_destroy_match_entries(struct pm_filter_data* filter);
