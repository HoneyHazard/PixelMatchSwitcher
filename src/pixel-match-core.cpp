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
#include <QApplication>
#include <QTimer>

//------------------------------------

PixelMatcher* PixelMatcher::m_instance = nullptr;

void init_pixel_match_switcher()
{
    PixelMatcher::m_instance = new PixelMatcher();
}

void free_pixel_match_switcher()
{
    PixelMatcher::m_instance->deleteLater();
    PixelMatcher::m_instance = nullptr;
}

//------------------------------------

PixelMatcher::PixelMatcher()
{
    // parent this to the app
    setParent(qApp);

    // parent the dialog to the main window
    //auto mainWindow = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    m_dialog = new PixelMatchDialog(this, nullptr);

    // add action item in the Tools menu of the app
    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction(
            obs_module_text("Pixel Match Switcher")));
    connect(action, &QAction::triggered, m_dialog, &QDialog::show);

    // periodic update timer
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout,
            this, &PixelMatcher::periodicUpdate);
    timer->start(100);
}

std::set<OBSWeakSource> PixelMatcher::filters() const
{
    QMutexLocker locker(&m_mutex);
    return m_filters;
}

OBSWeakSource PixelMatcher::activeFilter() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeFilter;
}

std::string PixelMatcher::scenesInfo()
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
    oss << "-----------------------------------\n";
    return oss.str();
}

void PixelMatcher::findFilters()
{
    using namespace std;
    QMutexLocker locker(&m_mutex);

    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);
    m_filters.clear();

    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto sceneSrc = scenes.sources.array[i];
        auto scene = obs_scene_from_source(sceneSrc);
        obs_scene_enum_items(
            scene,
            [](obs_scene_t*, obs_scene_item *item, void* p) {
                auto &filters = *static_cast<set<OBSWeakSource>*>(p);
                auto itemSrc = obs_sceneitem_get_source(item);
                obs_source_enum_filters(
                    itemSrc,
                    [](obs_source_t *, obs_source_t *filter, void *p) {
                        auto &filters = *static_cast<set<OBSWeakSource>*>(p);
                        auto id = obs_source_get_id(filter);
                        if (!strcmp(id, PIXEL_MATCH_FILTER_ID)) {
                            auto weakRef = obs_source_get_weak_source(filter);
                            filters.insert(OBSWeakSource(weakRef));
                        }
                    },
                    &filters
                );
                return true;
            },
            &m_filters
        );
    }
}

void PixelMatcher::periodicUpdate()
{
    findFilters();

    QMutexLocker locker(&m_mutex);
    if (!m_activeFilter) {
        if (m_filters.size()) {
            m_activeFilter = *(m_filters.begin());
        }
    } else {
        if (m_filters.find(m_activeFilter) == m_filters.end()) {
            m_activeFilter = nullptr;
        }
    }
}

