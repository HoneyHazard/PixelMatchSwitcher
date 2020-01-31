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

    // timer to fetch rendered frames
    QTimer *checkFrameTimer = new QTimer(this);
    connect(checkFrameTimer, &QTimer::timeout,
            this, &PixelMatcher::checkFrame);
    checkFrameTimer->start(10);
}

std::vector<PmFilterInfo> PixelMatcher::filters() const
{
    QMutexLocker locker(&m_mutex);
    return m_filters;
}

PmFilterInfo PixelMatcher::activeFilterInfo() const
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
    m_filters.push_back(PmFilterInfo());

    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto &fi = m_filters.back();
        fi.sceneSrc = scenes.sources.array[i];
        fi.scene = obs_scene_from_source(fi.sceneSrc);
        obs_scene_enum_items(
            fi.scene,
            [](obs_scene_t*, obs_sceneitem_t *item, void* p) {
                auto &filters = *static_cast<vector<PmFilterInfo>*>(p);
                auto &fi = filters.back();
                fi.sceneItem = item;
                fi.itemSrc = obs_sceneitem_get_source(item);
                obs_source_enum_filters(
                    fi.itemSrc,
                    [](obs_source_t *, obs_source_t *filter, void *p) {
                    auto &filters = *static_cast<vector<PmFilterInfo>*>(p);
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
    m_activeFilter = PmFilterInfo();
    m_filterData = nullptr;
}

void PixelMatcher::setActiveFilter(const PmFilterInfo &fi)
{
    m_activeFilter = fi;
    m_filterData = static_cast<pixel_match_filter_data*>(
        obs_obj_get_data(m_activeFilter.filter));

}

void PixelMatcher::updateActiveFilter()
{
    QMutexLocker locker(&m_mutex);
    if (m_activeFilter.filter) {
        bool found = false;
        for (const auto &fi: m_filters) {
            if (fi.filter == m_activeFilter.filter
             && obs_source_active(fi.itemSrc)) {
                found = true;
                break;
            }
        }
        if (found) return;

        unsetActiveFilter();
    }
    for (const auto &fi: m_filters) {
        if (obs_source_active(fi.itemSrc)) {
            setActiveFilter(fi);
        }
    }
}

void PixelMatcher::periodicUpdate()
{
    scanScenes();
    updateActiveFilter();

    if (m_filterData) {
        m_filterData->frame_wanted = true;
    }
}

void PixelMatcher::checkFrame()
{
    if (m_filterData) {
        if (m_filterData->frame_available) {
            // store a copy of the frame data
            int cx = int(m_filterData->cx);
            int cy = int(m_filterData->cy);
            int sz = cx * cy * 4;
            if (m_frameImage.width() != cx || m_frameImage.height() != cy) {
                m_frameRgba.resize(sz);
                // QImage provides an abstraction to utilize the frame data in previews
                m_frameImage = QImage(
                    (const uchar*)m_frameRgba.data(),
                    cx, cy, cx * 4,
                    QImage::Format_RGBA8888);
            }
            memcpy(m_frameRgba.data(), m_filterData->pixel_data, size_t(sz));

            m_filterData->frame_available = false;
        }
    } else {
        if (m_frameImage.width() != 0 || m_frameImage.height()) {
            m_frameImage = QImage();
        }
        if (m_frameRgba.size()) {
            m_frameRgba.clear();
        }
    }
    emit newFrameImage();
}

