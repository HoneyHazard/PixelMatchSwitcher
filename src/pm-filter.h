#pragma once

#include <pthread.h>
#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#define PIXEL_MATCH_FILTER_DISPLAY_NAME obs_module_text("Pixel Match Filter")

struct pm_match_entry_config
{
    // params
    bool is_enabled;
    int roi_left;
    int roi_bottom;
    bool mask_alpha;
    struct vec3 mask_color;
    float per_pixel_err_thresh;
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
    size_t selected_match_index;
    uint32_t base_width;
    uint32_t base_height;

    // debug and visualization
    bool visualize;

    // callbacks for fast reactions
    void (*on_frame_processed)(struct pm_filter_data *sender);
};

static void pm_destroy_match_entry_gfx(
    struct pm_filter_data* filter, size_t match_idx)
{
    if (match_idx >= filter->num_match_entries) return;

    pthread_mutex_lock(&filter->mutex);
    struct pm_match_entry_data* entry = filter->match_entries + match_idx;
    if (entry->match_img_tex) {
        obs_enter_graphics();
        gs_texture_destroy(entry->match_img_tex);
        obs_leave_graphics();
        entry->match_img_tex = NULL;
    }
    if (entry->match_img_data) {
        bfree(entry->match_img_data);
        entry->match_img_data = NULL;
    }
    pthread_mutex_unlock(&filter->mutex);
}

static void pm_supply_match_entry_gfx(
    struct pm_filter_data* filter, size_t match_idx,
    unsigned char* bits, uint32_t width, uint32_t height)
{
    if (match_idx >= filter->num_match_entries) return;

    pm_destroy_match_entry_gfx(filter, match_idx);

    pthread_mutex_lock(&filter->mutex);
    struct pm_match_entry_data* entry = filter->match_entries + match_idx;
    entry->match_img_width = width;
    entry->match_img_height = height;

    size_t sz = (size_t)width * (size_t)height * 3;
    entry->match_img_data = bmalloc(sz);
    memcpy(entry->match_img_data, bits, sz);
    pthread_mutex_unlock(&filter->mutex);
}

static void pm_supply_match_entry_config(
    struct pm_filter_data* filter, size_t match_idx,
    const struct pm_match_entry_config* cfg)
{
    if (match_idx >= filter->num_match_entries) return;
    pthread_mutex_lock(&filter->mutex);
    struct pm_match_entry_data* entry = filter->match_entries + match_idx;
    memcpy(&entry->cfg, cfg, sizeof(struct pm_match_entry_config));
    pthread_mutex_unlock(&filter->mutex);
}

static void pm_resize_match_entries(
    struct pm_filter_data* filter, size_t new_size)
{
    if (new_size == filter->num_match_entries) return;

    pthread_mutex_lock(&filter->mutex);
    struct pm_match_entry_data* old_entries = filter->match_entries;
    size_t old_num_entries = filter->num_match_entries;

    struct pm_match_entry_data* new_entries;

    if (new_size > 0) {
        new_entries = (struct pm_match_entry_data*)bmalloc(
            sizeof(struct pm_match_entry_data) * new_size);
        for (size_t i = 0; i < new_size; ++i) {
            if (i < old_num_entries) {
                memcpy((void*)(new_entries + i),
                       (void*)(old_entries + i),
                       sizeof(struct pm_match_entry_data));
            } else {
                memset((void*)(new_entries + i),
                       0, sizeof(struct pm_match_entry_data));
            }
        }
    } else {
        new_entries = NULL;
    }

    for (size_t i = new_size; i < filter->num_match_entries; i++)
        pm_destroy_match_entry_gfx(filter, i);
    if (old_entries)
        bfree(old_entries);

    filter->match_entries = new_entries;
    filter->num_match_entries = new_size;
    pthread_mutex_unlock(&filter->mutex);
}

static void pm_destroy_match_entries(struct pm_filter_data* filter)
{
    obs_enter_graphics();
    for (size_t i = 0; i < filter->num_match_entries; ++i) {
        struct pm_match_entry_data* entry = filter->match_entries + i;
        if (entry->match_img_tex)
            gs_texture_destroy(entry->match_img_tex);
        if (entry->match_img_data)
            bfree(entry->match_img_data);
    }
    obs_leave_graphics();
    if (filter->match_entries)
        bfree(filter->match_entries);
    filter->num_match_entries = 0;
    filter->match_entries = NULL;
}
