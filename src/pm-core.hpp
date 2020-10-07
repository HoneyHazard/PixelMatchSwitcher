#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

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
#include "pm-dialog.hpp"

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
    friend void on_snapshot_available();
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
    std::string noMatchScene() const;
    std::string noMatchTransition() const;
    std::string matchImgFilename(size_t matchIdx) const;
    bool hasFilename(size_t matchIdx) const;
    size_t selectedConfigIndex() const { return m_selectedMatchIndex; }

    PmPreviewConfig previewConfig() const;

    PmMultiMatchResults multiMatchResults() const;
    PmMatchResults matchResults(size_t matchIdx) const;
    PmScenes scenes() const;
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
    void sigScenesChanged(PmScenes);

    void sigFrameProcessed();
    void sigSnapshotAvailable();
    void sigNewMatchResults(size_t matchIndex, PmMatchResults results);

    void sigAvailablePresetsChanged();
    void sigActivePresetChanged();
    void sigActivePresetDirtyChanged();

    void sigMultiMatchConfigSizeChanged(size_t sz);
    void sigMatchConfigChanged(size_t matchIndex, PmMatchConfig config);
    void sigMatchConfigSelect(size_t matchIndex, PmMatchConfig config);
    void sigNoMatchSceneChanged(std::string sceneName);
    void sigNoMatchTransitionChanged(std::string transName);
    
    void sigPreviewConfigChanged(PmPreviewConfig cfg);
    void sigRunningEnabledChanged(bool enable);
    void sigSwitchingEnabledChanged(bool enable);

    void sigMatchImageLoadSuccess(
        size_t matchIdx, std::string filename, QImage img);
    void sigMatchImageLoadFailed(size_t matchIdx, std::string filename);
    void sigMatchImageCaptured(QImage img, int roiLeft, int roiBottom);
    void sigCaptureStateChanged(PmCaptureState capMode, int x=-1, int y=-1);

public slots:
    void onMatchPresetSelect(std::string name);
    void onMatchPresetSave(std::string name);
    void onMatchPresetRemove(std::string name);

    void onMultiMatchConfigReset();
    void onNoMatchSceneChanged(std::string sceneName);
    void onNoMatchTransitionChanged(std::string transName);

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

protected slots:
    void onMenuAction();
    void onPeriodicUpdate();
    void onFrameProcessed();
    void onSnapshotAvailable();

protected:
    static QHash<std::string, OBSWeakSource> getAvailableTransitions();

    void activate();
    void deactivate();

    void switchScene(
        const std::string& scene, const std::string &transition);
    void scanScenes();
    void updateActiveFilter(const QSet<OBSWeakSource> &filters);
    void activateMatchConfig(size_t matchIndex, const PmMatchConfig& cfg);
    void loadImage(size_t matchIndex);
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

    mutable QMutex m_filtersMutex;
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
    
    PmCaptureState m_captureState = PmCaptureState::Inactive;
    int m_captureStartX = 0, m_captureStartY = 0;
    int m_captureEndX = 0, m_captureEndY = 0;

    mutable QMutex m_matchImagesMutex;
    std::vector<QImage> m_matchImages;
};
