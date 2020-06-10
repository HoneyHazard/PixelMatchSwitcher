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

    PmMultiMatchConfig matchConfig() const;
    std::string matchImgFilename(size_t matchIdx) const;

    PmMultiMatchResults results() const;
    PmScenes scenes() const;
    //PmSwitchConfig switchConfig() const;
    gs_effect_t *drawMatchImageEffect() const { return m_drawMatchImageEffect; }
    const QImage& matchImage() { return m_matchImg; }

signals:
    void sigFrameProcessed();
    void sigImgSuccess(std::string filename, QImage img);
    void sigImgFailed(std::string filename);
    void sigNewMatchResults(PmMatchResults);
    void sigScenesChanged(PmScenes);
    void sigMatchPresetsChanged();
    void sigMatchPresetStateChanged();

public slots:
    //void onNewSwitchConfig(PmSwitchConfig);
    void onNewMatchConfig(PmMatchConfig);

    void onSelectActiveMatchPreset(std::string name);
    void onSaveMatchPreset(std::string name);
    void onRemoveMatchPreset(std::string name);

private slots:
    void onMenuAction();
    void onPeriodicUpdate();
    void onFrameProcessed();

protected:
    static void switchScene(OBSWeakSource& scene, OBSWeakSource& transition);
    void scanScenes();
    void updateActiveFilter();
    void supplyImageToFilter();
    void supplyConfigToFilter();

    void pmSave(obs_data_t *data);
    void pmLoad(obs_data_t *data);

    static PmCore *m_instance;
    QPointer<PmDialog> m_dialog;

    mutable QMutex m_filtersMutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;

    mutable QMutex m_matchConfigMutex;
    PmMultiMatchConfig m_matchConfig;
    
    std::string m_activeMatchPreset;
    PmMatchPresets m_matchPresets;

    mutable QMutex m_resultsMutex;
    PmMultiMatchResults m_results;

    mutable QMutex m_scenesMutex;
    PmScenes m_scenes;

    //mutable QMutex m_switchConfigMutex;
    //PmSwitchConfig m_switchConfig;

    QImage m_matchImg;
    gs_effect *m_drawMatchImageEffect = nullptr;
};
