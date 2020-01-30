#include "pixel-match-filter.h"
//#include

static const char *pixel_match_filter_get_name(void* unused)
{
        UNUSED_PARAMETER(unused);
        return PIXEL_MATCH_FILTER_DISPLAY_NAME;
}

static void *pixel_match_filter_create(
        obs_data_t *settings, obs_source_t *context)
{
        struct pixel_match_filter_data *filter = bzalloc(sizeof(*filter));
        // char *effect_path = obs_module_file("pixel_match_filter.effect");

        filter->context = context;
        filter->frame_wanted = false;
        filter->frame_available = false;
        filter->tex_render = gs_texrender_create(GS_RGBA, GS_ZS_NONE);

        //obs_enter_graphics();
        //filter->effect = gs_effect_create_from_file(effect_path, NULL);
        //obs_leave_graphics();
        // bfree(effect_path)
        // if (!filter->effect) {
        //         bfree(filter);
        //         return NULL;
        // }

        obs_source_update(context, settings);
        return filter;
}

static void pixel_match_filter_destroy(void *data)
{
        struct pixel_match_filter_data *filter = data;

        if (filter->tex_render) {
            gs_texrender_destroy(filter->tex_render);
        }
        //obs_enter_graphics();
        //gs_effect_destroy(filter->effect);
        //obs_leave_graphics();

        bfree(filter);
}

static void pixel_match_filter_update(void *data, obs_data_t *settings)
{
        struct pixel_match_filter_data *filter = data;

        filter->enabled = obs_data_get_bool(settings, "enabled");
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
        obs_properties_t *props = obs_properties_create();

        obs_property_t *p = obs_properties_add_bool(
                props, "enabled", obs_module_text("Enabled"));

        obs_properties_add_int(props,
                "roi_left", obs_module_text("RoiLeft"),
                0, 10000, 1);
        obs_properties_add_int(props,
                "roi_bottom", obs_module_text("RoiBottom"),
                0, 10000, 1);
        obs_properties_add_int(props,
                "roi_right", obs_module_text("RoiRight"),
                0, 10000, 1);
        obs_properties_add_int(props,
                "roi_top", obs_module_text("RoiTop"),
                0, 10000, 1);

        obs_properties_add_int(props,
                "per_pixel_err_thresh",
                obs_module_text("PerPixelErrorThreshold"),
                0, 100, 1);
        obs_properties_add_int(props,
                "total_match_thresh",
                obs_module_text("TotalMatchThreshold"),
                0, 100, 1);

        obs_property_set_modified_callback(
                p, pixel_match_prop_changed_callback);

        // TODO scenes
        // TODO mask image

        UNUSED_PARAMETER(data);
        return props;
}

static void pixel_match_filter_defaults(obs_data_t *settings)
{
        obs_data_set_default_bool(settings, "enabled", false);
        obs_data_set_default_int(settings, "per_pixel_err_thresh", 10);
        obs_data_set_default_int(settings, "total_match_thresh", 90);
}

static void pixel_match_filter_tick(void *data, float seconds)
{
        struct pixel_match_filter_data *filter = data;
        UNUSED_PARAMETER(seconds);
        UNUSED_PARAMETER(filter);
        // TODO: dispatch pixel processing every 100ms?
}

static void pixel_match_filter_render(void *data, gs_effect_t *effect)
{
        struct pixel_match_filter_data *filter = data;              

        // passthrough
        obs_source_skip_video_filter(filter->context);

        if (filter->frame_wanted && !filter->frame_available) {
            obs_source_t *target = obs_filter_get_target(filter->context);
            obs_source_t *parent = obs_filter_get_parent(filter->context);

            filter->cx = obs_source_get_base_width(target);
            filter->cy = obs_source_get_base_height(target);
            uint32_t parent_flags = obs_source_get_output_flags(target);
            bool custom_draw = (parent_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
            bool async = (parent_flags & OBS_SOURCE_ASYNC) != 0;

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

            //filter->tex_id = filter->tex_render->
            filter->frame_wanted = false;
            filter->frame_available = true;
        }

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
        .video_tick = pixel_match_filter_tick,
        .video_render = pixel_match_filter_render,
        .get_width = pixel_match_filter_width,
        .get_height = pixel_match_filter_height,
};
