#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QMutex>
#include <QByteArray>
#include <QImage>

#include <string>
#include <vector>

struct pixel_match_filter_data;
struct obs_scene_item;
struct obs_scene;
struct obs_source;
class PixelMatchDialog;

struct PmFilterInfo
{
    obs_scene *scene = nullptr;
    obs_source *sceneSrc = nullptr;

    obs_scene_item *sceneItem = nullptr;
    obs_source *itemSrc = nullptr;

    obs_source *filter = nullptr;
};

class PixelMatcher : public QObject
{
    Q_OBJECT
    friend void init_pixel_match_switcher();
    friend void free_pixel_match_switcher();

signals:
    void newFrameImage();

public:
    PixelMatcher();

    std::vector<PmFilterInfo> filters() const;
    PmFilterInfo activeFilterInfo() const;

    std::string scenesInfo() const;
    const QImage &frameImage() const { return m_frameImage; }

private slots:
    void periodicUpdate();
    void checkFrame();

private:
    static PixelMatcher *m_instance;
    PixelMatchDialog *m_dialog;

    void unsetActiveFilter();
    void setActiveFilter(const PmFilterInfo &fi);

    void scanScenes();
    void updateActiveFilter();

    mutable QMutex m_mutex;
    std::vector<PmFilterInfo> m_filters;
    PmFilterInfo m_activeFilter;
    pixel_match_filter_data *m_filterData = nullptr;
    QByteArray m_frameRgba;
    QImage m_frameImage;
};
