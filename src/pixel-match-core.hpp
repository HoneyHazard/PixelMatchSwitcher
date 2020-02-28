#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QMutex>
#include <QByteArray>
#include <QImage>

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

    std::vector<PmFilterRef> filters() const;
    PmFilterRef activeFilterRef() const;
    std::string scenesInfo() const;

    const QImage& qImage() { return m_qImage; }
    PmResultsPacket results() const;
    PmConfigPacket config() const;

signals:
    void sigFrameProcessed();
    void sigImgSuccess(QString filename);
    void sigImgFailed(QString filename);
    void sigNewResults(PmResultsPacket);

private slots:
    void onPeriodicUpdate();
    void onFrameProcessed();
    void onOpenImage(QString filename);
    void onNewUiConfig(PmConfigPacket);

private:

    static PmCore *m_instance;
    PmDialog *m_dialog;

    void scanScenes();
    void updateActiveFilter();
    void supplyImageToFilter();
    void supplyConfigToFilter();

    mutable QMutex m_mutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;

    QImage m_qImage;

    PmConfigPacket m_config;
    PmResultsPacket m_results;
};
