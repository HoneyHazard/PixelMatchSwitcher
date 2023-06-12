#pragma once

#include <obs-module.h>

#define PIXEL_MATCH_FILTER_ID "pixel_match_filter"

#ifndef __cplusplus
extern void init_pixel_match_switcher(void);
extern void free_pixel_match_switcher(void);
#endif

#ifdef __cplusplus
extern "C" lookup_t *obs_module_lookup;
extern "C" const char *obs_module_text(const char *val);
extern "C" bool obs_module_get_string(const char *val, const char **out);
extern "C" void obs_module_set_locale(const char *locale);
extern "C" void obs_module_free_locale(void);
#else
lookup_t *obs_module_lookup;
const char *obs_module_text(const char *val);
bool obs_module_get_string(const char *val, const char **out);
void obs_module_set_locale(const char *locale);
void obs_module_free_locale(void);
#endif
