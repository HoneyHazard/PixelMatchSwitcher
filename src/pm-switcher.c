#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("pixel-match-switcher", "en-US")

extern struct obs_source_info pixel_match_filter;

extern void init_pixel_match_switcher(void);
extern void free_pixel_match_switcher(void);

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
