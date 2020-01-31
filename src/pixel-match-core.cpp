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
    QTimer *periodicUpdateTimer = new QTimer(this);
    connect(periodicUpdateTimer, &QTimer::timeout,
            this, &PixelMatcher::periodicUpdate);
    periodicUpdateTimer->start(100);

    // timer to fetch rendered frames
    QTimer *checkFrameTimer = new QTimer(this);
    connect(checkFrameTimer, &QTimer::timeout,
            this, &PixelMatcher::checkFrame);
    checkFrameTimer->start(10);
}

std::vector<PixelMatchFilterInfo> PixelMatcher::filters() const
{
    QMutexLocker locker(&m_mutex);
    return m_filters;
}

PixelMatchFilterInfo PixelMatcher::activeFilterInfo() const
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
    m_filters.push_back(PixelMatchFilterInfo());

    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto sceneSrc = scenes.sources.array[i];
        auto scene = obs_scene_from_source(sceneSrc);
        auto &fi = m_filters.back();
        fi.scene = obs_source_get_name(sceneSrc);
        obs_scene_enum_items(
            scene,
            [](obs_scene_t*, obs_scene_item *item, void* p) {
                auto &filters = *static_cast<vector<PixelMatchFilterInfo>*>(p);
                auto &fi = filters.back();
                auto itemSrc = obs_sceneitem_get_source(item);
                fi.sceneItem = obs_source_get_name(itemSrc);
                obs_source_enum_filters(
                    itemSrc,
                    [](obs_source_t *, obs_source_t *filter, void *p) {
                    auto &filters = *static_cast<vector<PixelMatchFilterInfo>*>(p);
                        auto id = obs_source_get_id(filter);
                        if (!strcmp(id, PIXEL_MATCH_FILTER_ID)) {
                            auto copy = filters.back();
                            filters.back().filter = filter;
                            filters.push_back(copy);
                        }
                    },
                    &filters
                );
                return true;
            },
            &m_filters
        );
    }
    m_filters.pop_back();
}

void PixelMatcher::unsetActiveFilter()
{
    m_activeFilter.filter = nullptr;
    m_filterData = nullptr;
}

void PixelMatcher::setActiveFilter(const PixelMatchFilterInfo &fi)
{
    m_activeFilter = fi;
    m_filterData = static_cast<pixel_match_filter_data*>(
        obs_obj_get_data(m_activeFilter.filter));

}

void PixelMatcher::updateActiveFilter()
{
    QMutexLocker locker(&m_mutex);
    if (!m_activeFilter.filter) {
        if (m_filters.size()) {
            setActiveFilter(m_filters.front());
        }
    } else {
        bool found = false;
        for (const auto &fi: m_filters) {
            if (fi.filter == m_activeFilter.filter) {
                found = true;
                break;
            }
        }
        if (!found) {
            unsetActiveFilter();
        }
    }
}

void PixelMatcher::periodicUpdate()
{
    findFilters();
    updateActiveFilter();

    if (m_filterData) {
        m_filterData->frame_wanted = true;
    }
}

#include <GL/gl.h>

void PixelMatcher::checkFrame()
{
    if (m_filterData && m_filterData->frame_available) {
        // TODO: copy; signal display
        m_filterData->frame_available = false;
    }
}

