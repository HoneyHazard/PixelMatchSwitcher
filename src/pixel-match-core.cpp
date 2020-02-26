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

PmCore* PmCore::m_instance = nullptr;

void init_pixel_match_switcher()
{
    PmCore::m_instance = new PmCore();
}

void free_pixel_match_switcher()
{
    PmCore::m_instance->deleteLater();
    PmCore::m_instance = nullptr;
}

//------------------------------------

PmCore::PmCore()
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
            this, &PmCore::periodicUpdate);
    periodicUpdateTimer->start(100);
}

std::vector<PmFilterRef> PmCore::filters() const
{
    QMutexLocker locker(&m_mutex);
    return m_filters;
}

PmFilterRef PmCore::activeFilterRef() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeFilter;
}

std::string PmCore::scenesInfo() const
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

void PmCore::scanScenes()
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

void PmCore::updateActiveFilter()
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


void PmCore::periodicUpdate()
{
    scanScenes();
    updateActiveFilter();
}

