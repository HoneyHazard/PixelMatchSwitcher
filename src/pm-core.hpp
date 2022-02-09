#pragma once

#include <QObject>
#include <QMutex>
#include <QPointer>
#include <QImage>

#include <string>
#include <vector>
#include <QSet>

#include <obs.hpp>

#include "pm-filter-ref.hpp"
#include "pm-structs.hpp"
#include "pm-linger-queue.hpp"
#include "pm-dialog.hpp"
#include "pm-filter.h"

struct obs_scene_item;
struct obs_scene;
struct pm_filter_data;
class PmDialog;

// plugin's C functions
extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

// event handlers for OBS and filter events
void pm_save_load_callback(obs_data_t *save_data, bool saving, void *corePtr);
void on_frame_processed(struct pm_filter_data *filterData);
void on_match_image_captured(struct pm_filter_data *filterData);

/**
 *        and more
 */
class PmCore : public QObject   
{
    Q_OBJECT

    // interactions with OBS C components
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();
    friend void on_frame_processed(pm_filter_data *filterData);
    friend void on_settings_button_released();
    friend void on_match_image_captured(pm_filter_data *filterData);
    friend void pm_save_load_callback(
        obs_data_t *save_data, bool saving, void *corePtr);

public:
    PmCore();
    ~PmCore();

    PmFilterRef activeFilterRef() const;
    std::string scenesInfo() const;

    std::string activeMatchPresetName() const;
    bool matchPresetExists(const std::string &name) const;
    PmMultiMatchConfig matchPresetByName(const std::string &name) const;
    size_t matchPresetSize(const std::string& name) const;
    QList<std::string> matchPresetNames() const;
    bool matchConfigDirty() const;

    PmMultiMatchConfig multiMatchConfig() const;
    size_t multiMatchConfigSize() const;
    PmMatchConfig matchConfig(size_t matchIdx) const;
    std::string matchConfigLabel(size_t matchIdx) const;
    bool hasSceneAction(size_t matchIdx) const
        { return hasAction(matchIdx, PmActionType::Scene); }
    bool hasAction(size_t matchIdx, PmActionType actionType) const;
    bool hasMatchAction(size_t matchIdx, PmActionType actionType) const;
    bool hasUnmatchAction(size_t matchIdx, PmActionType actionType) const;
    bool hasGlobalMatchAction(PmActionType actionType) const;
    bool hasGlobalUnmatchAction(PmActionType actionType) const;
    PmReaction reaction(size_t matchIdx) const;
    PmReaction noMatchReaction() const;
    std::string matchImgFilename(size_t matchIdx) const;
    bool hasFilename(size_t matchIdx) const;
    size_t selectedConfigIndex() const { return m_selectedMatchIndex; }
    bool enforceTargetOrder(size_t matchIdx, const PmMatchConfig &newCfg);
    bool matchConfigCanMoveUp(size_t idx) const;
    bool matchConfigCanMoveDown(size_t idx) const;

    PmPreviewConfig previewConfig() const;

    PmMultiMatchResults multiMatchResults() const;
    PmMatchResults matchResults(size_t matchIdx) const;
    PmSourceHash scenes() const;
    QList<std::string> sceneNames() const;
    QList<std::string> sceneItemNames(const std::string &sceneName) const;
    PmSceneItemsHash sceneItems() const;
    QList<std::string> allFilterNames() const;
    QList<std::string> filterNames(const std::string &sceneItemName) const;
    QList<std::string> sceneItemNames() const;
    QList<std::string> audioSourcesNames() const;
    QList<std::string> availableTransitions() const 
        { return m_availableTransitions.keys(); }
    QImage matchImage(size_t matchIdx) const;
    bool matchImageLoaded(size_t matchIdx) const;

    inline bool runningEnabled() const { return m_runningEnabled; }
    inline bool switchingEnabled() const { return m_switchingEnabled; }
    inline PmCaptureState captureState() const { return m_captureState; }
    void getCaptureEndXY(int& x, int& y) const;

signals:
    void sigActiveFilterChanged(PmFilterRef newAf);
    void sigScenesChanged(
        QList<std::string> scenes, QList<std::string> sceneItems);
    void sigAudioSourcesChanged(QList<std::string> audioSources);

    void sigFrameProcessed(PmMultiMatchResults);
    void sigNewMatchResults(size_t matchIndex, PmMatchResults results);
    void sigShowException(std::string caption, std::string descr);

    void sigAvailablePresetsChanged();
    void sigActivePresetChanged();
    void sigActivePresetDirtyChanged();
    void sigPresetsImportAvailable(PmMatchPresets presets);

    void sigMultiMatchConfigSizeChanged(size_t sz);
    void sigMatchConfigChanged(size_t matchIndex, PmMatchConfig config);
    void sigMatchConfigSelect(size_t matchIndex, PmMatchConfig config);
    void sigNoMatchReactionChanged(PmReaction noMatchReaction);
    
    void sigPreviewConfigChanged(PmPreviewConfig cfg);
    void sigRunningEnabledChanged(bool enable);
    void sigSwitchingEnabledChanged(bool enable);

    void sigMatchImageLoadSuccess(
        size_t matchIdx, std::string filename, QImage img);
    void sigMatchImageLoadFailed(size_t matchIdx, std::string filename);
    void sigMatchImageCaptured(QImage img, int roiLeft, int roiBottom);
    void sigCaptureStateChanged(PmCaptureState capMode, int x=-1, int y=-1);

    void sigMatchImagesOrphaned(QList<std::string> filenames);

    void sigCooldownActive(size_t matchIdx, bool active);
    void sigLingerActive(size_t matchIdx, bool active);

public slots:
    void onMenuAction();

    void onMatchPresetSelect(std::string name);
    void onMatchPresetSave(std::string name);
    void onMatchPresetRemove(std::string name);
    void onMatchPresetActiveRevert();
    void onMatchPresetExport(
        std::string filename, QList<std::string> selectedPresets);
    void onMatchPresetsImport(std::string filename);
    void onMatchPresetsAppend(PmMatchPresets presets);

    void onMultiMatchConfigReset();
    void onNoMatchReactionChanged(PmReaction noMatchReaction);

    void onMatchConfigChanged(size_t matchIndex, PmMatchConfig cfg);
    void onMatchConfigInsert(size_t matchIndex, PmMatchConfig cfg);
    void onMatchConfigRemove(size_t matchIndex);
    void onMatchConfigMoveUp(size_t matchIndex);
    void onMatchConfigMoveDown(size_t matchIndex);
    void onMatchConfigSelect(size_t matchIndex);

    void onPreviewConfigChanged(PmPreviewConfig cfg);
    void onRunningEnabledChanged(bool enable);
    void onSwitchingEnabledChanged(bool enable);
    void onCaptureStateChanged(PmCaptureState capMode, int x=-1, int y=-1);
    void onMatchImageRefresh(size_t matchIndex);

    void onMatchImagesRemove(QList<std::string> orphanedImages);

protected slots:
    void onPeriodicUpdate();
    void onFrameProcessed(PmMultiMatchResults);

protected:
    static QHash<std::string, OBSWeakSource> getAvailableTransitions();

    void activate();
    void deactivate();

    void execReaction(bool &sceneSelected, size_t matchIdx, const QTime &time,
        const PmReaction &reaction, bool switchedOn);
    bool execSceneAction(
        size_t matchIdx, const PmReaction &reaction, bool switchedOn);
    bool execIndependentActions(const std::string &cfgName,
        const PmReaction &reaction, bool switchedOn);

    void scanScenes();
    void updateActiveFilter(const QSet<OBSWeakSource> &filters);
    void activateMatchConfig(size_t matchIndex, const PmMatchConfig& cfg,
        QSet<std::string>* orphanedImages = nullptr);
    void loadImage(size_t matchIndex);
    void resetMultiMatchConfig(const PmMultiMatchConfig *newCfg = nullptr);
    void activateMultiMatchConfig(const PmMultiMatchConfig& mCfg);
    void activeFilterChanged();

    void supplyImageToFilter(
        struct pm_filter_data *data, size_t matchIdx, const QImage &image);

    void pmSave(obs_data_t *data);
    void pmLoad(obs_data_t *data);

    static PmCore *m_instance;
    QThread* m_thread = nullptr;
    bool m_periodicUpdateActive = false;
    QPointer<PmDialog> m_dialog = nullptr;
    QTimer* m_periodicUpdateTimer = nullptr;

    mutable QMutex m_pmFilterMutex;
    PmFilterRef m_activeFilter;

    mutable QMutex m_scenesMutex;
    PmSourceHash m_scenes;
    PmSceneItemsHash m_sceneItems;
    PmSourceHash m_filters;
    PmSourceHash m_audioSources;

    PmLingerQueue m_sceneLingerQueue;
    PmLingerList m_lingerList;
    PmLingerList m_cooldownList;
    QSet<size_t> m_expiredLingers;
    bool m_forceSceneItemRefresh = true;

    QHash<std::string, OBSWeakSource> m_availableTransitions;

    mutable QMutex m_matchConfigMutex;
    PmMultiMatchConfig m_multiMatchConfig;
    size_t m_selectedMatchIndex = 0;
    
    std::string m_activeMatchPreset;
    PmMatchPresets m_matchPresets;

    mutable QMutex m_resultsMutex;
    PmMultiMatchResults m_results;

    mutable QMutex m_previewConfigMutex;
    PmPreviewConfig m_previewConfig;

    bool m_runningEnabled = false;
    bool m_switchingEnabled = true;
    bool m_enforceReactionTypeOrder = true;
    
    PmCaptureState m_captureState = PmCaptureState::Inactive;
    int m_captureStartX = 0, m_captureStartY = 0;
    int m_captureEndX = 0, m_captureEndY = 0;

    mutable QMutex m_matchImagesMutex;
    std::vector<QImage> m_matchImages;
};
