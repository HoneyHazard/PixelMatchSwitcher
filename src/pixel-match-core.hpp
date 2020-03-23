#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QMutex>
#include <QByteArray>
#include <QImage>
#include <QPointer>

#include <string>
#include <vector>

#include "pixel-match-filter-ref.hpp"
#include "pixel-match-structs.hpp"

struct obs_scene_item;
struct obs_scene;
struct pm_filter_data;
class PmDialog;

class PmCore : public QObject
{
    Q_OBJECT

    // interactions with the C components
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();
    friend void on_frame_processed(struct pm_filter_data* filter);

public:
    PmCore();
    ~PmCore();

    std::vector<PmFilterRef> filters() const;
    PmFilterRef activeFilterRef() const;
    std::string scenesInfo() const;

    const QString &matchImgFilename() { return m_matchImgFilename; }
    const QImage& matchImage() { return m_matchImg; }
    PmResultsPacket results() const;
    PmConfigPacket config() const;
    QSize videoBaseSize() const;
    gs_effect_t *drawMatchImageEffect() const { return m_drawMatchImageEffect; }

signals:
    void sigFrameProcessed();
    void sigImgSuccess(QString filename);
    void sigImgFailed(QString filename);
    void sigNewResults(PmResultsPacket);

private slots:
    void onMenuAction();
    void onPeriodicUpdate();
    void onFrameProcessed();
    void onOpenImage(QString filename);
    void onNewUiConfig(PmConfigPacket);

private:

    static PmCore *m_instance;
    QPointer<PmDialog> m_dialog;

    void scanScenes();
    void updateActiveFilter();
    void supplyImageToFilter();
    void supplyConfigToFilter();

    mutable QMutex m_mutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;

    QString m_matchImgFilename;
    QImage m_matchImg;

    PmConfigPacket m_config;
    PmResultsPacket m_results;

    gs_effect *m_drawMatchImageEffect = nullptr;

};
