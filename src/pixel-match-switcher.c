#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("pixel-match-plugin", "en-US");

extern struct pixel_match_filter_data pixel_match_filter;

bool obs_module_load(void)
{
    obs_register_source(&pixel_match_filter);
    return true;
}
