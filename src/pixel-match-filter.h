#pragma once

#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

struct pixel_match_filter_data {
    obs_source_t *context;

    bool enabled;

    int roi_left;
    int roi_bottom;
    int roi_right;
    int roi_top;

    int per_pixel_err_thresh;
    int total_match_thresh;

    bool frame_wanted;
    bool frame_available;
    gs_texrender_t *frame_copy;
    bool frame_matched_prev;
    bool frame_matched;
};
