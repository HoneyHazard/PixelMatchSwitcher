#pragma once

#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

struct pixel_match_filter_data
{
    obs_source_t *context;

    bool enabled;

    // TODO: maybe we don't need these
    int roi_left;
    int roi_bottom;
    int roi_right;
    int roi_top;

    int per_pixel_err_thresh;
    int total_match_thresh;

    bool frame_wanted;
    bool frame_available;
    unsigned int cx;
    unsigned int cy;
    gs_texrender_t *tex_render;
    uint8_t *pixel_data;
};
