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

class PixelMatcher : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

public:
    PixelMatcher();

    std::vector<OBSWeakSource> filters() const;
    OBSWeakSource activeFilter() const;

    std::string enumSceneElements();

public slots:
    void findFilters();

private slots:
    void periodicUpdate();

private:
    static PixelMatcher *m_instance;
    PixelMatchDialog *m_dialog;

    mutable QMutex m_mutex;
    std::vector<OBSWeakSource> m_filters;
    OBSWeakSource m_activeFilter;
};
