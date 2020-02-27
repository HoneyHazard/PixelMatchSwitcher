#pragma once

#include <pthread.h>
#include <obs-module.h>
#include <graphics/image-file.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

#define READBACK 1

struct pm_filter_data
{
    // plugin basics
    obs_source_t *context;
    gs_effect_t *effect;
    obs_data_t *settings;

    // shader parameters and results
    gs_eparam_t *param_per_pixel_err_thresh;
    gs_eparam_t *param_debug;
    gs_eparam_t *param_match_img;
    gs_eparam_t *param_match_counter;
    gs_eresult_t *result_match_counter;

    // match stuff
    gs_image_file_t match_file;
    int roi_left;
    int roi_bottom;
    int roi_right;
    int roi_top;
    int per_pixel_err_thresh;
    int total_match_thresh;

    // dynamic data
    pthread_mutex_t mutex;
    uint32_t cx;
    uint32_t cy;
    uint32_t num_matched;

    // debug and visualization
    bool debug;

    // callbacks for fast reactions
    void (*on_frame_processed)(struct pm_filter_data *sender);
};
