#include <obs-module.h>

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE("cas-plugin", "en-US");

extern struct cas_filter_data cas_filter;

bool obs_module_load(void)
{
    obs_register_source(&cas_filter);
    return true;
}
