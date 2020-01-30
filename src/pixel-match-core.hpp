#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QMutex>

#include <string>
#include <vector>
#include <obs.hpp>

struct pixel_match_filter_data;
class PixelMatchDialog;

struct PixelMatchFilterInfo
{
    std::string scene;
    std::string sceneItem;
    OBSSource filter = nullptr;
};

class PixelMatcher : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

public:
    PixelMatcher();

    std::vector<PixelMatchFilterInfo> filters() const;
    PixelMatchFilterInfo activeFilterInfo() const;

    std::string scenesInfo();

private slots:
    void periodicUpdate();

private:
    static PixelMatcher *m_instance;
    PixelMatchDialog *m_dialog;

    void unsetActiveFilter();
    void setActiveFilter(const PixelMatchFilterInfo &fi);
    void findFilters();
    void updateActiveFilter();

    mutable QMutex m_mutex;
    std::vector<PixelMatchFilterInfo> m_filters;
    PixelMatchFilterInfo m_activeFilter;
    pixel_match_filter_data *m_filterData = nullptr;
};
