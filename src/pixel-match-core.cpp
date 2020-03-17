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
#include <QThread>
#include <QTextStream>
#include <QSpinBox>

PmCore* PmCore::m_instance = nullptr;

void init_pixel_match_switcher()
{
    qRegisterMetaType<PmResultsPacket>("PmResultsPacket");
    qRegisterMetaType<PmConfigPacket>("PmConfigPacket");

    PmCore::m_instance = new PmCore();
}

void free_pixel_match_switcher()
{
    PmCore::m_instance->deleteLater();
    PmCore::m_instance = nullptr;
}

void on_frame_processed(struct pm_filter_data *filter)
{
    auto core = PmCore::m_instance;
    if (core) {
        QMutexLocker locker(&core->m_mutex);
        const auto &fr = core->m_activeFilter;
        if (fr.filterData() == filter) {
            emit core->sigFrameProcessed();
        }
    }
}

//------------------------------------

PmCore::PmCore()
//: m_mutex(QMutex::Recursive)
{
    // parent this to the app
    //setParent(qApp);

    // move to own thread
    QThread *pmThread = new QThread(qApp);
    pmThread->setObjectName("pixel match core thread");
    moveToThread(pmThread);

    // add action item in the Tools menu of the app
    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction(
            obs_module_text("Pixel Match Switcher")));
    connect(action, &QAction::triggered, this, &PmCore::onMenuAction,
            Qt::DirectConnection);

    // periodic update timer: process in the UI thread
    QTimer *periodicUpdateTimer = new QTimer(this);
    connect(periodicUpdateTimer, &QTimer::timeout,
            this, &PmCore::onPeriodicUpdate, Qt::QueuedConnection);

    // fast reaction signal/slot: process in the core's thread
    connect(this, &PmCore::sigFrameProcessed,
            this, &PmCore::onFrameProcessed, Qt::QueuedConnection);

    pmThread->start();
    periodicUpdateTimer->start(100);
}

PmCore::~PmCore()
{
    if (m_dialog) {
        m_dialog->deleteLater();
    }
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

PmResultsPacket PmCore::results() const
{
    QMutexLocker locker(&m_mutex);
    return m_results;
}

PmConfigPacket PmCore::config() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
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

    obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);

    vector<PmFilterRef> filters;
    filters.push_back(PmFilterRef());

    // TODO: interfacing with OBS enumeration is kind of hacky, though not
    //       without a reason. for sure, this can be cleaned up.
    for (size_t i = 0; i < scenes.sources.num; ++i) {
        auto &fi = filters.back();
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
            &filters
        );
    }
    filters.pop_back();

    {
        QMutexLocker locker(&m_mutex);
        m_filters = filters;
    }
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

        auto data = m_activeFilter.filterData();
        if (data) {
                pthread_mutex_lock(&data->mutex);
                data->on_frame_processed = nullptr;
                pthread_mutex_unlock(&data->mutex);
        }
        m_activeFilter.reset();
    }
    for (const auto &fi: m_filters) {
        if (fi.isActive()) {
            m_activeFilter = fi;
            auto data = m_activeFilter.filterData();
            if (data) {
                pthread_mutex_lock(&data->mutex);
                data->on_frame_processed = on_frame_processed;
                pthread_mutex_unlock(&data->mutex);
                if (!m_qImage.isNull())
                    supplyImageToFilter();
                supplyConfigToFilter();
            }
        }
    }
}

void PmCore::onMenuAction()
{
    // main UI dialog
    if (!m_dialog) {
        // parent a dialog parented to the main window;
        // make sure it is destroyed once closed
        auto mainWindow
            = static_cast<QMainWindow*>(obs_frontend_get_main_window());
        m_dialog = new PmDialog(this, mainWindow);
        m_dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(m_dialog, &PmDialog::sigOpenImage,
                this, &PmCore::onOpenImage, Qt::QueuedConnection);
        connect(m_dialog, &PmDialog::sigNewUiConfig,
                this, &PmCore::onNewUiConfig, Qt::QueuedConnection);
    }
    m_dialog->show();
}

void PmCore::onPeriodicUpdate()
{
    scanScenes();
    updateActiveFilter();
}

void PmCore::onFrameProcessed()
{
    PmResultsPacket newResults;
    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
        newResults.baseWidth = filterData->base_width;
        newResults.baseHeight = filterData->base_height;
        newResults.matchImgWidth = filterData->match_img_width;
        newResults.matchImgHeight = filterData->match_img_height;
        newResults.numCompared = filterData->num_compared;
        newResults.numMatched = filterData->num_matched;
        // TODO more results
        pthread_mutex_unlock(&filterData->mutex);
    }

    newResults.percentageMatched
        = float(newResults.numMatched) / float(newResults.numCompared) * 100.0f;
    newResults.isMatched
        = newResults.percentageMatched >= m_config.totalMatchThresh;
    {
        QMutexLocker locker(&m_mutex);
        m_results = newResults;
    }
    emit sigNewResults(m_results);
}

void PmCore::onOpenImage(QString filename)
{
    QImage imgIn(filename);
    if (imgIn.isNull()) {
        blog(LOG_WARNING, "Unable to open filename: %s",
             filename.toUtf8().constData());
        emit sigImgFailed(filename);
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_qImage = imgIn.convertToFormat(QImage::Format_ARGB32);
    if (m_qImage.isNull()) {
        blog(LOG_WARNING, "Image conversion failed: %s",
             filename.toUtf8().constData());
        emit sigImgFailed(filename);
        return;
    }

    supplyImageToFilter();
    emit sigImgSuccess(filename);
}

void PmCore::supplyImageToFilter()
{
    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
        if (filterData->match_img_data) {
            bfree(filterData->match_img_data);
        }
        size_t sz = size_t(m_qImage.sizeInBytes());
        filterData->match_img_data = bmalloc(sz);
        memcpy(filterData->match_img_data, m_qImage.bits(), sz);

        filterData->match_img_width = uint32_t(m_qImage.width());
        filterData->match_img_height = uint32_t(m_qImage.height());

        pthread_mutex_unlock(&filterData->mutex);
    }
}

void PmCore::supplyConfigToFilter()
{
    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
        filterData->roi_left = m_config.roiLeft;
        filterData->roi_bottom = m_config.roiBottom;
        filterData->per_pixel_err_thresh = m_config.perPixelErrThresh;
        filterData->total_match_thresh = m_config.totalMatchThresh;
        pthread_mutex_unlock(&filterData->mutex);
    }
}

void PmCore::onNewUiConfig(PmConfigPacket config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    supplyConfigToFilter();
}
