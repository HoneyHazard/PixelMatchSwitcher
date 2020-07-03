#include "pm-core.hpp"
#include "pm-dialog.hpp"
#include "pm-filter.h"

#include <ostream>
#include <sstream>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-data.h>
//#include <obs-scene.h>

#include <QAction>
#include <QMainWindow>
#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QTextStream>
#include <QSet>

PmCore* PmCore::m_instance = nullptr;

void pm_save_load_callback(obs_data_t *save_data, bool saving, void *corePtr)
{
    PmCore *core = static_cast<PmCore*>(corePtr);
    if (saving) {
        core->pmSave(save_data);
    } else {
        core->pmLoad(save_data);
    }
}

void init_pixel_match_switcher()
{
    pmRegisterMetaTypes();

    PmCore::m_instance = new PmCore();

    obs_frontend_add_save_callback(
        pm_save_load_callback, static_cast<void*>(PmCore::m_instance));
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
        auto fr = core->activeFilterRef();
        if (fr.filterData() == filter) {
            emit core->sigFrameProcessed();
        }
    }
}

//------------------------------------

QHash<std::string, OBSWeakSource> PmCore::getAvailableTransitions()
{
    QHash<std::string, OBSWeakSource> ret;
    struct obs_frontend_source_list transitionList = { 0 };
    obs_frontend_get_transitions(&transitionList);
    size_t num = transitionList.sources.num;
    for (size_t i = 0; i < num; i++) {
        obs_source_t* src = transitionList.sources.array[i];
        auto weakSrc = obs_source_get_weak_source(src);
        auto name = obs_source_get_name(src);
        ret.insert(name, weakSrc);
    }
    obs_frontend_source_list_free(&transitionList);
    return ret;
}

PmCore::PmCore()
: m_matchConfigMutex(QMutex::Recursive)
{
    // add action item in the Tools menu of the app
    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction(
            obs_module_text("Pixel Match Switcher")));
    connect(action, &QAction::triggered, this, &PmCore::onMenuAction,
            Qt::DirectConnection);

    // periodic update timer: process in the UI thread
    //QTimer *periodicUpdateTimer = new QTimer(qApp);
    QTimer* periodicUpdateTimer = new QTimer(qApp);
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
    periodicUpdateTimer->start(100);

    // move to own thread
    QThread *pmThread = new QThread(qApp);
    pmThread->setObjectName("pixel match core thread");
    moveToThread(pmThread);
    pmThread->start();
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

PmPreviewConfig PmCore::previewConfig() const
{
    QMutexLocker locker(&m_previewConfigMutex);
    return m_previewConfig;
}

PmMultiMatchResults PmCore::multiMatchResults() const
{
    QMutexLocker locker(&m_resultsMutex);
    return m_results;
}

PmMatchResults PmCore::matchResults(size_t matchIdx) const
{
    QMutexLocker locker(&m_resultsMutex);
    return matchIdx < m_results.size() ? m_results[matchIdx] : PmMatchResults();
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

PmMultiMatchConfig PmCore::matchPresetByName(const std::string &name) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets.at(name);
}

PmMultiMatchConfig PmCore::multiMatchConfig() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_multiMatchConfig;
}

size_t PmCore::multiMatchConfigSize() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_multiMatchConfig.size();
}

PmMatchConfig PmCore::matchConfig(size_t matchIdx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return matchIdx < m_multiMatchConfig.size() ? m_multiMatchConfig[matchIdx]
                                                : PmMatchConfig();
}

std::string PmCore::noMatchScene() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_multiMatchConfig.noMatchScene;
}

std::string PmCore::noMatchTransition() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_multiMatchConfig.noMatchTransition;
}

std::string PmCore::matchImgFilename(size_t matchIdx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (matchIdx >= m_multiMatchConfig.size())
        return std::string();
    else 
        return m_multiMatchConfig[matchIdx].matchImgFilename;
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
        const PmMultiMatchConfig presetCfg 
            = m_matchPresets.at(m_activeMatchPreset);
        return presetCfg != m_multiMatchConfig;
    }
}

void PmCore::onSaveMatchPreset(std::string name)
{
    QMutexLocker locker(&m_matchConfigMutex);
    bool isNew = !matchPresetExists(name);
    m_matchPresets[name] = m_multiMatchConfig;
    if (isNew) {
        emit sigMatchPresetsChanged();
    }
    m_activeMatchPreset = name;
    emit sigMatchPresetStateChanged();
}

void PmCore::onChangedMatchConfig(size_t matchIdx, PmMatchConfig newCfg)
{
    size_t sz = multiMatchConfigSize();

    if (matchIdx >= sz) return; // invalid index?

    auto oldCfg = matchConfig(matchIdx);
    
    if (oldCfg == newCfg) return; // config hasn't changed?

    // notify other modules
    emit sigChangedMatchConfig(matchIdx, newCfg);

    // store the new config
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig[matchIdx] = newCfg;
    }    
   
    // update images
    bool imageChanged = false;
    QImage img;
    {
        if (newCfg.matchImgFilename != oldCfg.matchImgFilename) {
            const char* filename = newCfg.matchImgFilename.data();
            img = QImage(filename);
            if (img.isNull()) {
                blog(LOG_WARNING, "Unable to open filename: %s", filename);
                emit sigImgFailed(matchIdx, filename);
                img = QImage();
            } else {
                img = img.convertToFormat(QImage::Format_ARGB32);
                if (img.isNull()) {
                    blog(LOG_WARNING, "Image conversion failed: %s", filename);
                    emit sigImgFailed(matchIdx, filename);
                    img = QImage();
                } else {
                    emit sigImgSuccess(matchIdx, filename, img);
                }
            }
            {
                QMutexLocker locker(&m_matchImagesMutex);
                m_matchImages[matchIdx] = img;
                imageChanged = true;
            }
        }
    }

    // update filter
    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        pm_supply_match_entry_config(
            fr.filterData(), matchIdx, &newCfg.filterCfg);
        if (imageChanged) {
            supplyImageToFilter(filterData, matchIdx, img);
        }
    }
}

void PmCore::onInsertMatchConfig(size_t matchIndex, PmMatchConfig cfg)
{
    size_t oldSz = multiMatchConfigSize();
    if (matchIndex > oldSz) matchIndex = oldSz;
    size_t newSz = oldSz + 1;
    
    // notify other modules
    emit sigNewMultiMatchConfigSize(newSz);
    
    // reconfigure the filter
    auto fr = activeFilterRef();
    pm_filter_data* filterData = fr.filterData();
    if (filterData) {
        pm_resize_match_entries(filterData, newSz);
    }

    // reconfigure images
    {
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages.insert(m_matchImages.begin() + matchIndex, QImage());
    }
    
    // finish updating state and notifying
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig.resize(newSz);
    }
    for (size_t i = matchIndex + 1; i < newSz; ++i) {
        onChangedMatchConfig(i, m_multiMatchConfig[i-1]);
    }
    onChangedMatchConfig(matchIndex, cfg);
}

void PmCore::onRemoveMatchConfig(size_t matchIndex)
{
    size_t oldSz = multiMatchConfigSize();
    if (matchIndex >= oldSz) return;
    size_t newSz = oldSz - 1;

    // notify other modules
    emit sigNewMultiMatchConfigSize(newSz);

    // reconfigure the filter
    auto fr = activeFilterRef();
    pm_filter_data* filterData = fr.filterData();
    if (filterData) {
        pm_resize_match_entries(filterData, newSz);
    }

    // reconfigure images
    {
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages.erase(m_matchImages.begin() + matchIndex);
    }

    // finish updating state and notifying
    for (size_t i = matchIndex; i < newSz; ++i) {
        onChangedMatchConfig(i, matchConfig(i + 1));
    }
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig.resize(newSz);
    }
    onSelectMatchIndex(matchIndex);
}

void PmCore::onResetMatchConfigs()
{
    // notify other modules
    emit sigNewMultiMatchConfigSize(0);

    // reconfigure the filter
    auto fr = activeFilterRef();
    pm_filter_data* filterData = fr.filterData();
    if (filterData) {
        pm_resize_match_entries(filterData, 0);
    }

    // finish updating state and notifying
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig = PmMultiMatchConfig();
    }
    {
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages.clear();
    }
    onSelectMatchIndex(0);
}

void PmCore::onMoveMatchConfigUp(size_t matchIndex)
{
    if (matchIndex < 1 || matchIndex >= multiMatchConfigSize()) return;

    PmMatchConfig oldAbove = matchConfig(matchIndex - 1);
    PmMatchConfig oldBelow = matchConfig(matchIndex);
    onChangedMatchConfig(matchIndex - 1, oldBelow);
    onChangedMatchConfig(matchIndex, oldAbove);

    if (matchIndex == m_selectedMatchIndex)
        onSelectMatchIndex(matchIndex - 1);
}

void PmCore::onMoveMatchConfigDown(size_t matchIndex)
{
    if (matchIndex >= multiMatchConfigSize() - 1) return;

    PmMatchConfig oldAbove = matchConfig(matchIndex);
    PmMatchConfig oldBelow = matchConfig(matchIndex + 1);
    onChangedMatchConfig(matchIndex + 1, oldAbove);
    onChangedMatchConfig(matchIndex, oldBelow);

    if (matchIndex == m_selectedMatchIndex)
        onSelectMatchIndex(matchIndex + 1);
}

void PmCore::onSelectMatchIndex(size_t matchIndex)
{   
    m_selectedMatchIndex = matchIndex;

    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);
        filterData->selected_match_index = matchIndex;
        pthread_mutex_unlock(&filterData->mutex);
    }
    
    emit sigSelectMatchIndex(matchIndex, matchConfig(matchIndex));
}

void PmCore::onNoMatchSceneChanged(std::string sceneName)
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (m_multiMatchConfig.noMatchScene != sceneName) {
        m_multiMatchConfig.noMatchScene = sceneName;
        emit sigNoMatchSceneChanged(sceneName);
    }
}

void PmCore::onNoMatchTransitionChanged(std::string transName)
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (transName.empty()) {
        transName = "Cut";
    }
    if (m_multiMatchConfig.noMatchTransition != transName) {
        m_multiMatchConfig.noMatchTransition = transName;
        emit sigNoMatchTransitionChanged(transName);
    }
}

void PmCore::onPreviewConfigChanged(PmPreviewConfig cfg)
{
    QMutexLocker locker(&m_previewConfigMutex);

    if(m_previewConfig == cfg) return;

    m_previewConfig = cfg;
    emit sigPreviewConfigChanged(cfg);
}

void PmCore::onSelectActiveMatchPreset(std::string name)
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (m_activeMatchPreset == name) return;
    
    PmMultiMatchConfig multiConfig 
        = name.size() ? m_matchPresets[name] : PmMultiMatchConfig();      
    
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

QImage PmCore::matchImage(size_t matchIdx) const
{ 
    QMutexLocker locker(&m_matchImagesMutex);
    if (matchIdx >= m_matchImages.size())
        return QImage();
    else
        return m_matchImages[matchIdx]; 
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

    for (size_t i = 0; i < scenesInput.sources.num; ++i) {
        auto &fi = filters.back();
        auto &sceneSrc = scenesInput.sources.array[i];
        auto ws = OBSWeakSource(obs_source_get_weak_source(sceneSrc));
        scenes.insert(ws);
        fi.setScene(sceneSrc);
        obs_scene_enum_items(
            obs_scene_from_source(sceneSrc),
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

    bool scenesChanged = false;
    {
        QMutexLocker locker(&m_scenesMutex);
        if (m_scenes != scenes) {
            m_scenes = scenes;
            emit sigScenesChanged(scenes);
            scenesChanged = true;
        }
    }

    if (scenesChanged) {
        size_t cfgSize = multiMatchConfigSize();
        auto sceneNames = scenes.sceneNames();
        for (size_t i = 0; i < cfgSize; ++i) {
            auto cfgCpy = matchConfig(i);
            if (cfgCpy.matchScene.size()
             && !sceneNames.contains(cfgCpy.matchScene.data())) {
                cfgCpy.matchScene.clear();
                onChangedMatchConfig(i, cfgCpy);
            }
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
        pm_filter_data* data;
        if (fi.isActive() && (data = fi.filterData())) {
            m_activeFilter = fi;
            size_t cfgSize = multiMatchConfigSize();
            pthread_mutex_lock(&data->mutex);
            data->on_frame_processed = on_frame_processed;
            data->selected_match_index = m_selectedMatchIndex;
            pm_resize_match_entries(data, cfgSize);
            for (size_t i = 0; i < cfgSize; ++i) {
                auto cfg = matchConfig(i).filterCfg;
                pm_supply_match_entry_config(data, i, &cfg);
                supplyImageToFilter(data, i, matchImage(i));
            }
            pthread_mutex_unlock(&data->mutex);
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
    if (m_availableTransitions.empty()) {
        m_availableTransitions = getAvailableTransitions();
    }

    scanScenes();
    updateActiveFilter();
}

void PmCore::onFrameProcessed()
{
    PmMultiMatchResults newResults;

    // fetch new results
    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        pthread_mutex_lock(&filterData->mutex);

        newResults.resize(filterData->num_match_entries);
        for (size_t i = 0; i < newResults.size(); ++i) {
            auto& newResult = newResults[i];
            const auto& filterEntry = filterData->match_entries + i;
            newResult.baseWidth = filterData->base_width;
            newResult.baseHeight = filterData->base_height;
            newResult.matchImgWidth = filterEntry->match_img_width;
            newResult.matchImgHeight = filterEntry->match_img_height;
            newResult.numCompared = filterEntry->num_compared;
            newResult.numMatched = filterEntry->num_matched;
            newResult.percentageMatched
                = float(newResult.numMatched) / float(newResult.numCompared) * 100.0f;
            newResult.isMatched
                = newResult.percentageMatched >= matchConfig(i).totalMatchThresh;
        }
        pthread_mutex_unlock(&filterData->mutex);
    }

    // store new results
    {
        QMutexLocker resLocker(&m_resultsMutex);
        m_results = newResults;
    }
    
    for (size_t i = 0; i < newResults.size(); ++i) {
         emit sigNewMatchResults(i, newResults[i]);
    }

    for (size_t i = 0; i < m_results.size(); ++i) {
        const auto& resEntry = newResults[i];
        if (resEntry.isMatched) {
            auto cfg = matchConfig(i);
            if (cfg.filterCfg.is_enabled && cfg.matchScene.size()) {
                switchScene(cfg.matchScene, cfg.matchTransition);
                return;
            }
        }
    }
    
    std::string nms = noMatchScene();
    if (nms.size()) {
        switchScene(nms, noMatchTransition());
    }

#if 0
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
#endif
}

// copied (and slightly simplified) from Advanced Scene Switcher:
// https://github.com/WarmUpTill/SceneSwitcher/blob/05540b61a118f2190bb3fae574c48aea1b436ac7/src/advanced-scene-switcher.cpp#L1066
void PmCore::switchScene(
    const std::string &targetSceneName, const std::string &transitionName)
{
    obs_source_t* currSceneSrc = obs_frontend_get_current_scene();
    obs_source_t* targetSceneSrc = nullptr;

    {
        QMutexLocker locker(&m_scenesMutex);
        for (auto scene : m_scenes) {
            auto sceneSrc = obs_weak_source_get_source(scene);
            if (targetSceneName == obs_source_get_name(sceneSrc)) {
                targetSceneSrc = sceneSrc;
                break;
            }
        }
     }

    if (targetSceneSrc && targetSceneSrc != currSceneSrc) {
        obs_source_t* transitionSrc = nullptr;
        if (!transitionName.empty()) {
            auto find = m_availableTransitions.find(transitionName);
            if (find == m_availableTransitions.end()) {
                find = m_availableTransitions.find("Cut");
            }
            if (find != m_availableTransitions.end()) {
                auto weakTransSrc = *find;
                transitionSrc = obs_weak_source_get_source(weakTransSrc);
            }
            //obs_source_release(transitionSrc);
        }
        if (transitionSrc) {
            obs_frontend_set_current_transition(transitionSrc);
        }
        if (targetSceneSrc) {
            obs_frontend_set_current_scene(targetSceneSrc);
        }
    }
    //obs_source_release(currSceneSrc);
    //obs_source_release(targetSceneSrc);
}

void PmCore::supplyImageToFilter(
    struct pm_filter_data* data, size_t matchIdx, const QImage &image)
{
    if (data) {
        pthread_mutex_lock(&data->mutex);
        auto entryData = data->match_entries + matchIdx;

        size_t sz = size_t(image.sizeInBytes());
        if (sz) {
            entryData->match_img_data = bmalloc(sz);
            memcpy(entryData->match_img_data, image.bits(), sz);
        } else {
            entryData->match_img_data = NULL;
        }

        entryData->match_img_width = uint32_t(image.width());
        entryData->match_img_height = uint32_t(image.height());
        pthread_mutex_unlock(&data->mutex);
    }
}

void PmCore::pmSave(obs_data_t *saveData)
{
    obs_data_t *saveObj = obs_data_create();

    // match configuration and presets
    {
        QMutexLocker locker(&m_matchConfigMutex);

        // match presets
        obs_data_array_t *matchPresetArray = obs_data_array_create();
        for (const auto &p: m_matchPresets) {
            std::string presetName = p.first;
            PmMultiMatchConfig presetCfg = p.second;
            obs_data_t *matchPresetObj = presetCfg.save(presetName);
            obs_data_array_push_back(matchPresetArray, matchPresetObj);
            obs_data_release(matchPresetObj);
        }
        obs_data_set_array(saveObj, "match_presets", matchPresetArray);
        obs_data_array_release(matchPresetArray);

        // active match config/preset
        obs_data_t *activeCfgObj;
        if (m_activeMatchPreset.empty() || matchConfigDirty()) {
            activeCfgObj = m_multiMatchConfig.save("");
        } else {
            PmMultiMatchConfig presetCfg = m_matchPresets[m_activeMatchPreset];
            activeCfgObj = presetCfg.save(m_activeMatchPreset);
        }
        obs_data_set_obj(saveObj, "match_config", activeCfgObj);
        obs_data_release(activeCfgObj);
    }

    // preview config
    {
        QMutexLocker locker(&m_previewConfigMutex);
        obs_data_t* previewCfgObj = m_previewConfig.save();
        obs_data_set_obj(saveObj, "preview_config", previewCfgObj);
        obs_data_release(previewCfgObj);
    }

    obs_data_set_obj(saveData, "pixel-match-switcher", saveObj);
}

void PmCore::pmLoad(obs_data_t *data)
{
    obs_data_t *loadObj = obs_data_get_obj(data, "pixel-match-switcher");

    m_selectedMatchIndex = 0;

    // match configuration and presets
    {
        // match presets
        m_matchPresets.clear();
        obs_data_array_t *matchPresetArray
            = obs_data_get_array(loadObj, "match_presets");
        size_t count = obs_data_array_count(matchPresetArray);
        for (size_t i = 0; i < count; ++i) {
            obs_data_t *matchPresetObj
                = obs_data_array_item(matchPresetArray, i);
            std::string presetName
                = obs_data_get_string(matchPresetObj, "name");
            m_matchPresets[presetName] = PmMultiMatchConfig(matchPresetObj);
            obs_data_release(matchPresetObj);
        }
        obs_data_array_release(matchPresetArray);

        // active match config/preset
        obs_data_t *activeCfgObj = obs_data_get_obj(loadObj, "match_config");
        std::string activePresetName
            = obs_data_get_string(activeCfgObj, "name");
        if (activePresetName.size()) {
            onSelectActiveMatchPreset(activePresetName);
        } else {
            PmMultiMatchConfig activeCfg(activeCfgObj);
            onResetMatchConfigs();
            for (size_t i = 0; i < activeCfg.size(); ++i) {
                onInsertMatchConfig(i, activeCfg[i]);
            }
            onNoMatchSceneChanged(activeCfg.noMatchScene);
            onNoMatchTransitionChanged(activeCfg.noMatchTransition);
        }
        obs_data_release(activeCfgObj);
    }

    // preview configuration
    {
        obs_data_t* previewCfgObj = obs_data_get_obj(loadObj, "preview_config");
        PmPreviewConfig previewCfg(previewCfgObj);
        onPreviewConfigChanged(previewCfg);
        obs_data_release(previewCfgObj);
    }
    

    obs_data_release(loadObj);
}

#if 0
PmSwitchConfig PmCore::switchConfig() const
{
    QMutexLocker locker(&m_switchConfigMutex);
    return m_switchConfig;
}
#endif

#if 0
switch (m_matchConfig.maskMode) {
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
    uint8_t* colorBytes = reinterpret_cast<uint8_t*>(
        &m_matchConfig.customColor);
#if PM_LITTLE_ENDIAN
    vec3_set(&maskColor,
        float(colorBytes[2]) / 255.f,
        float(colorBytes[1]) / 255.f,
        float(colorBytes[0]) / 255.f);
#else
    vec3_set(&maskColor,
        float(colorBytes[1]) / 255.f,
        float(colorBytes[2]) / 255.f,
        float(colorBytes[3]) / 255.f);
#endif
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
#endif