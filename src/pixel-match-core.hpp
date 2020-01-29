#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <obs.hpp>
#include <string>
#include <vector>

struct pixel_match_filter_data;
class PixelMatchDialog;

class PixelMatcher : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

public:
    PixelMatcher();

    std::vector<obs_source_t*> filters() const { return m_filters; }

    std::string enumSceneElements();

public slots:
    void findFilters();

private:
    static PixelMatcher *m_instance;
    PixelMatchDialog *m_dialog;
    std::vector<obs_source_t*> m_filters;
};
