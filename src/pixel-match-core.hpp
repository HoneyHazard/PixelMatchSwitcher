#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <obs.hpp>

struct pixel_match_filter_data;
class PixelMatchDialog;

class PixelMatcher : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

public:
    PixelMatcher();

    obs_source_t* filter() const { return m_filter; }

public slots:
    void findFilter();

private:
    static PixelMatcher *m_instance;
    PixelMatchDialog *m_dialog;
    obs_source_t *m_filter;
};
