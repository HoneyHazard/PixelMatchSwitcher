#include "pixel-match-core.hpp"
#include "pixel-match-dialog.hpp"
#include "pixel-match-filter.h"

#include <ostream>
#include <sstream>

#include <obs-module.h>
#include <obs-frontend-api.h>
//#include <obs-scene.h>

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
}

std::vector<PmFilterRef> PixelMatcher::filters() const
{
    QMutexLocker locker(&m_mutex);
    return m_filters;
}

PmFilterRef PixelMatcher::activeFilterRef() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeFilter;
}

std::string PixelMatcher::scenesInfo() const
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
            [](obs_scene_t*, obs_sceneitem_t *item, void* p) {
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

void PixelMatcher::scanScenes()
{
    using namespace std;

    QMutexLocker locker(&m_mutex);

    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);

    m_filters.clear();
    m_filters.push_back(PmFilterRef());

    // TODO: interfacing with OBS enumeration is kind of hacky, though not
    //       without a reason. for sure, this can be cleaned up.
    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto &fi = m_filters.back();
        fi.setScene(scenes.sources.array[i]);
        obs_scene_enum_items(
            fi.scene(),
            [](obs_scene_t*, obs_sceneitem_t *item, void* p) {
                auto &filters = *static_cast<vector<PmFilterRef>*>(p);
                auto &fi = filters.back();
                fi.setItem(item);
                obs_source_enum_filters(
                    fi.itemSrc(),
                    [](obs_source_t *, obs_source_t *filter, void *p) {
                    auto &filters = *static_cast<vector<PmFilterRef>*>(p);
                        auto id = obs_source_get_id(filter);
                        if (!strcmp(id, PIXEL_MATCH_FILTER_ID)) {
                            auto copy = filters.back();
                            filters.back().setFilter(filter);
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

void PixelMatcher::updateActiveFilter()
{
    QMutexLocker locker(&m_mutex);
    if (m_activeFilter.isValid()) {
        bool found = false;
        for (const auto &fi: m_filters) {
            if (fi.filter() == m_activeFilter.filter() && fi.isActive()) {
                found = true;
                break;
            }
        }
        if (found) return;

        m_activeFilter.reset();
    }
    for (const auto &fi: m_filters) {
        if (fi.isActive()) {
            m_activeFilter = fi;
        }
    }
}


void PixelMatcher::periodicUpdate()
{
    scanScenes();
    updateActiveFilter();
}

//======================================


void PmFilterRef::setScene(obs_source_t *sceneSrc)
{
    if (m_sceneSrc) {
        obs_source_release(m_sceneSrc);
    }
    m_sceneSrc = sceneSrc;
    if (m_sceneSrc) {
        obs_source_addref(m_sceneSrc);
    }

    if (m_scene) {
        obs_scene_release(m_scene);
    }
    if (m_sceneSrc) {
        m_scene = obs_scene_from_source(m_sceneSrc);
    }
    if (m_scene) {
        obs_scene_addref(m_scene);
    }
}

void PmFilterRef::setItem(obs_scene_item *item)
{
    m_sceneItem = item;

    if (m_itemSrc) {
        obs_source_release(m_itemSrc);
    }
    if (m_sceneItem) {
        m_itemSrc = obs_sceneitem_get_source(item);
    }
    if (m_itemSrc) {
        obs_source_addref(m_itemSrc);
    }
}

void PmFilterRef::setFilter(obs_source_t *filter)
{
    if (m_filter) {
        obs_source_release(m_filter);
    }
    m_filter = filter;
    if (m_filter) {
        obs_source_addref(m_filter);
    }
}

void PmFilterRef::lockData() const
{
    auto data = filterData();
    pthread_mutex_lock(&data->mutex);
}

void PmFilterRef::unlockData() const
{
    auto data = filterData();
    pthread_mutex_unlock(&data->mutex);
}

PmFilterRef::PmFilterRef(const PmFilterRef &other)
{
    setScene(other.sceneSrc());
    setItem(other.sceneItem());
    setFilter(other.filter());
}

void PmFilterRef::reset()
{
    if (m_filter) {
        obs_source_release(m_filter);
        m_filter = nullptr;
    }
    if (m_itemSrc) {
        obs_source_release(m_itemSrc);
        m_itemSrc = nullptr;
    }
    m_sceneItem = nullptr;
    if (m_scene) {
        obs_scene_release(m_scene);
        m_scene = nullptr;
    }
    if (m_sceneSrc) {
        obs_source_release(m_sceneSrc);
        m_sceneSrc = nullptr;
    }
}

pixel_match_filter_data *PmFilterRef::filterData() const
{
    if (!m_filter) {
        return nullptr;
    } else {
        return static_cast<pixel_match_filter_data*>(
            obs_obj_get_data(m_filter));
    }
}

bool PmFilterRef::isActive() const
{
    return m_itemSrc != nullptr && obs_source_active(m_itemSrc);
}

uint32_t PmFilterRef::filterSrcWidth() const
{
    return m_filter ? obs_source_get_width(m_filter) : 0;
}

uint32_t PmFilterRef::filterSrcHeight() const
{
    return m_filter ? obs_source_get_height(m_filter) : 0;
}

uint32_t PmFilterRef::filterDataWidth() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->cx;
        unlockData();
    }
    return ret;
}

uint32_t PmFilterRef::filterDataHeight() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->cy;
        unlockData();
    }
    return ret;
}

uint32_t PmFilterRef::numMatched() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->num_matched;
        unlockData();
    }
    return ret;
}
