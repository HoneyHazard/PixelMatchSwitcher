#pragma once

#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

#define READBACK 1

struct pixel_match_filter_data
{
    obs_source_t *context;
    gs_effect_t *effect;
    obs_data_t *settings;

    gs_texture_t *match_tex;
    gs_eparam_t *param_per_pixel_err_thresh;
    gs_eparam_t *param_match_counter;
    gs_eparam_t *param_debug;
    gs_eparam_t *param_count_enabled;
    gs_eresult_t *result_match_counter;

    bool enabled;

    int roi_left;
    int roi_bottom;
    int roi_right;
    int roi_top;

    int per_pixel_err_thresh;
    int total_match_thresh;
    bool count_enabled;
    bool debug;

    unsigned int cx;
    unsigned int cy;
    unsigned int num_matched;
};
