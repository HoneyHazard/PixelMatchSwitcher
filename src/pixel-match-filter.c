#include "pixel-match-filter.h"

#include <graphics/graphics.h>

static const char *pixel_match_filter_get_name(void* unused)
{
    UNUSED_PARAMETER(unused);
    return PIXEL_MATCH_FILTER_DISPLAY_NAME;
}

static void *pixel_match_filter_create(
    obs_data_t *settings, obs_source_t *context)
{
    struct pixel_match_filter_data *filter = bzalloc(sizeof(*filter));
    char *effect_path = obs_module_file("pixel_match.effect");

    memset(filter, 0, sizeof(struct pixel_match_filter_data));
    filter->context = context;
    filter->settings = settings;

    pthread_mutex_init(&filter->mutex, NULL);

    bfree(effect_path);

    if (!filter->effect) {
        bfree(filter);
        return NULL;
    }

    // will be made dynamic
    gs_image_file_init(&filter->match_file, "~/Documents/mask2.png");

    //  gfx init
    obs_enter_graphics();
    filter->effect = gs_effect_create_from_file(effect_path, NULL);
    gs_image_file_init_texture(&filter->match_file);
    obs_leave_graphics();

    // init filters and result handles
    filter->param_per_pixel_err_thresh =
        gs_effect_get_param_by_name(filter->effect,
                                    "per_pixel_err_thresh");

    filter->param_debug = gs_effect_get_param_by_name(filter->effect, "debug");
    filter->param_match_counter =
        gs_effect_get_param_by_name(filter->effect, "match_counter");
    filter->result_match_counter =
        gs_effect_get_result_by_name(filter->effect, "match_counter");

    obs_source_update(context, settings);
    return filter;
}

static void pixel_match_filter_destroy(void *data)
{
    struct pixel_match_filter_data *filter = data;

    pthread_mutex_lock(&filter->mutex);
    obs_enter_graphics();
    gs_effect_destroy(filter->effect);
    obs_leave_graphics();
    gs_image_file_free(&filter->match_file);
    pthread_mutex_unlock(&filter->mutex);

    pthread_mutex_destroy(&filter->mutex);

    bfree(filter);
}

static void pixel_match_filter_update(void *data, obs_data_t *settings)
{
    struct pixel_match_filter_data *filter = data;

    pthread_mutex_lock(&filter->mutex);
    filter->roi_left = (int)obs_data_get_int(settings, "roi_left");
    filter->roi_bottom = (int)obs_data_get_int(settings, "roi_bottom");
    filter->roi_right = (int)obs_data_get_int(settings, "roi_right");
    filter->roi_top = (int)obs_data_get_int(settings, "roi_top");
    filter->per_pixel_err_thresh
        = (int)obs_data_get_int(settings, "per_pixel_err_thresh");
    filter->total_match_thresh
        = (int)obs_data_get_int(settings, "total_match_thresh");
    // TODO mask image
    // TODO scenes
    pthread_mutex_unlock(&filter->mutex);

    UNUSED_PARAMETER(data);
}

static bool pixel_match_prop_changed_callback(
    obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(p);
    UNUSED_PARAMETER(settings);
    return true;
}

static obs_properties_t *pixel_match_filter_properties(void *data)
{
    struct pixel_match_filter_data *filter
        = (struct pixel_match_filter_data*)data;

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
}

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
    struct pixel_match_filter_data *filter = data;

    pthread_mutex_lock(&filter->mutex);
    obs_source_t *target = obs_filter_get_target(filter->context);
    obs_source_t *parent = obs_filter_get_parent(filter->context);
    if (target && parent) {
        filter->cx = obs_source_get_base_width(target);
        filter->cy = obs_source_get_base_height(target);
    }

    if (!obs_source_process_filter_begin(
            filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING))
        return;

    gs_effect_set_atomic_uint(filter->param_match_counter, 0);
    gs_effect_set_int(filter->param_per_pixel_err_thresh,
                      filter->per_pixel_err_thresh);

    //bool master_render = (active_gfx == filter->main_gfx);
    gs_effect_set_bool(filter->param_debug, filter->debug);

    obs_source_process_filter_end(filter->context, filter->effect,
                                  filter->cx, filter->cy);
    filter->num_matched =
        gs_effect_get_atomic_uint_result(filter->result_match_counter);

    obs_data_set_int(filter->settings, "num_matched", filter->num_matched);
    pthread_mutex_unlock(&filter->mutex);

#if 0
    // passthrough
    obs_source_skip_video_filter(filter->context);

    // adjust structres
    if (target && parent) {
        cx = obs_source_get_base_width(target);
        cy = obs_source_get_base_height(target);
    }

    // frame capture
    if (cx > 0 && cy > 0 && filter->frame_wanted && !filter->frame_available) {        uint32_t parent_flags = obs_source_get_output_flags(target);
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

    UNUSED_PARAMETER(effect);
    return;
}

static uint32_t pixel_match_filter_width(void *data)
{
    struct pixel_match_filter_data *filter = data;
    obs_source_t *parent = obs_filter_get_parent(filter->context);
    return obs_source_get_base_width(parent);
}

static uint32_t pixel_match_filter_height(void *data)
{
    struct pixel_match_filter_data *filter = data;
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
    .update = pixel_match_filter_update,
    .get_properties = pixel_match_filter_properties,
    .get_defaults = pixel_match_filter_defaults,
    //.video_tick = pixel_match_filter_tick,
    .video_render = pixel_match_filter_render,
    .get_width = pixel_match_filter_width,
    .get_height = pixel_match_filter_height,
};
