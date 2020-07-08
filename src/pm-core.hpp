#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QPointer>

#include <string>
#include <vector>

#include <obs.hpp>

#include "pm-filter-ref.hpp"
#include "pm-structs.hpp"

struct obs_scene_item;
struct obs_scene;
struct pm_filter_data;
class PmDialog;

class PmCore : public QObject
{
    Q_OBJECT

    // interactions with OBS C components
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();
    friend void on_frame_processed();
    friend void pm_save_load_callback(
        obs_data_t *save_data, bool saving, void *corePtr);

public:
    PmCore();
    ~PmCore();

    std::vector<PmFilterRef> filters() const;
    PmFilterRef activeFilterRef() const;
    std::string scenesInfo() const;

    std::string activeMatchPresetName() const;
    bool matchPresetExists(const std::string &name) const;
    PmMultiMatchConfig matchPresetByName(const std::string &name) const;
    PmMatchPresets matchPresets() const;
    bool matchConfigDirty() const;

    PmMultiMatchConfig multiMatchConfig() const;
    size_t multiMatchConfigSize() const;
    PmMatchConfig matchConfig(size_t matchIdx) const;
    std::string noMatchScene() const;
    std::string noMatchTransition() const;
    std::string matchImgFilename(size_t matchIdx) const;
    size_t selectedConfigIndex() const { return m_selectedMatchIndex; }

    PmPreviewConfig previewConfig() const;

    PmMultiMatchResults multiMatchResults() const;
    PmMatchResults matchResults(size_t matchIdx) const;
    PmScenes scenes() const;
    QList<std::string> availableTransitions() const 
        { return m_availableTransitions.keys(); }
    gs_effect_t *drawMatchImageEffect() const { return m_drawMatchImageEffect; }
    QImage matchImage(size_t matchIdx) const;

signals:
    void sigFrameProcessed();
    void sigNewMatchResults(size_t matchIndex, PmMatchResults results);
    void sigChangedMatchConfig(size_t matchIndex, PmMatchConfig config);
    void sigNewMultiMatchConfigSize(size_t sz);
    void sigSelectMatchIndex(size_t matchIndex, PmMatchConfig config);
    void sigNoMatchSceneChanged(std::string sceneName);
    void sigNoMatchTransitionChanged(std::string transName);
    void sigPreviewConfigChanged(PmPreviewConfig cfg);

    void sigImgSuccess(size_t matchIdx, std::string filename, QImage img);
    void sigImgFailed(size_t matchIdx, std::string filename);
    void sigScenesChanged(PmScenes);
    void sigAvailablePresetsChanged();
    void sigActivePresetChanged();
    void sigActivePresetDirtyChanged();

    void sigRunningEnabledChanged(bool enable);
    void sigSwitchingEnabledChanged(bool enable);
    void sigNewActiveFilter(PmFilterRef newAf);

public slots:
    void onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg);   
    void onInsertMatchConfig(size_t matchIndex, PmMatchConfig cfg);
    void onRemoveMatchConfig(size_t matchIndex);
    void onMoveMatchConfigUp(size_t matchIndex);
    void onMoveMatchConfigDown(size_t matchIndex);
    void onResetMatchConfigs();
    void onSelectMatchIndex(size_t matchIndex);
    void onNoMatchSceneChanged(std::string sceneName);
    void onNoMatchTransitionChanged(std::string transName);
    void onPreviewConfigChanged(PmPreviewConfig cfg);

    void onSelectActiveMatchPreset(std::string name);
    void onSaveMatchPreset(std::string name);
    void onRemoveMatchPreset(std::string name);

    void onRunningEnabledChanged(bool enable);
    void onSwitchingEnabledChanged(bool enable);

protected slots:
    void onMenuAction();
    void onPeriodicUpdate();
    void onFrameProcessed();

protected:
    static QHash<std::string, OBSWeakSource> getAvailableTransitions();

    void activate();
    void deactivate();

    void switchScene(
        const std::string& scene, const std::string &transition);
    void scanScenes();
    void updateActiveFilter();
    void activateMultiMatchConfig(const PmMultiMatchConfig& mCfg);
    
    void supplyImageToFilter(
        struct pm_filter_data *data, size_t matchIdx, const QImage &image);

    void pmSave(obs_data_t *data);
    void pmLoad(obs_data_t *data);

    static PmCore *m_instance;
    QPointer<QThread> m_thread;
    QPointer<PmDialog> m_dialog;
    QPointer<QTimer> m_periodicUpdateTimer;

    mutable QMutex m_filtersMutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;

    mutable QMutex m_scenesMutex;
    PmScenes m_scenes;

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

    mutable QMutex m_matchImagesMutex;
    std::vector<QImage> m_matchImages;
    gs_effect *m_drawMatchImageEffect = nullptr;
};
