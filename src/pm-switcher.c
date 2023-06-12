#include "pm-module.h"

extern struct obs_source_info pixel_match_filter;

bool obs_module_load(void)
{
    obs_register_source(&pixel_match_filter);
    init_pixel_match_switcher();
    return true;
}

void obs_module_unload(void)
{
    free_pixel_match_switcher();
}
