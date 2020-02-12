#pragma once

#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

#define READBACK 1

struct pixel_match_filter_data
{
    obs_source_t *context;

    gs_effect_t *effect;
    gs_eparam_t *param_per_pixel_err_thresh;
    gs_eparam_t *param_match_counter;
    gs_eresult_t *result_match_counter;

    bool enabled;

    // TODO: maybe we don't need these
    int roi_left;
    int roi_bottom;
    int roi_right;
    int roi_top;

    int per_pixel_err_thresh;
    int total_match_thresh;

    unsigned int cx;
    unsigned int cy;
    unsigned int num_matched;
};
