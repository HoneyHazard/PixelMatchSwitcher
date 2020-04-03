#include "pm-core.hpp"
#include "pm-dialog.hpp"
#include "pm-filter.h"

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
    pmRegisterMetaTypes();

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
        QMutexLocker locker(&core->m_filtersMutex);
        const auto &fr = core->m_activeFilter;
        if (fr.filterData() == filter) {
            emit core->sigFrameProcessed();
        }
    }
}

//------------------------------------

PmCore::PmCore()
: m_matchConfigMutex(QMutex::Recursive)
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

    // basically the default effect except sampler is made Point instead of Linear
    obs_enter_graphics();
    char *effect_path = obs_module_file("draw_match_image.effect");
    m_drawMatchImageEffect
        = gs_effect_create_from_file(effect_path, nullptr);
    obs_leave_graphics();

    // fire up the engines
    pmThread->start();
    periodicUpdateTimer->start(100);
}

PmCore::~PmCore()
{
    if (m_dialog) {
        m_dialog->deleteLater();
    }
    if (m_drawMatchImageEffect) {
        gs_effect_destroy(m_drawMatchImageEffect);
    }
}

std::vector<PmFilterRef> PmCore::filters() const
{
    QMutexLocker locker(&m_filtersMutex);
    return m_filters;
}

PmFilterRef PmCore::activeFilterRef() const
{
    QMutexLocker locker(&m_filtersMutex);
    return m_activeFilter;
}

PmMatchResults PmCore::results() const
{
    QMutexLocker locker(&m_resultsMutex);
    return m_results;
}

std::string PmCore::activeMatchPresetName() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_activeMatchPreset;
}

bool PmCore::matchPresetExists(const std::string &name) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets.find(name) != m_matchPresets.end();
}

PmMatchConfig PmCore::matchPresetByName(const std::string &name) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets.at(name);
}

PmMatchConfig PmCore::matchConfig() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchConfig;
}

std::string PmCore::matchImgFilename() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchConfig.matchImgFilename;
}

PmMatchPresets PmCore::matchPresets() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets;
}

bool PmCore::matchConfigDirty() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (m_activeMatchPreset.empty()) {
        return true;
    } else {
        const PmMatchConfig presetCfg = m_matchPresets.at(m_activeMatchPreset);
        return presetCfg != m_matchConfig;
    }
}

void PmCore::onSaveMatchPreset(std::string name)
{
    QMutexLocker locker(&m_matchConfigMutex);
    bool isNew = !matchPresetExists(name);
    m_matchPresets[name] = m_matchConfig;
    if (isNew) {
        emit sigMatchPresetsChanged();
    }
    m_activeMatchPreset = name;
    emit sigMatchPresetStateChanged();
}

void PmCore::onSelectActiveMatchPreset(std::string name)
{
    QMutexLocker locker(&m_matchConfigMutex);
    m_activeMatchPreset = name;
    PmMatchConfig config = name.size() ? m_matchPresets[name] : PmMatchConfig();
    onNewMatchConfig(config);
    emit sigMatchPresetStateChanged();
}

void PmCore::onRemoveMatchPreset(std::string name)
{
    QMutexLocker locker(&m_matchConfigMutex);
    m_matchPresets.erase(name);
    emit sigMatchPresetsChanged();
    if (m_activeMatchPreset == name) {
        onSelectActiveMatchPreset("");
    }
}

PmScenes PmCore::scenes() const
{
    QMutexLocker locker(&m_scenesMutex);
    return m_scenes;
}

PmSwitchConfig PmCore::switchConfig() const
{
    QMutexLocker locker(&m_switchConfigMutex);
    return m_switchConfig;
}

QSize PmCore::videoBaseSize() const
{
    obs_video_info ovi;
    obs_get_video_info(&ovi);
    return QSize(int(ovi.base_width), int(ovi.base_height));
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

    obs_frontend_source_list scenesInput = {};
    obs_frontend_get_scenes(&scenesInput);

    PmScenes scenes;
    vector<PmFilterRef> filters;
    filters.push_back(PmFilterRef());

    // TODO: interfacing with OBS enumeration is kind of hacky, though not
    //       without a reason. for sure, this can be cleaned up.
    for (size_t i = 0; i < scenesInput.sources.num; ++i) {
        auto &fi = filters.back();
        auto &scene = scenesInput.sources.array[i];
        auto ws = OBSWeakSource(obs_source_get_weak_source(scene));
        scenes.insert(ws);
        fi.setScene(scene);
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
        QMutexLocker locker(&m_filtersMutex);
        m_filters = filters;
    }

    {
        QMutexLocker locker(&m_scenesMutex);
        if (m_scenes != scenes) {
            m_scenes = scenes;
            emit sigScenesChanged(scenes);
        }
    }
}

void PmCore::updateActiveFilter()
{
    QMutexLocker locker(&m_filtersMutex);
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
                if (!m_matchImg.isNull())
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
    PmMatchResults newResults;
    QSize baseSz = videoBaseSize();
    newResults.baseWidth = baseSz.width();
    newResults.baseHeight = baseSz.height();

    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
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
        = newResults.percentageMatched >= m_matchConfig.totalMatchThresh;
    {
        QMutexLocker locker(&m_resultsMutex);
        m_results = newResults;
    }
    emit sigNewMatchResults(m_results);

    {
        QMutexLocker locker(&m_switchConfigMutex);
        if (m_switchConfig.isEnabled
         && m_activeFilter.isValid() && !m_matchImg.isNull()) {
            QMutexLocker locker(&m_resultsMutex);
            if (m_results.isMatched) {
                switchScene(m_switchConfig.matchScene, m_switchConfig.defaultTransition);
            } else {
                switchScene(m_switchConfig.noMatchScene, m_switchConfig.defaultTransition);
            }
        }
    }
}

// copied (and slightly simplified) from Advanced Scene Switcher:
// https://github.com/WarmUpTill/SceneSwitcher/blob/05540b61a118f2190bb3fae574c48aea1b436ac7/src/advanced-scene-switcher.cpp#L1066
void PmCore::switchScene(OBSWeakSource &scene, OBSWeakSource &transition)
{
    obs_source_t* source = obs_weak_source_get_source(scene);
    obs_source_t* currentSource = obs_frontend_get_current_scene();

    if (source && source != currentSource)
    {
        obs_weak_source_t*  currentScene = obs_source_get_weak_source(currentSource);
        obs_weak_source_release(currentScene);

        if (transition) {
            obs_source_t* nextTransition = obs_weak_source_get_source(transition);
            //lock.unlock();
            //transitionCv.wait(transitionLock, transitionActiveCheck);
            //lock.lock();
            obs_frontend_set_current_transition(nextTransition);
            obs_source_release(nextTransition);
        }
        obs_frontend_set_current_scene(source);
    }
    obs_source_release(currentSource);
    obs_source_release(source);
}

void PmCore::supplyImageToFilter()
{
    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
        if (filterData->match_img_data) {
            bfree(filterData->match_img_data);
        }
        size_t sz = size_t(m_matchImg.sizeInBytes());
        filterData->match_img_data = bmalloc(sz);
        memcpy(filterData->match_img_data, m_matchImg.bits(), sz);

        filterData->match_img_width = uint32_t(m_matchImg.width());
        filterData->match_img_height = uint32_t(m_matchImg.height());

        pthread_mutex_unlock(&filterData->mutex);
    }
}

void PmCore::supplyConfigToFilter()
{
    struct vec3 maskColor;
    bool maskAlpha = false;

    switch(m_matchConfig.maskMode) {
    case PmMaskMode::AlphaMode:
        maskAlpha = true;
        break;
    case PmMaskMode::BlackMode:
        vec3_set(&maskColor, 0.f, 0.f, 0.f);
        break;
    case PmMaskMode::GreenMode:
        vec3_set(&maskColor, 0.f, 1.f, 0.f);
        break;
    case PmMaskMode::MagentaMode:
        vec3_set(&maskColor, 1.f, 0.f, 1.f);
        break;
    case PmMaskMode::CustomClrMode:
        {
            uint8_t *colorBytes = reinterpret_cast<uint8_t*>(&m_matchConfig.customColor);
            if (__BYTE_ORDER == __LITTLE_ENDIAN) {
                vec3_set(&maskColor,
                         float(colorBytes[2])/255.f,
                         float(colorBytes[1])/255.f,
                         float(colorBytes[0])/255.f);
            } else {
                vec3_set(&maskColor,
                         float(colorBytes[1])/255.f,
                         float(colorBytes[2])/255.f,
                         float(colorBytes[3])/255.f);
            }
        }
        break;
    default:
        blog(LOG_ERROR, "Unknown color mode: %i", m_matchConfig.maskMode);
        break;
    }

    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
        filterData->roi_left = m_matchConfig.roiLeft;
        filterData->roi_bottom = m_matchConfig.roiBottom;
        filterData->per_pixel_err_thresh = m_matchConfig.perPixelErrThresh / 100.f;
        filterData->total_match_thresh = m_matchConfig.totalMatchThresh;
        filterData->mask_alpha = maskAlpha;
        filterData->mask_color = maskColor;
        pthread_mutex_unlock(&filterData->mutex);
    }
}

void PmCore::onNewMatchConfig(PmMatchConfig config)
{
    QMutexLocker locker(&m_matchConfigMutex);

    const char* filename = config.matchImgFilename.data();
    if (filename != m_matchConfig.matchImgFilename) {
        QImage imgIn(filename);
        if (imgIn.isNull()) {
            blog(LOG_WARNING, "Unable to open filename: %s", filename);
            emit sigImgFailed(filename);
            m_matchImg = QImage();
        } else {
            m_matchImg = imgIn.convertToFormat(QImage::Format_ARGB32);
            if (m_matchImg.isNull()) {
                blog(LOG_WARNING, "Image conversion failed: %s", filename);
                emit sigImgFailed(filename);
                m_matchImg = QImage();
            } else {
                emit sigImgSuccess(filename);
            }
        }
        supplyImageToFilter();
    }
    if (m_matchConfig != config) {
        m_matchConfig = config;
        supplyConfigToFilter();
        emit sigMatchPresetStateChanged();
    }
}

void PmCore::onNewSwitchConfig(PmSwitchConfig sceneConfig)
{
    QMutexLocker locker(&m_switchConfigMutex);
    m_switchConfig = sceneConfig;
}
