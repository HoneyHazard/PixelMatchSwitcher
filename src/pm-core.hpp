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
    friend void on_frame_processed(struct pm_filter_data* filter);
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
    std::string matchImgFilename(size_t matchIdx) const;
    size_t selectedConfigIndex() const { return m_selectedMatchIndex; }

    PmMultiMatchResults multiMatchResults() const;
    PmMatchResults matchResults(size_t matchIdx) const;
    PmScenes scenes() const;
    //PmSwitchConfig switchConfig() const;
    gs_effect_t *drawMatchImageEffect() const { return m_drawMatchImageEffect; }
    QImage matchImage(size_t matchIdx);

signals:
    void sigFrameProcessed();
    void sigNewMatchResults(size_t matchIndex, PmMatchResults results);
    void sigChangedMatchConfig(size_t matchIndex, PmMatchConfig config);
    void sigNewMultiMatchConfigSize(size_t sz);
    void sigSelectMatchIndex(size_t matchIndex, PmMatchConfig config);
    void sigNoMatchSceneChanged(std::string sceneName, std::string transition);

    // REDO!!!
    void sigImgSuccess(size_t matchIdx, std::string filename, QImage img);
    void sigImgFailed(size_t matchIdx, std::string filename);
    void sigScenesChanged(PmScenes);
    void sigMatchPresetsChanged();
    void sigMatchPresetStateChanged();

public slots:
    //void onNewSwitchConfig(PmSwitchConfig);
    void onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg);   
    void onInsertMatchConfig(size_t matchIndex, PmMatchConfig cfg);
    void onRemoveMatchConfig(size_t matchIndex);
    void onMoveMatchConfigUp(size_t matchIndex);
    void onMoveMatchConfigDown(size_t matchIndex);
    void onResetMatchConfigs();
    void onSelectMatchIndex(size_t matchIndex);

    void onSelectActiveMatchPreset(std::string name);
    void onSaveMatchPreset(std::string name);
    void onRemoveMatchPreset(std::string name);

protected slots:
    //void onNewMultiMatchConfig(PmMultiMatchConfig cfg);

    void onMenuAction();
    void onPeriodicUpdate();
    void onFrameProcessed();

protected:
    void switchScene(
        //OBSWeakSource& scene, OBSWeakSource& transition);
        const std::string& scene, const std::string &transition);
    void scanScenes();
    void updateActiveFilter();
    
    void supplyImageToFilter(size_t matchIdx);
    void supplyImagesToFilter();

    void supplyConfigToFilter(size_t matchIdx);
    void supplyConfigsToFilter();

    void pmSave(obs_data_t *data);
    void pmLoad(obs_data_t *data);

    static PmCore *m_instance;
    QPointer<PmDialog> m_dialog;

    mutable QMutex m_filtersMutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;

    mutable QMutex m_matchConfigMutex;
    PmMultiMatchConfig m_multiMatchConfig;
    size_t m_selectedMatchIndex;
    
    std::string m_activeMatchPreset;
    PmMatchPresets m_matchPresets;

    mutable QMutex m_resultsMutex;
    PmMultiMatchResults m_results;

    mutable QMutex m_scenesMutex;
    PmScenes m_scenes;

    //mutable QMutex m_switchConfigMutex;
    //PmSwitchConfig m_switchConfig;

    std::vector<QImage> m_matchImages;
    gs_effect *m_drawMatchImageEffect = nullptr;
};
