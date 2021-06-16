#include "pm-core.hpp"

#include <ostream>
#include <sstream>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs-data.h>

#include <QAction>
#include <QMainWindow>
#include <QApplication>
#include <QTimer>
#include <QThread>
#include <QTextStream>
#include <QMessageBox>

#include <QFile>
#include <QFileInfo>
#include <QDir>

/*
 * @brief Holds the data structures while scene, scene items and filters 
 *        are being scanned
 */
struct PmSceneScanInfo
{
    PmSourceHash scenes;
    PmSceneItemsHash sceneItems;
    PmSourceHash filters;
    QSet<OBSWeakSource> pmFilters;

    PmSourceData *lastSceneData = nullptr;
    PmSceneItemData *lastSiData = nullptr;
};


PmCore* PmCore::m_instance = nullptr;

void init_pixel_match_switcher()
{
    pmRegisterMetaTypes();

    PmCore::m_instance = new PmCore();

    obs_frontend_add_save_callback(
        pm_save_load_callback, static_cast<void*>(PmCore::m_instance));
}

void free_pixel_match_switcher()
{
    delete PmCore::m_instance;
    PmCore::m_instance = nullptr;
}

void pm_save_load_callback(obs_data_t *save_data, bool saving, void *corePtr)
{
    PmCore *core = static_cast<PmCore *>(corePtr);
    if (saving) {
        core->pmSave(save_data);
    } else {
        core->pmLoad(save_data);
    }
}

void on_frame_processed(pm_filter_data *filterData)
{
    auto core = PmCore::m_instance;
    if (core) {
        PmMultiMatchResults newResults;
        pthread_mutex_lock(&filterData->mutex);
        newResults.resize(filterData->num_match_entries);
        for (size_t i = 0; i < newResults.size(); ++i) {
            auto &newResult = newResults[i];
            const auto &filterEntry = filterData->match_entries + i;
            newResult.baseWidth = filterData->base_width;
            newResult.baseHeight = filterData->base_height;
            newResult.matchImgWidth = filterEntry->match_img_width;
            newResult.matchImgHeight = filterEntry->match_img_height;
            newResult.numCompared = filterEntry->num_compared;
            newResult.numMatched = filterEntry->num_matched;
        }
        pthread_mutex_unlock(&filterData->mutex);
        emit core->sigFrameProcessed(newResults);
    }
}

void on_settings_button_released()
{
    auto core = PmCore::m_instance;
    if (core) {
        core->onMenuAction();
    }
}

void on_match_image_captured(pm_filter_data *filterData)
{
    auto core = PmCore::m_instance;
    if (core) {
        pthread_mutex_lock(&filterData->mutex);
        int roiLeft = std::min(core->m_captureStartX, core->m_captureEndX);
        int roiBottom = std::min(core->m_captureStartY, core->m_captureEndY);
        int roiRight = std::max(core->m_captureStartX, core->m_captureEndX);
        int roiTop = std::max(core->m_captureStartY, core->m_captureEndY);

        QImage matchImg(filterData->captured_region_data,
                   roiRight - roiLeft + 1, roiTop - roiBottom + 1,
                   QImage::Format_RGBA8888);

        emit core->sigMatchImageCaptured(matchImg.copy(), roiLeft, roiBottom);
        bfree(filterData->captured_region_data);
        filterData->captured_region_data = nullptr;
        filterData->filter_mode = PM_MATCH;

        pthread_mutex_unlock(&filterData->mutex);
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
        if (src) {
            auto weakSrc = obs_source_get_weak_source(src);
            auto name = obs_source_get_name(src);
            ret.insert(name, weakSrc);
            obs_weak_source_release(weakSrc);
        }
    }
    obs_frontend_source_list_free(&transitionList);
    return ret;
}

PmCore::PmCore()
: m_filtersMutex(QMutex::Recursive)
, m_matchConfigMutex(QMutex::Recursive)
, m_scenesMutex(QMutex::Recursive)
{
    // add action item in the Tools menu of the app
    auto action = static_cast<QAction*>(
        obs_frontend_add_tools_menu_qaction(
            obs_module_text("Pixel Match Switcher")));
    connect(action, &QAction::triggered, this, &PmCore::onMenuAction,
            Qt::DirectConnection);

    // periodic update timer: process in the UI thread
    m_periodicUpdateTimer = new QTimer(qApp);
    connect(m_periodicUpdateTimer, &QTimer::timeout,
            this, &PmCore::onPeriodicUpdate, Qt::QueuedConnection);

    // fast reaction signal/slot: process in the core's thread
    connect(this, &PmCore::sigFrameProcessed,
            this, &PmCore::onFrameProcessed, Qt::QueuedConnection);

    // move to own thread
    m_thread = new QThread(this);
    m_thread->setObjectName("pixel match core thread");

    if (m_runningEnabled) {
        activate();
    } else {
        // only start the timer for the sake of fetching transitions
        m_periodicUpdateTimer->start();
    }
}

PmCore::~PmCore()
{
    deactivate();
    m_thread->exit();
    while (m_thread->isRunning() || m_periodicUpdateActive) {
        QThread::msleep(1);
    }

    if (m_dialog) {
        m_dialog->deleteLater();
    }
    if (m_periodicUpdateTimer) {
        m_periodicUpdateTimer->deleteLater();
    }
}

void PmCore::activate()
{
    m_runningEnabled = true;

    auto cfgCpy = multiMatchConfig();
    activateMultiMatchConfig(cfgCpy);

    // fire up the engines
    m_periodicUpdateTimer->start(100);
    m_thread->start();
    moveToThread(m_thread);
}

void PmCore::deactivate()
{
    auto previewCfg = previewConfig();
    previewCfg.previewMode = PmPreviewMode::Video;
    emit sigPreviewConfigChanged(previewCfg);

    m_runningEnabled = false;
    m_periodicUpdateTimer->stop();

    PmFilterRef oldAfr;
    {
        QMutexLocker locker(&m_filtersMutex);
        oldAfr = m_activeFilter;
        m_activeFilter.reset();
        activeFilterChanged();
    }
    if (oldAfr.isValid()) {
        auto data = oldAfr.filterData();
        if (data) {
            pm_resize_match_entries(data, 0);
            oldAfr.lockData();
            data->on_frame_processed = nullptr;
            data->on_match_image_captured = nullptr;
            oldAfr.unlockData();
        }
    }
    {
        QMutexLocker locker(&m_matchImagesMutex);
        for (size_t i = 0; i < m_matchImages.size(); ++i) {
            m_matchImages[i] = QImage();
        }
    }
    {
        QMutexLocker locker(&m_resultsMutex);
        for (size_t i = 0; i < m_results.size(); ++i) {
            emit sigNewMatchResults(i, PmMatchResults());
        }
        m_results.clear();
    }

    moveToThread(QApplication::instance()->thread());
    m_thread->exit();
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

std::string PmCore::matchConfigLabel(size_t matchIdx) const
{
	QMutexLocker locker(&m_matchConfigMutex);
	return matchIdx < m_multiMatchConfig.size()
        ? m_multiMatchConfig[matchIdx].label : "";
}

bool PmCore::hasAction(size_t matchIdx, PmActionType actionType) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return matchIdx < m_multiMatchConfig.size()
        ? m_multiMatchConfig[matchIdx].reaction.hasAction(actionType) : false;
}

bool PmCore::hasMatchAction(size_t matchIdx, PmActionType actionType) const
{
	QMutexLocker locker(&m_matchConfigMutex);
    return matchIdx < m_multiMatchConfig.size()
        ? m_multiMatchConfig[matchIdx].reaction.hasMatchAction(actionType)
        : false;
}

bool PmCore::hasUnmatchAction(size_t matchIdx, PmActionType actionType) const
{
	QMutexLocker locker(&m_matchConfigMutex);
    return matchIdx < m_multiMatchConfig.size()
        ? m_multiMatchConfig[matchIdx].reaction.hasUnmatchAction(actionType)
        : false;
}

bool PmCore::hasGlobalMatchAction(PmActionType actionType) const
{
	QMutexLocker locker(&m_matchConfigMutex);
	return m_multiMatchConfig.noMatchReaction.hasMatchAction(actionType);
}

bool PmCore::hasGlobalUnmatchAction(PmActionType actionType) const
{
	QMutexLocker locker(&m_matchConfigMutex);
	return m_multiMatchConfig.noMatchReaction.hasUnmatchAction(actionType);
}

PmReaction PmCore::reaction(size_t matchIdx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return matchIdx < m_multiMatchConfig.size()
               ? m_multiMatchConfig[matchIdx].reaction
               : PmReaction();
}

PmReaction PmCore::noMatchReaction() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_multiMatchConfig.noMatchReaction;
}

std::string PmCore::matchImgFilename(size_t matchIdx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (matchIdx >= m_multiMatchConfig.size())
        return std::string();
    else 
        return m_multiMatchConfig[matchIdx].matchImgFilename;
}

bool PmCore::hasFilename(size_t matchIdx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (matchIdx >= m_multiMatchConfig.size())
        return false;
    else
        return m_multiMatchConfig[matchIdx].matchImgFilename.size() > 0;
}

bool PmCore::matchConfigCanMoveUp(size_t idx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    size_t sz = m_multiMatchConfig.size();

    if (sz <= 1 || idx == 0 || idx >= sz) return false;

    if (!hasSceneAction(idx - 1) && hasSceneAction(idx))
        return false;

    return true;
}

bool PmCore::matchConfigCanMoveDown(size_t idx) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    size_t sz = m_multiMatchConfig.size();

    if (sz <= 1 || idx >= sz-1) return false;

    if (!hasSceneAction(idx) && hasSceneAction(idx + 1))
        return false;
 
    return true;
}

bool PmCore::enforceTargetOrder(size_t matchIdx, const PmMatchConfig &newCfg)
{
    size_t sz = multiMatchConfigSize();

    if (!newCfg.reaction.hasAction(PmActionType::ANY)) return false;

    // enforce reaction type order
    if (m_enforceReactionTypeOrder && sz > 1) {
        if (!newCfg.reaction.hasSceneAction()
          && matchIdx < sz-1
          && hasSceneAction(matchIdx + 1)) {
            // remove + insert to enforce scenes after scene items
            onMatchConfigRemove(matchIdx);
            while (matchIdx < sz-1 && hasSceneAction(matchIdx)) {
                matchIdx++;
            }
            onMatchConfigInsert(matchIdx, newCfg);
	        onMatchConfigSelect(matchIdx);
            return true;
        } else if (newCfg.reaction.hasSceneAction()
                && matchIdx > 0
                && !hasSceneAction(matchIdx - 1)) {
            // remove + insert to enforce scene items before scenes
            onMatchConfigRemove(matchIdx);
            while (matchIdx > 0 && !hasSceneAction(matchIdx -1)) {
                matchIdx--;
            }
            onMatchConfigInsert(matchIdx, newCfg);
    	    onMatchConfigSelect(matchIdx);
            return true;
        }
    }
    return false;
}

void PmCore::onMatchConfigChanged(size_t matchIdx, PmMatchConfig newCfg)
{
    size_t sz = multiMatchConfigSize();

    // invalid index?
    if (matchIdx >= sz) return; 

    PmMatchConfig oldCfg = matchConfig(matchIdx);

     // config hasn't changed? stop callback loops
    if (oldCfg == newCfg) return;

    // if change breaks the order of target reaction types, do things differently
    if (enforceTargetOrder(matchIdx, newCfg)) return;

    // go forward with setting new config state and activation
    activateMatchConfig(matchIdx, newCfg);

    {
        // check for orphaned images
        QMutexLocker locker(&m_matchConfigMutex);
        if (oldCfg.wasDownloaded && oldCfg.matchImgFilename.size()
         && oldCfg.matchImgFilename != newCfg.matchImgFilename
         && !m_multiMatchConfig.containsImage(oldCfg.matchImgFilename, matchIdx)
         && !m_matchPresets.containsImage(oldCfg.matchImgFilename)) {
            emit sigMatchImagesOrphaned({oldCfg.matchImgFilename});
        }
    }
}

void PmCore::onMatchConfigInsert(size_t matchIndex, PmMatchConfig cfg)
{
    size_t oldSz = multiMatchConfigSize();
    if (matchIndex > oldSz) matchIndex = oldSz;
    size_t newSz = oldSz + 1;
    
    // notify other modules about the new size
    emit sigMultiMatchConfigSizeChanged(newSz);
    
    // reconfigure the filter
    if (m_runningEnabled) {
        auto fr = activeFilterRef();
        auto filterData = fr.filterData();
        if (filterData) {
            pm_resize_match_entries(filterData, newSz);
        }
    }

    // reconfigure images
    {
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages.insert(m_matchImages.begin() + int(matchIndex), QImage());
    }
    
    // finish updating state and notifying
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig.resize(newSz);
    }
    for (size_t i = newSz - 1; i > matchIndex; i--) {
        activateMatchConfig(i, m_multiMatchConfig[i-1]);
    }
    activateMatchConfig(matchIndex, cfg);
    
    emit sigActivePresetDirtyChanged();
}

void PmCore::onMatchConfigRemove(size_t matchIndex)
{
    size_t oldSz = multiMatchConfigSize();
    if (matchIndex >= oldSz) return;
    size_t newSz = oldSz - 1;

    // notify other modules
    emit sigMultiMatchConfigSizeChanged(newSz);

    // reconfigure the filter
    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        pm_resize_match_entries(filterData, newSz);
    }

    {
        // reconfigure images
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages.erase(m_matchImages.begin() + int(matchIndex));
    }

    // check for an orphaned image
    QList<std::string> orphanedImages;
    {
        QMutexLocker locker(&m_matchConfigMutex);
        const PmMatchConfig &cfg = m_multiMatchConfig[matchIndex];
        if (cfg.wasDownloaded
         && !m_multiMatchConfig.containsImage(cfg.matchImgFilename, matchIndex)
         && !m_matchPresets.containsImage(cfg.matchImgFilename)) {
            orphanedImages = {cfg.matchImgFilename};
        }
    }

    // finish updating state and notifying
    for (size_t i = matchIndex; i < newSz; ++i) {
        onMatchConfigChanged(i, matchConfig(i + 1));
    }
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig.resize(newSz);
        emit sigActivePresetDirtyChanged();
    }
    onMatchConfigSelect(matchIndex);

    // notify about orphaned images
    if (orphanedImages.size())
        emit sigMatchImagesOrphaned(orphanedImages);
}

void PmCore::onMultiMatchConfigReset()
{
    resetMultiMatchConfig();
}

void PmCore::onMatchConfigMoveUp(size_t matchIndex)
{
    if (matchIndex < 1 || matchIndex >= multiMatchConfigSize()) return;

    PmMatchConfig oldAbove = matchConfig(matchIndex - 1);
    PmMatchConfig oldBelow = matchConfig(matchIndex);
    onMatchConfigChanged(matchIndex - 1, oldBelow);
    onMatchConfigChanged(matchIndex, oldAbove);

    if (matchIndex == m_selectedMatchIndex)
        onMatchConfigSelect(matchIndex - 1);
}

void PmCore::onMatchConfigMoveDown(size_t matchIndex)
{
    if (matchIndex >= multiMatchConfigSize() - 1) return;

    PmMatchConfig oldAbove = matchConfig(matchIndex);
    PmMatchConfig oldBelow = matchConfig(matchIndex + 1);
    onMatchConfigChanged(matchIndex + 1, oldAbove);
    onMatchConfigChanged(matchIndex, oldBelow);

    if (matchIndex == m_selectedMatchIndex)
        onMatchConfigSelect(matchIndex + 1);
}

void PmCore::onMatchConfigSelect(size_t matchIndex)
{   
    onCaptureStateChanged(PmCaptureState::Inactive);

    m_selectedMatchIndex = matchIndex;

    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        fr.lockData();
        filterData->selected_match_index = matchIndex;
        fr.unlockData();
    }
    
    if (!matchImageLoaded(matchIndex)) {
        auto previewCfg = previewConfig();
        previewCfg.previewMode = PmPreviewMode::Video;
        onPreviewConfigChanged(previewCfg);
    }

    auto selCfg = matchConfig(matchIndex);
    emit sigMatchConfigSelect(matchIndex, selCfg);
}

void PmCore::onNoMatchReactionChanged(PmReaction noMatchReaction)
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (m_multiMatchConfig.noMatchReaction != noMatchReaction) {
        m_multiMatchConfig.noMatchReaction = noMatchReaction;
        emit sigNoMatchReactionChanged(m_multiMatchConfig.noMatchReaction);
        emit sigActivePresetDirtyChanged();
    }
}

void PmCore::onPreviewConfigChanged(PmPreviewConfig cfg)
{
    QMutexLocker locker(&m_previewConfigMutex);

    if (m_previewConfig == cfg) return;

    if (cfg.previewMode != PmPreviewMode::Video)
        onCaptureStateChanged(PmCaptureState::Inactive);

    m_previewConfig = cfg;
    emit sigPreviewConfigChanged(cfg);
}

void PmCore::onMatchImageRefresh(size_t matchIndex)
{
    loadImage(matchIndex);
}

void PmCore::onMatchImagesRemove(QList<std::string> orphanedImages)
{
    for (const auto &imgFilename : orphanedImages) {
        QFile file(imgFilename.data());
        QFileInfo info(file);
        if (info.exists()) {
            file.remove();
            QDir dir = info.dir();
            if (dir.isEmpty()) {
                dir.removeRecursively();
            }
        }
    }
}

std::string PmCore::activeMatchPresetName() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_activeMatchPreset;
}

bool PmCore::matchPresetExists(const std::string& name) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets.find(name) != m_matchPresets.end();
}

PmMultiMatchConfig PmCore::matchPresetByName(const std::string& name) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets.value(name, PmMultiMatchConfig());
}

size_t PmCore::matchPresetSize(const std::string& name) const
{
    QMutexLocker locker(&m_matchConfigMutex);
    auto find = m_matchPresets.find(name);
    if (find == m_matchPresets.end()) {
        return 0;
    } else {
        return find->size();
    }
}

QList<std::string> PmCore::matchPresetNames() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    return m_matchPresets.keys();
}

bool PmCore::matchConfigDirty() const
{
    QMutexLocker locker(&m_matchConfigMutex);
    if (m_activeMatchPreset.empty()) {
        for (size_t i = 0; i < m_multiMatchConfig.size(); ++i) {
            if (m_multiMatchConfig[i] != PmMatchConfig()) {
                return true;
            }
        }
        return m_multiMatchConfig.noMatchReaction != PmReaction();
    } else {
        const PmMultiMatchConfig& presetCfg 
            = m_matchPresets[m_activeMatchPreset];
        return presetCfg != m_multiMatchConfig;
    }
}

void PmCore::onMatchPresetSave(std::string name)
{
    bool isNew = !matchPresetExists(name);
    QSet<std::string> orphanedImages;
    {
        QMutexLocker locker(&m_matchConfigMutex);

        PmMatchPresets newPresets = m_matchPresets;
        newPresets[name] = m_multiMatchConfig;

        if (m_matchPresets.contains(name)) {
            PmMultiMatchConfig oldMcfg = m_matchPresets[name];
            orphanedImages = newPresets.orphanedImages(oldMcfg);
        }

        m_matchPresets = newPresets;
    }
    if (isNew) {
        emit sigAvailablePresetsChanged();
    }
    onMatchPresetSelect(name);
    emit sigActivePresetDirtyChanged();
    if (orphanedImages.size()) {
        emit sigMatchImagesOrphaned(orphanedImages.toList());
    }

    obs_frontend_save();
}


void PmCore::onMatchPresetSelect(std::string name)
{
    PmMultiMatchConfig multiConfig;
    {
        QMutexLocker locker(&m_matchConfigMutex);
        if (m_activeMatchPreset == name) return;
        m_activeMatchPreset = name;

        multiConfig = name.size() ? m_matchPresets[name] : PmMultiMatchConfig();
    }
    emit sigActivePresetChanged();
    activateMultiMatchConfig(multiConfig);
}

void PmCore::onMatchPresetRemove(std::string name)
{
    std::string selOther;
    QSet<std::string> orphanedImages;
    {
        QMutexLocker locker(&m_matchConfigMutex);
        if (m_matchPresets.empty()) return;

        // stage reduced presets
        PmMatchPresets newPresets = m_matchPresets;
        newPresets.remove(name);

        // check for orphaned images
        PmMultiMatchConfig mcfgRemoved = m_matchPresets[name];
        auto orphanedImages = newPresets.orphanedImages(
            mcfgRemoved, &m_multiMatchConfig);

        // pick a new selection, when needed
        if (m_activeMatchPreset == name && newPresets.size()) {
            selOther = newPresets.keys().first();
        }

        // finalize data
        m_matchPresets = newPresets;
    }

    emit sigAvailablePresetsChanged();
    onMatchPresetSelect(selOther);
    if (orphanedImages.size()) {
        emit sigMatchImagesOrphaned(orphanedImages.toList());
    }
}

void PmCore::onMatchPresetActiveRevert()
{
    std::string activeName = activeMatchPresetName();
    onMatchPresetSelect("");
    onMatchPresetSelect(activeName);
}

void PmCore::onMatchPresetExport(
    std::string filename,  QList<std::string> selectedPresets)
{
    try {
        m_matchPresets.exportXml(filename, selectedPresets);
    } catch (const std::exception &e) {
        blog(LOG_ERROR, "Preset Export Failed: %s", e.what());
        emit sigShowException(
            obs_module_text("Preset Export Failed"), e.what());
    } catch (...) {
        blog(LOG_ERROR, "Preset Export Failed: Unknown error.");
        emit sigShowException(
            obs_module_text("Preset Export Failed"),
            obs_module_text("Unknown error."));
    }
}

void PmCore::onMatchPresetsImport(std::string filename)
{
    try {
        PmMatchPresets impPresets(filename);
        emit sigPresetsImportAvailable(impPresets);
    } catch (const std::exception &e) {
        blog(LOG_ERROR, "Preset Import Failed: %s", e.what());
        emit sigShowException(
            obs_module_text("Preset Import Failed"), e.what());
    } catch (...) {
        blog(LOG_ERROR, "Preset Import Failed: Uknown error.");
        emit sigShowException(
            obs_module_text("Preset Import Failed"),
            obs_module_text("Unknown error."));
    }
}

void PmCore::onMatchPresetsAppend(PmMatchPresets newPresets)
{
    for (const auto &name : newPresets.keys()) {
        m_matchPresets.insert(name, newPresets[name]);
        if (name == activeMatchPresetName())
            onMatchPresetActiveRevert();
    }
    emit sigAvailablePresetsChanged();
    emit sigActivePresetDirtyChanged();
    obs_frontend_save();
}

void PmCore::onRunningEnabledChanged(bool enable)
{
    if (m_runningEnabled != enable) {
        if (enable) {
            activate();
        } else {
            deactivate();
        }
        sigRunningEnabledChanged(enable);
    }
}

void PmCore::onSwitchingEnabledChanged(bool enable)
{
    if (m_switchingEnabled != enable) {
        m_switchingEnabled = enable;
        emit sigSwitchingEnabledChanged(enable);
    }
}

void PmCore::onCaptureStateChanged(PmCaptureState state, int x, int y)
{
    if (m_captureState == PmCaptureState::Inactive
     && state == PmCaptureState::Inactive) 
        return;

    // TODO: verify state transitions
    PmCaptureState prevState = m_captureState;
    m_captureState = state;

    if (state != PmCaptureState::Inactive) {
        auto previewCfg = previewConfig();
        if (previewCfg.previewMode != PmPreviewMode::Video) {
            previewCfg.previewMode = PmPreviewMode::Video;
            onPreviewConfigChanged(previewCfg);
        }
    }

    PmFilterRef filter;
    pm_filter_data *filterData;

    switch(state) {
    case PmCaptureState::Inactive:
        m_captureStartX = m_captureStartY = 0;
        m_captureEndX = m_captureEndY = 0;
        filter = activeFilterRef();
        filterData = filter.filterData();
        if (filterData) {
            filter.lockData();
            filterData->select_left = 0;
            filterData->select_bottom = 0;
            filterData->select_right = 0;
            filterData->select_top = 0;
            filter.unlockData();
        }
        break;
    case PmCaptureState::Activated:
        break;
    case PmCaptureState::SelectBegin:
        m_captureStartX = x;
        m_captureStartY = y;
        break;
    case PmCaptureState::SelectMoved:
    case PmCaptureState::SelectFinished:
        if (x >= 0 && y >= 0) {
            filter = activeFilterRef();
            filterData = filter.filterData();
            if (filterData) {
                filter.lockData();
                x = std::min(x, int(filterData->base_width - 1));
                y = std::min(y, int(filterData->base_height - 1));
                m_captureEndX = x;
                m_captureEndY = y;
                filterData->select_left = (uint32_t)(
                    std::min(m_captureStartX, m_captureEndX));
                filterData->select_bottom = (uint32_t)(
                    std::min(m_captureStartY, m_captureEndY));
                filterData->select_right = (uint32_t)(
                    std::max(m_captureStartX, m_captureEndX));
                filterData->select_top = (uint32_t)(
                    std::max(m_captureStartY, m_captureEndY));
                filter.unlockData();
            }
        }
        break;
    case PmCaptureState::Accepted:
        filter = activeFilterRef();
        filterData = filter.filterData();
        if (filterData) {
            filter.lockData();
            if (prevState == PmCaptureState::Automask)
                filterData->filter_mode = PM_MASK_END;
            else
                filterData->filter_mode = PM_SNAPSHOT;
            filter.unlockData();
        }
        break;
    case PmCaptureState::Automask:
        filter = activeFilterRef();
        filterData = filter.filterData();
        if (filterData) {
            filter.lockData();
            filterData->filter_mode = PM_MASK_BEGIN;
            filter.unlockData();
        }
    }

    emit sigCaptureStateChanged(m_captureState, x, y);
}

PmSourceHash PmCore::scenes() const
{
    QMutexLocker locker(&m_scenesMutex);
    return m_scenes;
}

QList<std::string> PmCore::sceneNames() const
{
    QMutexLocker locker(&m_scenesMutex);
    return m_scenes.keys();
}

QList<std::string> PmCore::sceneItemNames(const std::string &sceneName) const
{
    QMutexLocker locker(&m_scenesMutex);
    return m_scenes[sceneName].childNames;
}

PmSceneItemsHash PmCore::sceneItems() const
{
    QMutexLocker locker(&m_scenesMutex);
    return m_sceneItems;
}

QList<std::string> PmCore::allFilterNames() const
{
    QMutexLocker locker(&m_scenesMutex);
    QList<std::string> ret;
    for (const std::string &siName : m_sceneItems.keys()) {
        ret.append(filterNames(siName));
    }
    return ret;
}

QList<std::string> PmCore::filterNames(const std::string &sceneItemName) const
{
    QMutexLocker locker(&m_scenesMutex);
    auto find = m_sceneItems.find(sceneItemName);
    if (find == m_sceneItems.end()) {
        return QList<std::string>();
    } else {
        return find->filtersNames;
    }
}

QList<std::string> PmCore::sceneItemNames() const
{
    QMutexLocker locker(&m_scenesMutex);
    return m_sceneItems.keys();
}

QImage PmCore::matchImage(size_t matchIdx) const
{ 
    QMutexLocker locker(&m_matchImagesMutex);
    if (matchIdx >= m_matchImages.size())
        return QImage();
    else
        return m_matchImages[matchIdx]; 
}

bool PmCore::matchImageLoaded(size_t matchIdx) const
{
    QMutexLocker locker(&m_matchImagesMutex);

    if (matchIdx >= m_matchImages.size())
        return false;
    else 
        return !m_matchImages[matchIdx].isNull();
}

void PmCore::getCaptureEndXY(int& x, int& y) const
{
    x = m_captureEndX;
    y = m_captureEndY;
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

    PmSceneScanInfo scanInfo;

    for (size_t i = 0; i < scenesInput.sources.num; ++i) {
        auto &sceneSrc = scenesInput.sources.array[i];
        auto sceneName = obs_source_get_name(sceneSrc);

        // map scene names to scene refs
        obs_weak_source_t *sceneWs = obs_source_get_weak_source(sceneSrc);
        OBSWeakSource sceneWsWs(sceneWs);
        obs_weak_source_release(sceneWs);

        // enumerate scene items
        obs_scene_t *scene = obs_scene_from_source(sceneSrc);
        PmSourceData sceneData = {sceneWsWs};
        scanInfo.lastSceneData = &sceneData;
        obs_scene_enum_items(
            scene,
            [](obs_scene_t *scene, obs_sceneitem_t *item,
               void *p) -> bool {
                PmSceneScanInfo *scanInfo = (PmSceneScanInfo *)p;

                //obs_source_t *sceneSrc = obs_scene_get_source(scene);
                //std::string sceneName = obs_source_get_name(sceneSrc);
                obs_source_t *siSrc = obs_sceneitem_get_source(item);
                std::string siName = obs_source_get_name(siSrc);

                // map scenes to scene items
                scanInfo->lastSceneData->childNames.push_back(siName);

                // map scene items to scene item refs
                OBSSceneItem sceneItemSi(item);
                PmSceneItemData siData{sceneItemSi, {}};
                scanInfo->lastSiData = &siData;
                scanInfo->sceneItems.insert(siName, siData);

                // enumerate filters
                obs_source_enum_filters(
                    siSrc,
                    [](obs_source_t *parent, obs_source_t *filterSrc, void *p) {
                        PmSceneScanInfo *scanInfo = (PmSceneScanInfo *)p;
                        std::string fiName = obs_source_get_name(filterSrc);

                        // map filter names to filter refs
                        obs_weak_source_t *filterWs
                            = obs_source_get_weak_source(filterSrc);
                        OBSWeakSource filterWsWs(filterWs);
                        obs_weak_source_release(filterWs);
                        scanInfo->filters.insert(fiName, {filterWsWs});

                        // map scene items to filters
                        scanInfo->lastSiData->filtersNames.push_back(fiName);

                        // locate pixel match filters
                        auto id = obs_source_get_id(filterSrc);
                        if (!strcmp(id, PIXEL_MATCH_FILTER_ID)) {
                            if (obs_obj_get_data(filterSrc)) {
                                scanInfo->pmFilters.insert(filterWsWs);
                            }
                        }
                        UNUSED_PARAMETER(parent);
                    },
                    scanInfo);
                scanInfo->sceneItems.insert(siName, siData);
                return true;
            },
            &scanInfo);
        scanInfo.scenes.insert(sceneName, sceneData);
    }

    obs_frontend_source_list_free(&scenesInput);

    updateActiveFilter(scanInfo.pmFilters);

    bool scenesChanged = false;
    bool sceneItemsChanged = false;
    QSet<std::string> oldSceneNames;
    QSet<std::string> oldSceneItemNames;
    QSet<std::string> oldFilterNames;
    {
        QMutexLocker locker(&m_scenesMutex);
        if (m_scenes != scanInfo.scenes) {
            oldSceneNames = m_scenes.sourceNames();
            scenesChanged = true;
        }
        if (m_sceneItems != scanInfo.sceneItems) {
            oldSceneItemNames = m_sceneItems.sceneItemNames();
            sceneItemsChanged = true;
        }
        if (m_filters != scanInfo.filters) {
            oldFilterNames = m_filters.sourceNames();
            sceneItemsChanged = true;
        }
    }

    if (scenesChanged || sceneItemsChanged) {
        emit sigScenesChanged(scanInfo.scenes.keys(), scanInfo.sceneItems.keys());
    }

    // adjust for scene changes
    if (scenesChanged) {
        QMutexLocker locker(&m_scenesMutex);
        size_t cfgSize = multiMatchConfigSize();
        for (const std::string& oldSceneName : oldSceneNames) {
            if (!scanInfo.scenes.contains(oldSceneName)) {
                auto ws = m_scenes[oldSceneName];
                std::string newSceneName = scanInfo.scenes.key(ws);
                if (m_multiMatchConfig.noMatchReaction.hasSceneAction()) {
                    auto noMatchReaction = m_multiMatchConfig.noMatchReaction;
                    if (noMatchReaction.renameElement(
                        PmActionType::Scene, oldSceneName, newSceneName)) {
                            onNoMatchReactionChanged(noMatchReaction);
                    }
                }
                for (size_t i = 0; i < cfgSize; i++) {
                    if (hasSceneAction(i)) {
                        PmMatchConfig cfg = m_multiMatchConfig[i];
                        if (cfg.reaction.renameElement(
                            PmActionType::Scene, oldSceneName, newSceneName)) {
                            onMatchConfigChanged(i, cfg);
                        }
                    }
                }
            }
        }
    }

    // adjust for scene item and filter changes
    if (sceneItemsChanged) {
        QMutexLocker locker(&m_scenesMutex);
        size_t cfgSize = multiMatchConfigSize();

        // scene items
        auto newSiNames = scanInfo.sceneItems.keys();
        for (const std::string& oldSiName : oldSceneItemNames) {
            if (!newSiNames.contains(oldSiName)) {
                auto src = m_sceneItems[oldSiName];
                std::string newSiName = scanInfo.sceneItems.key(src);
                if (m_multiMatchConfig.noMatchReaction.hasAction(
                        PmActionType::SceneItem)) {
                    auto noMatchReaction = m_multiMatchConfig.noMatchReaction;
                    if (noMatchReaction.renameElement(
                        PmActionType::SceneItem, oldSiName, newSiName)) {
                            onNoMatchReactionChanged(noMatchReaction);
                    }
                    for (size_t i = 0; i < cfgSize; i++) {
                        if (hasSceneAction(i)) {
                            PmMatchConfig cfg = m_multiMatchConfig[i];
                            if (cfg.reaction.renameElement(
                                PmActionType::SceneItem, oldSiName, newSiName)) {
                                onMatchConfigChanged(i, cfg);
                            }
                        }
                    }
                }
            }
        }

        // filters
        auto newFiNames = scanInfo.filters.keys();
        for (const std::string& oldFiName : oldFilterNames) {
            if (!newFiNames.contains(oldFiName)) {
                auto ws = m_filters[oldFiName];
                std::string newFiName = scanInfo.filters.key(ws);
                if (m_multiMatchConfig.noMatchReaction.hasAction(
                    PmActionType::SceneItem)) {
                    auto noMatchReaction = m_multiMatchConfig.noMatchReaction;
                    if (noMatchReaction.renameElement(
                        PmActionType::SceneItem, oldFiName, newFiName)) {
                            onNoMatchReactionChanged(noMatchReaction);
                    }
                    for (size_t i = 0; i < cfgSize; i++) {
                        if (hasSceneAction(i)) {
                            PmMatchConfig cfg = m_multiMatchConfig[i];
                            if (cfg.reaction.renameElement(
                                PmActionType::SceneItem, oldFiName, newFiName)) {
                                onMatchConfigChanged(i, cfg);
                            }
                        }
                    }
                }
            }
        }
    }

    // save state
    if (scenesChanged || sceneItemsChanged)
    {
        QMutexLocker locker(&m_scenesMutex);
        m_scenes = scanInfo.scenes;
        m_sceneItems = scanInfo.sceneItems;
        m_filters = scanInfo.filters;
    }
}

void PmCore::updateActiveFilter(
    const QSet<OBSWeakSource> &activeFilters)
{
    using namespace std;
    bool changed = false;

    QMutexLocker locker(&m_filtersMutex);
    if (m_activeFilter.isValid()) {
        if (activeFilters.contains(m_activeFilter.filterWeakSource())) {
            return;
        } else {
            auto data = m_activeFilter.filterData();
            if (data) {
                m_activeFilter.lockData();
                data->on_frame_processed = nullptr;
                data->on_match_image_captured = nullptr;
                m_activeFilter.unlockData();
                pm_resize_match_entries(data, 0);
            }
            m_activeFilter.reset();
            changed = true;
        }
    }
    if (activeFilters.size() > 0) {
        OBSWeakSource newFilter = *(activeFilters.begin());
        m_activeFilter.setFilter(newFilter);
        auto data = m_activeFilter.filterData();
        if (data) {
            size_t cfgSize = multiMatchConfigSize();
            m_activeFilter.lockData();
            data->on_frame_processed = on_frame_processed;
            data->on_match_image_captured = on_match_image_captured;
            data->on_settings_button_released = on_settings_button_released;
            data->selected_match_index = m_selectedMatchIndex;
            m_activeFilter.unlockData();
            pm_resize_match_entries(data, cfgSize);
            for (size_t i = 0; i < cfgSize; ++i) {
                auto cfg = matchConfig(i).filterCfg;
                pm_supply_match_entry_config(data, i, &cfg);
                supplyImageToFilter(data, i, matchImage(i));
            }
        }
        changed = true;
    }

    if (changed) {
        emit activeFilterChanged();
    }
}

void PmCore::activateMatchConfig(
    size_t matchIdx, const PmMatchConfig& newCfg,
    QSet<std::string>* orphanedImages)
{
    // clean linger delay, for safety
    m_sceneLingerQueue.removeAll();
    m_sceneItemLingerList.removeAll();

    auto oldCfg = matchConfig(matchIdx);

    // notify other modules
    emit sigMatchConfigChanged(matchIdx, newCfg);

    // store the new config
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig[matchIdx] = newCfg;
        emit sigActivePresetDirtyChanged();
    }

    // update filter
    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        pm_supply_match_entry_config(
            filterData, matchIdx, &newCfg.filterCfg);
    }

    // update images
    if (m_runningEnabled) {
        if (newCfg.matchImgFilename != oldCfg.matchImgFilename) {
            loadImage(matchIdx);
        }
        if (orphanedImages && oldCfg.matchImgFilename.size()
         && oldCfg.wasDownloaded) {
            orphanedImages->insert(oldCfg.matchImgFilename);
        }
    }

    // force toggle scene items and filters to proper values
    m_forceSceneItemRefresh = true;
}

void PmCore::loadImage(size_t matchIdx)
{
    auto cfg = matchConfig(matchIdx);
    const char* filename = cfg.matchImgFilename.data();
    QImage img(filename);
    if (img.isNull()) {
        blog(LOG_WARNING, "Unable to open filename: %s", filename);
        emit sigMatchImageLoadFailed(matchIdx, filename);
        img = QImage();

        if (matchIdx == m_selectedMatchIndex) {
            auto previewCfg = previewConfig();
            if (previewCfg.previewMode != PmPreviewMode::Video) {
                previewCfg.previewMode = PmPreviewMode::Video;
                onPreviewConfigChanged(previewCfg);
            }
        }
    }
    else {
        img = img.convertToFormat(QImage::Format_ARGB32);
        if (img.isNull()) {
            blog(LOG_WARNING, "Image conversion failed: %s", filename);
            emit sigMatchImageLoadFailed(matchIdx, filename);
            img = QImage();
        }
        else {
            emit sigMatchImageLoadSuccess(matchIdx, filename, img);
        }
    }
    {
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages[matchIdx] = img;
    }

    // update filter
    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        supplyImageToFilter(filterData, matchIdx, img);
    }
}

void PmCore::resetMultiMatchConfig(const PmMultiMatchConfig *newCfg)
{
    // cheeck for orphaned images
    QSet<std::string> orphanedImages;
    {
        QMutexLocker locker(&m_matchConfigMutex);
        orphanedImages
            = m_matchPresets.orphanedImages(m_multiMatchConfig, newCfg);
    }

    // reset no-match reaction
    onNoMatchReactionChanged(PmReaction());

    // notify other modules about changing size
    emit sigMultiMatchConfigSizeChanged(0);

    // reconfigure the filter
    auto fr = activeFilterRef();
    auto filterData = fr.filterData();
    if (filterData) {
        pm_resize_match_entries(filterData, 0);
    }

    // finish updating state and notifying
    {
        QMutexLocker locker(&m_matchConfigMutex);
        m_multiMatchConfig = PmMultiMatchConfig();
        emit sigActivePresetDirtyChanged();
    }
    {
        QMutexLocker locker(&m_matchImagesMutex);
        m_matchImages.clear();
    }
    onMatchConfigSelect(0);

    // report orphaned images
    if (orphanedImages.size()) {
        emit sigMatchImagesOrphaned(orphanedImages.toList());
    }
}

void PmCore::activateMultiMatchConfig(const PmMultiMatchConfig& mCfg)
{
    resetMultiMatchConfig(&mCfg);
    for (size_t i = 0; i < mCfg.size(); ++i) {
        onMatchConfigInsert(i, mCfg[i]);
    }
    onNoMatchReactionChanged(mCfg.noMatchReaction);
    onMatchConfigSelect(0);
}

void PmCore::activeFilterChanged()
{
    if (!m_activeFilter.isValid()) {
        onCaptureStateChanged(PmCaptureState::Inactive);
    }
    emit sigActiveFilterChanged(m_activeFilter);
}

void PmCore::onMenuAction()
{
    // main UI dialog
    if (!m_dialog) {
        // make the dialog parented to the main window
        auto mainWindow
            = static_cast<QMainWindow*>(obs_frontend_get_main_window());
        m_dialog = new PmDialog(this, mainWindow);
    }
    m_dialog->show();
}

void PmCore::onPeriodicUpdate()
{
    m_periodicUpdateActive = true;
    if (m_availableTransitions.empty()) {
        m_availableTransitions = getAvailableTransitions();
    }
    if (m_runningEnabled) {
        scanScenes();
    } else if (m_availableTransitions.size()) {
        m_periodicUpdateTimer->stop();
    }
    m_periodicUpdateActive = false;
}

void PmCore::onFrameProcessed(PmMultiMatchResults newResults)
{
    QTime currTime = QTime::currentTime();

    // expired lingers for scenes will disappear, allowing other scenes
    m_sceneLingerQueue.removeExpired(currTime);

    // expired lingers for scene items
    auto expiredSceneItems = m_sceneItemLingerList.removeExpired(currTime);
    for (size_t siIdx : expiredSceneItems) {
        PmReaction siReaction = reaction(siIdx);
        execIndependentActions(siReaction, false);
    }

    bool sceneSelected = false;
    bool anythingWasMatched = false;
    bool anythingIsMatched = false;

    for (size_t matchIndex = 0; matchIndex < m_results.size(); matchIndex++) {
        if (m_results[matchIndex].isMatched) {
            anythingWasMatched = true;
            break;
        }
    }

    for (size_t matchIndex = 0; matchIndex < newResults.size(); matchIndex++) {
        // assign match state
        auto &newResult = newResults[matchIndex];
        auto cfg = matchConfig(matchIndex);
        newResult.percentageMatched
            = float(newResult.numMatched) / float(newResult.numCompared) * 100.f;
        newResult.isMatched
            = newResult.percentageMatched >= cfg.totalMatchThresh;
        if (cfg.invertResult)
            newResult.isMatched = !newResult.isMatched;

        // notify other modules of the result and match state
        emit sigNewMatchResults(matchIndex, newResults[matchIndex]);

        const PmReaction &reaction = cfg.reaction;

        if (!m_switchingEnabled || !cfg.filterCfg.is_enabled
         || !reaction.isSet()) {
            // no reaction necessary
            continue;
        }

        //uint32_t lingerMs = reaction.lingerMs;
        bool isMatched = newResult.isMatched;
        bool wasMatched = matchResults(matchIndex).isMatched;
        anythingIsMatched |= isMatched;
        bool switched = (isMatched != wasMatched);

        if (switched) {
            execReaction(
                sceneSelected, matchIndex, currTime, reaction, isMatched);
        }
    }

    if (m_switchingEnabled) {
        bool everythingSwitched = (anythingWasMatched != anythingIsMatched);
        const PmReaction &nmr = m_multiMatchConfig.noMatchReaction;
        if (everythingSwitched) {
            // no scene selected due to no match or linger
            if (m_sceneLingerQueue.size()) {
                // a lingering scene takes precedence
                const auto &lingerInfo = m_sceneLingerQueue.top();
                auto reaction = matchConfig(lingerInfo.matchIndex).reaction;
                execSceneAction(lingerInfo.matchIndex, reaction, true);
            } else {
                // nothing matched so we fall back to a no-match scene
                if (!sceneSelected) {
                    execSceneAction(0, nmr, anythingIsMatched);
                }
            }
            execIndependentActions(nmr, anythingIsMatched);
        }
    }

    // store new results
    {
        QMutexLocker resLocker(&m_resultsMutex);
        m_results = newResults;
    }

    if (m_forceSceneItemRefresh)
        m_forceSceneItemRefresh = false;
}

void PmCore::execReaction(
    bool &sceneSelected, size_t matchIdx, const QTime &time,
    const PmReaction &reaction, bool switchedOn)
{
    // no action on cooldown
    if (m_cooldownList.contains(matchIdx)) return;

    // maintain linger info
    if (switchedOn && reaction.lingerMs > 0) {
        m_sceneLingerQueue.push({matchIdx, time});
    }

    // activate cooldown
    if (switchedOn && reaction.cooldownMs > 0) {
        m_cooldownList.push_back({matchIdx, time});
    }

    // activate scene match/unmatch action and take note if switched
    if (!sceneSelected) {
        if (execSceneAction(matchIdx, reaction, switchedOn)) {
            sceneSelected = true;
        }
    }

    // activate independent match/unmatch actions
    execIndependentActions(reaction, switchedOn);
}

// copied (and slightly simplified) from Advanced Scene Switcher:
// https://github.com/WarmUpTill/SceneSwitcher/blob/05540b61a118f2190bb3fae574c48aea1b436ac7/src/advanced-scene-switcher.cpp#L1066
bool PmCore::execSceneAction(
    size_t matchIdx, const PmReaction &reaction, bool switchedOn)
{
    // higher priority lingers? => don't switch
    if (m_sceneLingerQueue.size() > 0
     && m_sceneLingerQueue.top().matchIndex < matchIdx)
        return false;

    // is there a higher priority 
    obs_source_t* currSceneSrc = obs_frontend_get_current_scene();
    obs_source_t* targetSceneSrc = nullptr;
    std::string targetSceneName;
    std::string targetTransition;

    // do the switch
    if (switchedOn) {
        reaction.getMatchScene(targetSceneName, targetTransition);
    } else {
        reaction.getUnmatchScene(targetSceneName, targetTransition);
    }

    if (targetSceneName.empty()) return false;

    {
        QMutexLocker locker(&m_scenesMutex);
        auto find = m_scenes.find(targetSceneName);
        if (find != m_scenes.end()) {
            OBSWeakSource sceneWs = find->wsrc;
            targetSceneSrc = obs_weak_source_get_source(sceneWs);
        }
    }

    if (targetSceneSrc) {
        if (targetSceneSrc != currSceneSrc) {
            obs_source_t* transitionSrc = nullptr;
            if (!targetTransition.empty()) {
                auto find = m_availableTransitions.find(targetTransition);
                if (find != m_availableTransitions.end()) {
                    auto weakTransSrc = *find;
                    transitionSrc = obs_weak_source_get_source(weakTransSrc);
                }
            }

            obs_source_t *currTransition = nullptr;
            if (transitionSrc) {
                currTransition = obs_frontend_get_current_transition();
                obs_frontend_set_current_transition(transitionSrc);
                obs_source_release(transitionSrc);
            }
            obs_frontend_set_current_scene(targetSceneSrc);
            if (transitionSrc) {
                obs_source_release(currTransition);
            }
        }
        obs_source_release(targetSceneSrc);
    }

    obs_source_release(currSceneSrc);

    return true;
}

void PmCore::execIndependentActions(
    const PmReaction &reaction, bool switchedOn)
{
    const auto &actions
        = switchedOn ? reaction.matchActions : reaction.unmatchActions;
	for (const auto &action : actions) {
		if (action.actionType == PmActionType::SceneItem) {
			obs_sceneitem_t *sceneItem = nullptr;
			{
				QMutexLocker locker(&m_scenesMutex);
				auto find = m_sceneItems.find(
					action.targetElement);
				if (find != m_sceneItems.end()) {
					OBSSceneItem sceneItemSi = find->si;
					sceneItem = sceneItemSi;
				}
			}
			if (sceneItem) {
				if (switchedOn &&
				    action.actionCode ==
					    (size_t)PmToggleCode::Show) {
					obs_sceneitem_set_visible(sceneItem,
								  true);
				} else if (!switchedOn &&
					   action.actionCode ==
						   (size_t)PmToggleCode::Hide) {
					obs_sceneitem_set_visible(sceneItem,
								  false);
				}
				obs_sceneitem_release(sceneItem);
			}
		} else if (action.actionType == PmActionType::Filter) {
			obs_source_t *filterSrc = nullptr;
			{
				QMutexLocker locker(&m_scenesMutex);
				auto find =
					m_filters.find(action.targetElement);
				if (find != m_filters.end()) {
					OBSWeakSource filterWs = find->wsrc;
					filterSrc = obs_weak_source_get_source(
						filterWs);
				}
			}
			if (filterSrc) {
				if (switchedOn &&
				    action.actionCode ==
					    (size_t)PmToggleCode::Show) {
					obs_source_set_enabled(filterSrc, true);
				} else if (!switchedOn &&
					   action.actionCode ==
						   (size_t)PmToggleCode::Hide) {
					obs_source_set_enabled(filterSrc,
							       false);
				}
				obs_source_release(filterSrc);
			}
		} else if (action.actionType == PmActionType::Hotkey) {
			PmHotkeyActionCode hkeyCode = (PmHotkeyActionCode)action.actionCode;
			//obs_hotkey_inject_event(action.keyCombo, true);
			if (hkeyCode == PmHotkeyActionCode::Press
             || hkeyCode == PmHotkeyActionCode::Both) {
    			obs_hotkey_inject_event(action.keyCombo, true);
            }
			if (hkeyCode == PmHotkeyActionCode::Release
             || hkeyCode == PmHotkeyActionCode::Both) {
				obs_hotkey_inject_event(action.keyCombo, false);
			}
		}
	}
}

void PmCore::supplyImageToFilter(
    struct pm_filter_data* data, size_t matchIdx, const QImage &image)
{
    if (data) {
        pthread_mutex_lock(&data->mutex);
        auto entryData = data->match_entries + matchIdx;

        size_t sz = (size_t)(image.bytesPerLine()) * (size_t)(image.height());
        if (sz) {
            entryData->match_img_data = bmalloc(sz);
            memcpy(entryData->match_img_data, image.bits(), sz);
        } else {
            entryData->match_img_data = nullptr;
        }

        entryData->match_img_width = uint32_t(image.width());
        entryData->match_img_height = uint32_t(image.height());
        pthread_mutex_unlock(&data->mutex);
    }
}

void PmCore::pmSave(obs_data_t *saveData)
{
    obs_data_t *saveObj = obs_data_create();

    // master toggles
    {
        obs_data_set_bool(saveObj, "running_enabled", m_runningEnabled);
        obs_data_set_bool(saveObj, "switching_enabled", m_switchingEnabled);
    }

    // match configuration and presets
    {
        QMutexLocker locker(&m_matchConfigMutex);

        // match presets
        obs_data_array_t *matchPresetArray = obs_data_array_create();
        for (const auto &name: m_matchPresets.keys()) {
            PmMultiMatchConfig presetCfg = m_matchPresets[name];
            obs_data_t *matchPresetObj = presetCfg.save(name);
            obs_data_array_push_back(matchPresetArray, matchPresetObj);
            obs_data_release(matchPresetObj);
        }
        obs_data_set_array(saveObj, "match_presets", matchPresetArray);
        obs_data_array_release(matchPresetArray);

        // active match config/preset
        obs_data_t *activeCfgObj;
        if (m_activeMatchPreset.empty()) {
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

    obs_data_release(saveObj);
}

void PmCore::pmLoad(obs_data_t *data)
{
    obs_data_t *loadObj = obs_data_get_obj(data, "pixel-match-switcher");

    m_selectedMatchIndex = 0;

    // master toggles
    {
        obs_data_set_default_bool(loadObj, "running_enabled", false);
        m_runningEnabled = obs_data_get_bool(loadObj, "running_enabled");

        obs_data_set_default_bool(loadObj, "switching_enabled", true);
        m_switchingEnabled = obs_data_get_bool(loadObj, "switching_enabled");
    }

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
            onMatchPresetSelect(activePresetName);
        } else {
            PmMultiMatchConfig activeCfg(activeCfgObj);
            activateMultiMatchConfig(activeCfg);
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
