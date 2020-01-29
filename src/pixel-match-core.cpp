#include "pixel-match-core.hpp"
#include "pixel-match-dialog.hpp"
#include "pixel-match-filter.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>

#include <QAction>
#include <QMainWindow>

#include <iostream> // debugging

PixelMatcher* PixelMatcher::m_instance = nullptr;

void init_pixel_match_switcher()
{
    PixelMatcher::m_instance = new PixelMatcher();
}

void free_pixel_match_switcher()
{
    delete PixelMatcher::m_instance;
    PixelMatcher::m_instance = nullptr;
}

//------------------------------------

PixelMatcher::PixelMatcher()
: m_filter()
{
    auto mainWindow = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    m_dialog = new PixelMatchDialog(this, mainWindow);

    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction(
            obs_module_text("Pixel Match Switcher")));
    connect(action, &QAction::triggered, m_dialog, &QDialog::exec);
}

void PixelMatcher::findFilter()
{
    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);

    using namespace std;
    //cout << "--------- SCENES ------------------\n";
    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto src = scenes.sources.array[i];
        cout << obs_source_get_name(src) << endl;

        auto scene = obs_scene_from_source(src);
        obs_scene_enum_items(
            scene,
            [](obs_scene_t*, obs_scene_item *item, void*) {
                cout << "  scene item id: " << obs_sceneitem_get_id(item) << endl;
                auto itemSrc = obs_sceneitem_get_source(item);
                cout << "    src name: " << obs_source_get_name(itemSrc) << endl;

                obs_source_enum_filters(
                    itemSrc,
                    [](obs_source_t *, obs_source_t *filter, void *) {
                        auto name = obs_source_get_name(filter);
                        cout << "      "
                             << "filter id = \"" << obs_source_get_id(filter)
                             << "\", name = \"" << obs_source_get_name(filter)
                             << "\"" << endl;
                    },
                    nullptr
                );

                return true;
            },
            nullptr
        );

#if 0
        obs_source_enum_filters(
            src,
            [](obs_source_t *, obs_source_t *filter, void *) {
                auto name = obs_source_get_name(filter);
                cout << name << endl;
            },
            nullptr
        );
#endif
        //auto scene = obs_scene_from_source(src);

        //auto sceneSrc = obs_scene_get_source(scene);
        //cout << obs_source_get_name(sceneSrc) << endl;
#if 0
        for (size_t i = 0; i < src->filters.num; i++) {
            obs_source_t *filter =
                //obs_source_get_filter_by_name(sceneSrc, PIXEL_MATCH_FILTER_DISPLAY_NAME);
                src->filters.array[i];
            if (filter) {
                cout << "filter: " << filter->info.id << endl;
                m_filter = filter;
                break;
            }
        }
#endif
    }
    //cout << "------------------------------------\n";

}

