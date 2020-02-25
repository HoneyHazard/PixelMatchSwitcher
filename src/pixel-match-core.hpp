#pragma once

extern "C" void init_pixel_match_switcher();
extern "C" void free_pixel_match_switcher();

#include <QObject>
#include <QMutex>
#include <QByteArray>
#include <QImage>

#include <string>
#include <vector>

#include <obs.h>

struct obs_scene_item;
struct obs_scene;
struct pixel_match_filter_data;
class PixelMatchDialog;

class PmFilterRef
{
public:
    PmFilterRef() {}
    PmFilterRef(const PmFilterRef &other);
    ~PmFilterRef() { reset(); }

    obs_scene *scene() const { return m_scene; }
    obs_source_t *sceneSrc() const { return m_sceneSrc; }
    obs_scene_item *sceneItem() const { return m_sceneItem; }
    obs_source_t *itemSrc() const { return m_itemSrc; }
    obs_source_t *filter() const { return m_filter; }

    bool isValid() const { return m_filter ? true : false; }
    bool isActive() const;
    uint32_t filterSrcWidth() const;
    uint32_t filterSrcHeight() const;
    uint32_t filterDataWidth() const;
    uint32_t filterDataHeight() const;
    uint32_t numMatched() const;

    void reset();
    void setScene(obs_source_t *sceneSrc);
    void setItem(obs_scene_item *item);
    void setFilter(obs_source_t *filter);
    void lockData() const;
    void unlockData() const;

protected:
    pixel_match_filter_data *filterData() const;

    obs_scene *m_scene = nullptr;
    obs_source_t *m_sceneSrc = nullptr;

    obs_scene_item *m_sceneItem = nullptr;
    obs_source_t *m_itemSrc = nullptr;

    obs_source_t *m_filter = nullptr;
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

    std::vector<PmFilterRef> filters() const;
    PmFilterRef activeFilterRef() const;

    std::string scenesInfo() const;

private slots:
    void periodicUpdate();

private:
    static PixelMatcher *m_instance;
    PixelMatchDialog *m_dialog;

    void scanScenes();
    void updateActiveFilter();

    mutable QMutex m_mutex;
    std::vector<PmFilterRef> m_filters;
    PmFilterRef m_activeFilter;
};
