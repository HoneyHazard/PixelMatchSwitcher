#include "pm-filter.h"

#include <graphics/graphics.h>

static const char *pixel_match_filter_get_name(void* unused)
{
    UNUSED_PARAMETER(unused);
    return PIXEL_MATCH_FILTER_DISPLAY_NAME;
}

static void pixel_match_filter_destroy(void *data)
{
    struct pm_filter_data *filter = data;

    pthread_mutex_lock(&filter->mutex);
    pm_destroy_match_entries(filter);
    obs_enter_graphics();
    gs_effect_destroy(filter->effect);
    obs_leave_graphics();
    pthread_mutex_unlock(&filter->mutex);
    pthread_mutex_destroy(&filter->mutex);
    bfree(filter);
}

static void *pixel_match_filter_create(
    obs_data_t *settings, obs_source_t *context)
{
    struct pm_filter_data *filter = bzalloc(sizeof(*filter));
    char *effect_path = obs_module_file("pixel_match.effect");

    memset(filter, 0, sizeof(struct pm_filter_data));
    filter->context = context;
    filter->settings = settings;
    filter->visualize = true;

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

    // init filters and result handles
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

    filter->param_visualize =
        gs_effect_get_param_by_name(filter->effect, "visualize");
    filter->param_show_border =
        gs_effect_get_param_by_name(filter->effect, "show_border");
    filter->param_px_width =
        gs_effect_get_param_by_name(filter->effect, "px_width");
    filter->param_px_height =
        gs_effect_get_param_by_name(filter->effect, "px_height");


    if (!filter->param_match_img || !filter->param_per_pixel_err_thresh
     || !filter->param_visualize || !filter->param_show_border
     || !filter->param_match_counter || !filter->result_match_counter
     || !filter->param_compare_counter || !filter->result_compare_counter)
        goto error;

    obs_source_update(context, settings);
    return filter;

gfx_fail:
    blog(LOG_ERROR, "%s", obs_module_text("filter gfx initialization failed."));
    obs_leave_graphics();

error:
    blog(LOG_ERROR, "%s", obs_module_text("filter initialization failed."));
    pixel_match_filter_destroy(filter);
    return NULL;
}

#if 0
static void pixel_match_filter_update(void *data, obs_data_t *settings)
{
    struct pm_filter_data *filter = data;

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
    obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(p);
    UNUSED_PARAMETER(settings);
    return true;
}
#endif

#if 0
static obs_properties_t *pixel_match_filter_properties(void *data)
{
    //struct pm_filter_data *filter
    //    = (struct pm_filter_data*)data;

    obs_properties_t *properties = obs_properties_create();

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

    // TODO scenes
    // TODO mask image
    return properties;

    UNUSED_PARAMETER(data);
}
#endif

static void pixel_match_filter_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings, "per_pixel_err_thresh", 10);
    obs_data_set_default_int(settings, "total_match_thresh", 90);
}

#if 0
static void pixel_match_filter_tick(void *data, float seconds)
{
    struct pixel_match_filter_data *filter = data;
    UNUSED_PARAMETER(seconds);
    UNUSED_PARAMETER(filter);
    // TODO: dispatch pixel processing every
}
#endif

static void pixel_match_filter_render(void *data, gs_effect_t *effect)
{
    struct pm_filter_data *filter = data;
    obs_source_t *target, *parent;

    pthread_mutex_lock(&filter->mutex);

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

    for (size_t i = 0; i < filter->num_match_entries; ++i) {       
        struct pm_match_entry_data* entry = filter->match_entries + i;

        if (filter->preview_mode && i != filter->selected_match_index)
             continue;

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
            goto done;
        }

        float roi_left_u 
            = (float)(entry->cfg.roi_left) / (float)(filter->base_width);
        float roi_bottom_v 
            = (float)(entry->cfg.roi_bottom) / (float)(filter->base_height);
        float roi_right_u = roi_left_u
            + (float)(entry->match_img_width) / (float)(filter->base_width);
        float roi_top_v = roi_bottom_v
            + (float)(entry->match_img_height) / (float)(filter->base_height);

        gs_effect_set_atomic_uint(filter->param_compare_counter, 0);
        gs_effect_set_atomic_uint(filter->param_match_counter, 0);
        gs_effect_set_float(filter->param_roi_left, roi_left_u);
        gs_effect_set_float(filter->param_roi_bottom, roi_bottom_v);
        gs_effect_set_float(filter->param_roi_right, roi_right_u);
        gs_effect_set_float(filter->param_roi_top, roi_top_v);
        gs_effect_set_float(filter->param_per_pixel_err_thresh,
            entry->cfg.per_pixel_err_thresh / 100.f);
        gs_effect_set_bool(filter->param_mask_alpha, entry->cfg.mask_alpha);
        gs_effect_set_vec3(filter->param_mask_color, &entry->cfg.mask_color);
        gs_effect_set_texture(filter->param_match_img, entry->match_img_tex);

        gs_effect_set_bool(filter->param_visualize,
            filter->visualize && filter->preview_mode);
        gs_effect_set_bool(filter->param_show_border, filter->show_border);
        gs_effect_set_float(filter->param_px_width,
            1.f / (float)(filter->base_width));
        gs_effect_set_float(filter->param_px_height,
            1.f / (float)(filter->base_height));

        obs_source_process_filter_end(filter->context, filter->effect,
            filter->base_width, filter->base_height);

        if (!filter->preview_mode) {
            entry->num_compared =
                gs_effect_get_atomic_uint_result(filter->result_compare_counter);
            entry->num_matched =
                gs_effect_get_atomic_uint_result(filter->result_match_counter);
        }
    }

done:
    pthread_mutex_unlock(&filter->mutex);

    if (!filter->preview_mode && filter->on_frame_processed)
        filter->on_frame_processed(filter);
    filter->preview_mode = false;
    filter->show_border = false;

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

struct obs_source_info pixel_match_filter = {
    .id = PIXEL_MATCH_FILTER_ID,
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = pixel_match_filter_get_name,
    .create = pixel_match_filter_create,
    .destroy = pixel_match_filter_destroy,
    //.update = pixel_match_filter_update,
    //.get_properties = pixel_match_filter_properties,
    .get_defaults = pixel_match_filter_defaults,
    //.video_tick = pixel_match_filter_tick,
    .video_render = pixel_match_filter_render,
    .get_width = pixel_match_filter_width,
    .get_height = pixel_match_filter_height,
};

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
