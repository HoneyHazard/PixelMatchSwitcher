#include "pixel-match-core.hpp"
#include "pixel-match-dialog.hpp"
#include "pixel-match-filter.h"

#include <ostream>
#include <sstream>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>

#include <QAction>
#include <QMainWindow>


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
{
    auto mainWindow = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    m_dialog = new PixelMatchDialog(this, mainWindow);

    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction(
            obs_module_text("Pixel Match Switcher")));
    connect(action, &QAction::triggered, m_dialog, &QDialog::exec);
}

std::string PixelMatcher::enumSceneElements()
{
    using namespace std;
    ostringstream oss;

    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);

    oss << "--------- SCENES ------------------\n";
    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto src = scenes.sources.array[i];
        oss << obs_source_get_name(src) << endl;

        auto scene = obs_scene_from_source(src);
        obs_scene_enum_items(
            scene,
            [](obs_scene_t*, obs_scene_item *item, void* p) {
                ostringstream &oss = *static_cast<ostringstream*>(p);
                oss << "  scene item id: "
                    << obs_sceneitem_get_id(item) << endl;
                auto itemSrc = obs_sceneitem_get_source(item);
                oss << "    src name: "
                    << obs_source_get_name(itemSrc) << endl;

                obs_source_enum_filters(
                    itemSrc,
                    [](obs_source_t *, obs_source_t *filter, void *p) {
                    ostringstream &oss = *static_cast<ostringstream*>(p);
                    oss << "      "
                         << "filter id = \"" << obs_source_get_id(filter)
                         << "\", name = \"" << obs_source_get_name(filter)
                         << "\"" << endl;
                    },
                    &oss
                );

                return true;
            },
            &oss
        );
    }
    return oss.str();
}

void PixelMatcher::findFilters()
{
    using namespace std;
    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);
    m_filters.clear();

    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto src = scenes.sources.array[i];

        auto scene = obs_scene_from_source(src);
        obs_scene_enum_items(
            scene,
            [](obs_scene_t*, obs_scene_item *item, void* p) {
                auto &filters = *static_cast<vector<obs_source_t*>*>(p);
                auto itemSrc = obs_sceneitem_get_source(item);
                auto filter =
                    obs_source_get_filter_by_name(itemSrc,
                        PIXEL_MATCH_FILTER_DISPLAY_NAME);
                if (filter) {
                    filters.push_back(filter);
                }
                return true;
            },
            &m_filters
        );

    }
}

