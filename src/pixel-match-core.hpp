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

struct obs_scene_item;
struct obs_scene;
struct pixel_match_filter_data;
class PmDialog;

class PmCore : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

signals:
    void newFrameImage();

public:
    PmCore();

    std::vector<PmFilterRef> filters() const;
    PmFilterRef activeFilterRef() const;
    std::string scenesInfo() const;

private slots:
    void periodicUpdate();

private:
    static PmCore *m_instance;
    PmDialog *m_dialog;

    void scanScenes();
    void updateActiveFilter();

    mutable QMutex m_mutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;
};
