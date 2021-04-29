#include "pm-filter.h"

#include <graphics/graphics.h>

const float PM_VISUALIZE_BORDER_THICKNESS = 2.f;
const float PM_SELECT_REGION_BORDER_THICKNESS = 4.f;
const float PM_AUTOMASK_BORDER_THICKNESS = 4.f;

struct vec3 vec3_dummy;

bool pm_filter_failed = false;

static const char *pixel_match_filter_get_name(void* unused)
{
    UNUSED_PARAMETER(unused);
    return PIXEL_MATCH_FILTER_DISPLAY_NAME;
}

static void pixel_match_filter_destroy(void *data)
{
    struct pm_filter_data *filter = data;

    pthread_mutex_lock(&filter->mutex);
    pm_resize_match_entries(filter, 0);
    obs_enter_graphics();
    if (filter->snapshot_texrender)
        gs_texrender_destroy(filter->snapshot_texrender);
    if (filter->snapshot_stagesurface)
        gs_stagesurface_destroy(filter->snapshot_stagesurface);
    if (filter->mask_texrender)
        gs_texrender_destroy(filter->mask_texrender);
    if (filter->mask_stagesurface)
        gs_stagesurface_destroy(filter->mask_stagesurface);
    if (filter->mask_region_texture)
        gs_texture_destroy(filter->mask_region_texture);
    gs_effect_destroy(filter->effect);
    obs_leave_graphics();
    if (filter->captured_region_data)
        bfree(filter->captured_region_data);
    pthread_mutex_unlock(&filter->mutex);
    pthread_mutex_destroy(&filter->mutex);
    bfree(filter);
}

static void *pixel_match_filter_create(
    obs_data_t *settings, obs_source_t *context)
{
    struct pm_filter_data *filter = bzalloc(sizeof(struct pm_filter_data));
    char *effect_path = obs_module_file("pixel_match.effect");
    filter->context = context;

#if 1
    // recursive mutex
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&filter->mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
#else
    // non-recursive mutex
    pthread_mutex_init(&filter->mutex, NULL);
#endif

    //  gfx init
    obs_enter_graphics();
    filter->effect = gs_effect_create_from_file(effect_path, NULL);
    if (!filter->effect)
        goto gfx_fail;
    obs_leave_graphics();

    bfree(effect_path);
    if (!filter->effect)
        goto gfx_fail;

    // init filters and result handles
    filter->param_image = gs_effect_get_param_by_name(
        filter->effect, "image");
    filter->param_match_img = gs_effect_get_param_by_name(
        filter->effect, "match_img");
    filter->param_roi_left =
        gs_effect_get_param_by_name(filter->effect, "roi_left");
    filter->param_roi_bottom =
        gs_effect_get_param_by_name(filter->effect, "roi_bottom");
    filter->param_roi_right =
        gs_effect_get_param_by_name(filter->effect, "roi_right");
    filter->param_roi_top =
        gs_effect_get_param_by_name(filter->effect, "roi_top");

    filter->param_mask_color =
        gs_effect_get_param_by_name(filter->effect, "mask_color");
    filter->param_mask_alpha =
        gs_effect_get_param_by_name(filter->effect, "mask_alpha");
    filter->param_store_match_alpha =
        gs_effect_get_param_by_name(filter->effect, "store_match_alpha");
    filter->param_per_pixel_err_thresh =
        gs_effect_get_param_by_name(filter->effect, "per_pixel_err_thresh");

    filter->param_compare_counter =
        gs_effect_get_param_by_name(filter->effect, "compare_counter");
    filter->param_match_counter =
        gs_effect_get_param_by_name(filter->effect, "match_counter");
    filter->result_compare_counter =
        gs_effect_get_result_by_name(filter->effect, "compare_counter");
    filter->result_match_counter =
        gs_effect_get_result_by_name(filter->effect, "match_counter");

    filter->param_show_color_indicator =
        gs_effect_get_param_by_name(filter->effect, "show_color_indicator");
    filter->param_show_border =
        gs_effect_get_param_by_name(filter->effect, "show_border");
    filter->param_border_px_width =
        gs_effect_get_param_by_name(filter->effect, "border_px_width");
    filter->param_border_px_height =
        gs_effect_get_param_by_name(filter->effect, "border_px_height");


    if (!filter->param_match_img || !filter->param_per_pixel_err_thresh
     || !filter->param_show_color_indicator || !filter->param_show_border
     || !filter->param_match_counter || !filter->result_match_counter
     || !filter->param_compare_counter || !filter->result_compare_counter)
        goto error;

    //obs_source_update(context, settings);
    return filter;

gfx_fail:
    blog(LOG_ERROR, "%s", obs_module_text("filter gfx initialization failed."));
    obs_leave_graphics();

error:
    blog(LOG_ERROR, "%s", obs_module_text("filter initialization failed."));
    pixel_match_filter_destroy(filter);
    pm_filter_failed = true;
    return NULL;

    UNUSED_PARAMETER(settings);
}

void render_select_region(struct pm_filter_data* filter)
{
    // select region mode just displays a selection rectangle
        // TODO: move to a function
    if (!obs_source_process_filter_begin(
        filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
        blog(LOG_ERROR,
            "pm_filter_data: obs_source_process_filter_begin failed.");
        return;
    }

    float roi_left_u
        = (float)(filter->select_left) / (float)(filter->base_width);
    float roi_bottom_v
        = (float)(filter->select_bottom) / (float)(filter->base_height);
    float roi_right_u
        = (float)(filter->select_right + 1) / (float)(filter->base_width);
    float roi_top_v
        = (float)(filter->select_top + 1) / (float)(filter->base_height);

    // these values will be actually relevant to drawing a region selection
    gs_effect_set_float(filter->param_roi_left, roi_left_u);
    gs_effect_set_float(filter->param_roi_bottom, roi_bottom_v);
    gs_effect_set_float(filter->param_roi_right, roi_right_u);
    gs_effect_set_float(filter->param_roi_top, roi_top_v);
    gs_effect_set_bool(filter->param_show_border, true);
    gs_effect_set_bool(filter->param_show_color_indicator, false);
    gs_effect_set_float(filter->param_border_px_width,
        PM_SELECT_REGION_BORDER_THICKNESS / (float)(filter->base_width));
    gs_effect_set_float(filter->param_border_px_height,
        PM_SELECT_REGION_BORDER_THICKNESS / (float)(filter->base_height));

    // the rest are just values stop unassigned value errors
    gs_effect_set_atomic_uint(filter->param_compare_counter, 0);
    gs_effect_set_atomic_uint(filter->param_match_counter, 0);
    gs_effect_set_float(filter->param_per_pixel_err_thresh, 0.f);
    gs_effect_set_bool(filter->param_mask_alpha, false);
    gs_effect_set_bool(filter->param_store_match_alpha, false);
    gs_effect_set_vec3(filter->param_mask_color, &vec3_dummy);
    gs_effect_set_texture(filter->param_match_img, NULL);

    obs_source_process_filter_end(filter->context, filter->effect,
        filter->base_width, filter->base_height);
}

void render_passthrough(struct pm_filter_data* filter)
{
    // no match entries to display; just do passthrough
    gs_effect_t* default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
    if (!obs_source_process_filter_begin(
        filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
        blog(LOG_ERROR,
            "pm_filter_data: obs_source_process_filter_begin failed.");
        return;
    }
    obs_source_process_filter_end(filter->context, default_effect,
        filter->base_width, filter->base_height);
}

void render_match_entries(struct pm_filter_data* filter)
{
    bool nothing_rendered = true;

    for (size_t i = 0; i < filter->num_match_entries; ++i) {
        struct pm_match_entry_data* entry = filter->match_entries + i;

        if (filter->filter_mode == PM_MATCH_VISUALIZE) {
            if (i != filter->selected_match_index) {
                // visualize mode only renders the selected match entry
                continue;
            } else {
                nothing_rendered = false;
            }
        } else {
            if (!entry->cfg.is_enabled) {
                entry->num_compared = 0;
                entry->num_matched = 0;
                // disable entries are skipped in matching
                continue;
            }
        }

        if (entry->match_img_data
            && entry->match_img_width
            && entry->match_img_height) {
            if (entry->match_img_tex)
                gs_texture_destroy(entry->match_img_tex);
            entry->match_img_tex = gs_texture_create(
                entry->match_img_width, entry->match_img_height,
                GS_BGRA, (uint8_t)-1,
                (const uint8_t**)(&entry->match_img_data), 0);
            bfree(entry->match_img_data);
            entry->match_img_data = NULL;
        }

        if (!obs_source_process_filter_begin(
            filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
            blog(LOG_ERROR,
                "pm_filter_data: obs_source_process_filter_begin failed.");
            return;
        }

        float roi_left_u
            = (float)(entry->cfg.roi_left) / (float)(filter->base_width);
        float roi_bottom_v
            = (float)(entry->cfg.roi_bottom) / (float)(filter->base_height);
        float roi_right_u = roi_left_u
            + (float)(entry->match_img_width) / (float)(filter->base_width);
        float roi_top_v = roi_bottom_v
            + (float)(entry->match_img_height) / (float)(filter->base_height);
        bool visualize = (filter->filter_mode == PM_MATCH_VISUALIZE);

        gs_effect_set_atomic_uint(filter->param_compare_counter, 0);
        gs_effect_set_atomic_uint(filter->param_match_counter, 0);
        gs_effect_set_float(filter->param_roi_left, roi_left_u);
        gs_effect_set_float(filter->param_roi_bottom, roi_bottom_v);
        gs_effect_set_float(filter->param_roi_right, roi_right_u);
        gs_effect_set_float(filter->param_roi_top, roi_top_v);
        gs_effect_set_float(filter->param_per_pixel_err_thresh,
            entry->cfg.per_pixel_err_thresh / 100.f);
        gs_effect_set_bool(filter->param_mask_alpha, entry->cfg.mask_alpha);
        gs_effect_set_bool(filter->param_store_match_alpha, false);
        gs_effect_set_vec3(filter->param_mask_color, &entry->cfg.mask_color);
        if (visualize) {
            gs_effect_set_texture(
                filter->param_match_img, entry->match_img_tex);
        } else {
            gs_effect_set_texture_srgb(
                filter->param_match_img, entry->match_img_tex);
        }
        gs_effect_set_bool(filter->param_show_border, visualize);
        gs_effect_set_bool(filter->param_show_color_indicator, visualize);
        gs_effect_set_float(filter->param_border_px_width,
            PM_VISUALIZE_BORDER_THICKNESS / (float)(filter->base_width));
        gs_effect_set_float(filter->param_border_px_height,
            PM_VISUALIZE_BORDER_THICKNESS / (float)(filter->base_height));

        obs_source_process_filter_end(filter->context, filter->effect,
            filter->base_width, filter->base_height);

        if (filter->filter_mode == PM_MATCH) {
            entry->num_compared =
                gs_effect_get_atomic_uint_result(filter->result_compare_counter);
            entry->num_matched =
                gs_effect_get_atomic_uint_result(filter->result_match_counter);
        }
    }

    if (nothing_rendered) {
        render_passthrough(filter);
    }
}

bool stagerender_begin(
    struct pm_filter_data* filter,gs_stagesurf_t** sss, gs_texrender_t** stx)
{
    if (!*stx) {
        *stx = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
    }

    if (*sss) {
        uint32_t width = gs_stagesurface_get_width(*sss);
        uint32_t height = gs_stagesurface_get_height(*sss);
        if (width != filter->base_width
         || height != filter->base_height) {
            gs_stagesurface_destroy(*sss);
            *sss = NULL;
        }
    }
    if (!*sss) {
        *sss = gs_stagesurface_create(
            filter->base_width, filter->base_height, GS_RGBA);
    }

    // https://github.com/synap5e/obs-screenshot-plugin/blob/master/screenshot-filter.c

    gs_texrender_reset(*stx);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

    if (!gs_texrender_begin(*stx, filter->base_width, filter->base_height)) {
        blog(LOG_ERROR, "%s",
            obs_module_text("pm_filter_data: texrender begin failed"));
        return false;
    }

    struct vec4 clear_color;
    vec4_zero(&clear_color);
    gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
    gs_ortho(0.0f, (float)filter->base_width,
        0.0f, (float)filter->base_height, -100.0f, 100.0f);

    return true;
}

void stagerender_end(gs_texrender_t* stx)
{
    gs_texrender_end(stx);

    gs_blend_state_pop();
}

void copy_region_from_stagerender(
    struct pm_filter_data* filter,
    gs_stagesurf_t* sss, gs_texrender_t* stx,
    uint8_t *dstPtr, uint32_t dstStride)
{
    const size_t pixSz = 4;
    size_t height 
        = (size_t)filter->select_top - (size_t)filter->select_bottom + 1;

    gs_texture_t* tex = gs_texrender_get_texture(stx);
    gs_stage_texture(sss, tex);
    uint8_t* stageSurfData;
    uint32_t srcStride;
    if (!gs_stagesurface_map(sss, &stageSurfData, &srcStride)) {
        blog(LOG_ERROR, "pm_filter_data: failed to map stage surface");
        return;
    }

    uint8_t* srcPtr = stageSurfData
        + (size_t)(filter->select_bottom) * srcStride 
        + (size_t)(filter->select_left) * pixSz;
    size_t lineWidth = srcStride < dstStride ? srcStride : dstStride;
    for (size_t i = 0; i < height; ++i) {
        memcpy(dstPtr, srcPtr, lineWidth);
        dstPtr += dstStride;
        srcPtr += srcStride;
    }
    gs_stagesurface_unmap(sss);
}

void snapshot_stagerender(
    struct pm_filter_data* filter, obs_source_t* target, obs_source_t* parent)
{
    stagerender_begin(
        filter, &filter->snapshot_stagesurface, &filter->snapshot_texrender);

    uint32_t parent_flags = obs_source_get_output_flags(target);
    bool custom_draw = (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
    bool async = (parent_flags & OBS_SOURCE_ASYNC) != 0;
    if (target == parent && !custom_draw && !async) {
        obs_source_default_render(target);
    } else {
        obs_source_video_render(target);
    }

    stagerender_end(filter->snapshot_texrender);
}

void capture_region_from_stagerender(
    struct pm_filter_data* filter, gs_stagesurf_t* sss, gs_texrender_t* stx)
{
    const uint32_t pixSz = 4;
    uint32_t width = filter->select_right - filter->select_left + 1;
    uint32_t height = filter->select_top - filter->select_bottom + 1;
    uint8_t* dst_ptr;
    uint32_t dst_stride = width * pixSz;

    if (filter->captured_region_data)
        bfree(filter->captured_region_data);
    filter->captured_region_data 
        = (uint8_t*)bmalloc((size_t)dst_stride * (size_t)height);
    dst_ptr = filter->captured_region_data;

    copy_region_from_stagerender(
        filter, sss, stx,
        dst_ptr, dst_stride);
}

void capture_mask_tex_from_stagerender(
    struct pm_filter_data* filter, gs_stagesurf_t* sss, gs_texrender_t* stx,
    bool reset)
{
    uint32_t width = filter->select_right - filter->select_left + 1;
    uint32_t height = filter->select_top - filter->select_bottom + 1;
    uint8_t* dst_ptr;
    uint32_t dst_stride;

    // mask begin
    if (reset) {
        if (filter->mask_region_texture)
            gs_texture_destroy(filter->mask_region_texture);
        filter->mask_region_texture = NULL;
        filter->mask_region_texture = gs_texture_create(
            (uint32_t)width, (uint32_t)height, GS_RGBA,
            1, NULL, GS_DYNAMIC);
    }
    if (!gs_texture_map(filter->mask_region_texture, &dst_ptr, &dst_stride)) {
        blog(LOG_ERROR, "pm_filter_data: failed to map texture");
        return;
    }

    copy_region_from_stagerender(filter, sss, stx, dst_ptr, dst_stride);

    gs_texture_unmap(filter->mask_region_texture);
}

void configure_mask(struct pm_filter_data* filter)
{
    size_t sel_idx = filter->selected_match_index;
    if (sel_idx >= filter->num_match_entries) return;
    struct pm_match_entry_data* entry = filter->match_entries + sel_idx;
    float match_ratio = entry->cfg.per_pixel_err_thresh / 100.f;

    float roi_left_u
        = (float)(filter->select_left) / (float)(filter->base_width);
    float roi_bottom_v
        = (float)(filter->select_bottom) / (float)(filter->base_height);
    float roi_right_u
        = (float)(filter->select_right + 1) / (float)(filter->base_width);
    float roi_top_v
        = (float)(filter->select_top + 1) / (float)(filter->base_height);

    bool visualize = (filter->filter_mode == PM_MASK_VISUALIZE);

    gs_effect_set_atomic_uint(filter->param_compare_counter, 0);
    gs_effect_set_atomic_uint(filter->param_match_counter, 0);
    gs_effect_set_float(filter->param_roi_left, roi_left_u);
    gs_effect_set_float(filter->param_roi_bottom, roi_bottom_v);
    gs_effect_set_float(filter->param_roi_right, roi_right_u);
    gs_effect_set_float(filter->param_roi_top, roi_top_v);
    gs_effect_set_float(filter->param_per_pixel_err_thresh, match_ratio);
    gs_effect_set_bool(filter->param_mask_alpha, true);
    gs_effect_set_bool(filter->param_store_match_alpha, !visualize);
    gs_effect_set_vec3(filter->param_mask_color, &vec3_dummy);
    gs_effect_set_texture(filter->param_match_img, filter->mask_region_texture);
    gs_effect_set_bool(filter->param_show_border, visualize);
    gs_effect_set_bool(filter->param_show_color_indicator, visualize);
    gs_effect_set_float(filter->param_border_px_width,
        PM_AUTOMASK_BORDER_THICKNESS / (float)(filter->base_width));
    gs_effect_set_float(filter->param_border_px_height,
        PM_AUTOMASK_BORDER_THICKNESS / (float)(filter->base_height));
}

void render_mask_visualization(struct pm_filter_data* filter)
{
    if (!obs_source_process_filter_begin(
        filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
        blog(LOG_ERROR,
            "pm_filter_data: obs_source_process_filter_begin failed.");
        return;
    }
    configure_mask(filter);
    obs_source_process_filter_end(filter->context, filter->effect,
        filter->base_width, filter->base_height);
}

void mask_stagerender(
    struct pm_filter_data* filter, obs_source_t* target, obs_source_t* parent)
{
    // grab a snapshot to use it as input to mask rendering
    snapshot_stagerender(filter, target, parent);
    gs_texture_t* snapshot_texture 
        = gs_texrender_get_texture(filter->snapshot_texrender);

    // prepare and render mask stagerender
    stagerender_begin(
        filter, &filter->mask_stagesurface, &filter->mask_texrender);

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(0.0f, (float)filter->base_width, 0.0f, (float)filter->base_height,
        -100.0f, 100.0f);
    gs_set_viewport(0, 0, (int)filter->base_width, (int)filter->base_height);

    gs_blend_state_push();
    gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);

    struct vec4 clear_color;
    vec4_zero(&clear_color);
    if (filter->filter_mode == PM_MASK_END) {
        size_t selIdx = filter->selected_match_index;
        if (selIdx < filter->num_match_entries) {
            struct pm_match_entry_data* entry = filter->match_entries + selIdx;
            if (!entry->cfg.mask_alpha) {
                // background color for the final mask capture, when appropriate
                vec4_from_vec3(&clear_color, &entry->cfg.mask_color);
            }
        }
    }
    gs_clear(GS_CLEAR_COLOR, &clear_color, .0f, 0);

    gs_effect_set_texture(filter->param_image, snapshot_texture);
    configure_mask(filter);
    while (gs_effect_loop(filter->effect, "Draw")) {
        gs_draw(GS_TRISTRIP, 0, 0);
    }

    gs_blend_state_pop();

    gs_projection_pop();
    gs_viewport_pop();

    stagerender_end(filter->mask_texrender);
}

static void pixel_match_filter_render(void *data, gs_effect_t *effect)
{
    struct pm_filter_data *filter = data;
    obs_source_t *target, *parent;
    enum pm_filter_mode prevMode;

    pthread_mutex_lock(&filter->mutex);
    prevMode = filter->filter_mode;

    target = obs_filter_get_target(filter->context);
    parent = obs_filter_get_parent(filter->context);
    if (target && parent) {
        filter->base_width = obs_source_get_base_width(target);
        filter->base_height = obs_source_get_base_height(target);
    } else {
        filter->base_width = 0;
        filter->base_height = 0;
    }

    if (filter->base_width == 0 || filter->base_height == 0)
        goto done;

    if (filter->filter_mode == PM_SNAPSHOT) {
        snapshot_stagerender(filter, target, parent);
        capture_region_from_stagerender(filter,
            filter->snapshot_stagesurface, filter->snapshot_texrender);
        goto done;
    }

    if (filter->filter_mode == PM_MASK_BEGIN) {
        snapshot_stagerender(filter, target, parent);
        capture_mask_tex_from_stagerender(
            filter, filter->snapshot_stagesurface, filter->snapshot_texrender,
            true);
        goto done;
    }

    if (filter->filter_mode == PM_MASK) {
        mask_stagerender(filter, target, parent);
        capture_mask_tex_from_stagerender(
            filter, filter->mask_stagesurface, filter->mask_texrender,
            false);
        render_passthrough(filter);
        goto done;
    }

    if (filter->filter_mode == PM_MASK_END) {
        mask_stagerender(filter, target, parent);
        capture_region_from_stagerender(filter,
            filter->mask_stagesurface, filter->mask_texrender);
        goto done;
    }

    if (filter->filter_mode == PM_MASK_VISUALIZE) {
        render_mask_visualization(filter);
        goto done;
    }

    if (filter->filter_mode == PM_SELECT_REGION_VISUALIZE) {
        render_select_region(filter);
        goto done;
    }

    render_match_entries(filter);

done:
    if (filter->filter_mode == PM_MATCH_VISUALIZE
     || filter->filter_mode == PM_SELECT_REGION_VISUALIZE)
        filter->filter_mode = PM_MATCH;
    else if (filter->filter_mode == PM_MASK_VISUALIZE
          || filter->filter_mode == PM_MASK_BEGIN)
        filter->filter_mode = PM_MASK;
    pthread_mutex_unlock(&filter->mutex);

    if (filter->on_match_image_captured
     && (prevMode == PM_SNAPSHOT || prevMode == PM_MASK_END)) {
        filter->on_match_image_captured(filter);
    }

    if ((prevMode == PM_MASK 
      || (prevMode == PM_MATCH && filter->num_match_entries > 0))
     && filter->on_frame_processed) {
        filter->on_frame_processed(filter);
    }

    UNUSED_PARAMETER(effect);
}

static uint32_t pixel_match_filter_width(void *data)
{
    struct pm_filter_data *filter = data;
    obs_source_t *parent = obs_filter_get_parent(filter->context);
    return obs_source_get_base_width(parent);
}

static uint32_t pixel_match_filter_height(void *data)
{
    struct pm_filter_data *filter = data;
    obs_source_t *parent = obs_filter_get_parent(filter->context);
    return obs_source_get_base_height(parent);
}

bool settings_button_callback(
    obs_properties_t *props, obs_property_t *property, void *data)
{
    struct pm_filter_data *filter = (struct pm_filter_data *)data;
    if (filter && filter->on_settings_button_released) 
        filter->on_settings_button_released();
    return true;

    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
}

static obs_properties_t* pixel_match_filter_properties(void* data)
{
    //struct pm_filter_data *filter = (struct pm_filter_data*)data;

    obs_properties_t* props = obs_properties_create();

    obs_properties_add_button(props, "settings_button",
        obs_module_text("Open Settings"), settings_button_callback);

#if 0
    obs_properties_add_int(properties,
        "roi_left", obs_module_text("Roi Left"),
        0, 10000, 1);
    obs_properties_add_int(properties,
        "roi_bottom", obs_module_text("Roi Bottom"),
        0, 10000, 1);
    obs_properties_add_int(properties,
        "roi_right", obs_module_text("Roi Right"),
        0, 10000, 1);
    obs_properties_add_int(properties,
        "roi_top", obs_module_text("Roi Top"),
        0, 10000, 1);

    obs_properties_add_int(properties,
        "per_pixel_err_thresh",
        obs_module_text("Per-Pixel Error Threshold, %"),
        0, 100, 1);
    obs_properties_add_int(properties,
        "total_match_thresh",
        obs_module_text("Total Match Threshold, %"),
        0, 100, 1);

    obs_properties_add_int(properties,
        "num_matched",
        obs_module_text("Number Matched:"),
        0, 65536, 1);

    //obs_property_set_modified_callback(
    //    p, pixel_match_prop_changed_callback);

#endif

    return props;

    UNUSED_PARAMETER(data);
}

struct obs_source_info pixel_match_filter = {
    .id = PIXEL_MATCH_FILTER_ID,
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = pixel_match_filter_get_name,
    .create = pixel_match_filter_create,
    .destroy = pixel_match_filter_destroy,
    //.update = pixel_match_filter_update,
    .get_properties = pixel_match_filter_properties,
    //.get_defaults = pixel_match_filter_defaults,
    //.video_tick = pixel_match_filter_tick,
    .video_render = pixel_match_filter_render,
    .get_width = pixel_match_filter_width,
    .get_height = pixel_match_filter_height,
};

//---------------------------------------

void pm_destroy_match_gfx(struct gs_texture *tex, void *img_data)
{
    if (tex) {
        obs_enter_graphics();
        gs_texture_destroy(tex);
        obs_leave_graphics();
    }
    if (img_data) {
        bfree(img_data);
    }
}

void pm_supply_match_entry_config(struct pm_filter_data *filter,
    size_t match_idx, const struct pm_match_entry_config *cfg)
{
    pthread_mutex_lock(&filter->mutex);
    if (match_idx >= filter->num_match_entries) {
        pthread_mutex_unlock(&filter->mutex);
        return;
    }

    struct pm_match_entry_data *entry = filter->match_entries + match_idx;
    memcpy(&entry->cfg, cfg, sizeof(struct pm_match_entry_config));
    pthread_mutex_unlock(&filter->mutex);
}

void pm_resize_match_entries(struct pm_filter_data *filter, size_t new_size)
{
    pthread_mutex_lock(&filter->mutex);
    if (new_size == filter->num_match_entries) {
        pthread_mutex_unlock(&filter->mutex);
        return;
    }

    size_t old_size = filter->num_match_entries;
    struct pm_match_entry_data *old_entries = filter->match_entries;
    if (new_size > 0) {
        filter->match_entries = (struct pm_match_entry_data *)bmalloc(
            sizeof(struct pm_match_entry_data) * new_size);
        for (size_t i = 0; i < new_size; ++i) {
            if (i < old_size) {
                memcpy((void *)(filter->match_entries + i),
                       (void *)(old_entries + i),
                       sizeof(struct pm_match_entry_data));
            } else {
                memset((void *)(filter->match_entries + i), 0,
                       sizeof(struct pm_match_entry_data));
            }
        }
    } else {
        filter->match_entries = NULL;
    }
    filter->num_match_entries = new_size;
    pthread_mutex_unlock(&filter->mutex);

    for (size_t i = new_size; i < old_size; i++) {
        struct pm_match_entry_data *old_entry = old_entries + i;
        pm_destroy_match_gfx(old_entry->match_img_tex,
                     old_entry->match_img_data);
    }
    if (old_entries)
        bfree(old_entries);
}


#if 0
    // passthrough
    obs_source_skip_video_filter(filter->context);

    // adjust structres
    if (target && parent) {
        cx = obs_source_get_base_width(target);
        cy = obs_source_get_base_height(target);
    }

    // frame capture
    if (cx > 0 && cy > 0) {
        uint32_t parent_flags = obs_source_get_output_flags(target);
        bool custom_draw = (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
        bool async = (parent_flags & OBS_SOURCE_ASYNC) != 0;

        gs_texrender_reset(filter->tex_render);
        gs_blend_state_push();
        gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

        gs_texrender_begin(filter->tex_render, filter->cx, filter->cy);
        struct vec4 clear_color;
        vec4_zero(&clear_color);
        gs_clear(GS_CLEAR_COLOR, &clear_color, .0f, 0);
        gs_ortho(0.0f, (float)filter->cx, 0.0f, (float)filter->cy,
             -100.0f, 100.0f);

        if (target == parent && !custom_draw && !async)
        obs_source_default_render(target);
        else
        obs_source_video_render(target);

        gs_texrender_end(filter->tex_render);
        gs_blend_state_pop();

        gs_texture_t *texture = gs_texrender_get_texture(filter->tex_render);
        if (texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->texture);
        glGetTexImage(GL_TEXTURE_2D,
                  0,
                  GL_RGBA,
                  GL_UNSIGNED_BYTE,
                  filter->pixel_data);
        //printf("reading stuff!\n");
        filter->frame_wanted = false;
        filter->frame_available = true;
        }
    }
#endif

#if 0
    static void pixel_match_filter_update(void* data, obs_data_t* settings)
    {
        struct pm_filter_data* filter = data;

        pthread_mutex_lock(&filter->mutex);
        filter->roi_left = (int)obs_data_get_int(settings, "roi_left");
        filter->roi_bottom = (int)obs_data_get_int(settings, "roi_bottom");
        filter->per_pixel_err_thresh
            = (int)obs_data_get_int(settings, "per_pixel_err_thresh");
        filter->total_match_thresh
            = (int)obs_data_get_int(settings, "total_match_thresh");
        // TODO mask image
        // TODO scenes
        pthread_mutex_unlock(&filter->mutex);

        UNUSED_PARAMETER(data);
    }
#endif

#if 0
    static bool pixel_match_prop_changed_callback(
        obs_properties_t* props, obs_property_t* p, obs_data_t* settings)
    {
        UNUSED_PARAMETER(props);
        UNUSED_PARAMETER(p);
        UNUSED_PARAMETER(settings);
        return true;
    }
#endif

#if 0
    static void pixel_match_filter_tick(void* data, float seconds)
    {
        struct pixel_match_filter_data* filter = data;
        UNUSED_PARAMETER(seconds);
        UNUSED_PARAMETER(filter);
        // TODO: dispatch pixel processing every
    }
#endif

#if 0
static void pixel_match_filter_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings, "per_pixel_err_thresh", 10);
    obs_data_set_default_int(settings, "total_match_thresh", 90);
}
#endif
