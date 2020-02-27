#include "pixel-match-filter-ref.hpp"
#include "pixel-match-filter.h"

void PmFilterRef::setScene(obs_source_t *sceneSrc)
{
    if (m_sceneSrc) {
        obs_source_release(m_sceneSrc);
    }
    m_sceneSrc = sceneSrc;
    if (m_sceneSrc) {
        obs_source_addref(m_sceneSrc);
    }

    if (m_scene) {
        obs_scene_release(m_scene);
    }
    if (m_sceneSrc) {
        m_scene = obs_scene_from_source(m_sceneSrc);
    }
    if (m_scene) {
        obs_scene_addref(m_scene);
    }
}

void PmFilterRef::setItem(obs_scene_item *item)
{
    m_sceneItem = item;

    if (m_itemSrc) {
        obs_source_release(m_itemSrc);
    }
    if (m_sceneItem) {
        m_itemSrc = obs_sceneitem_get_source(item);
    }
    if (m_itemSrc) {
        obs_source_addref(m_itemSrc);
    }
}

void PmFilterRef::setFilter(obs_source_t *filter)
{
    if (m_filter) {
        obs_source_release(m_filter);
    }
    m_filter = filter;
    if (m_filter) {
        obs_source_addref(m_filter);
    }
}

void PmFilterRef::lockData() const
{
    auto data = filterData();
    pthread_mutex_lock(&data->mutex);
}

void PmFilterRef::unlockData() const
{
    auto data = filterData();
    pthread_mutex_unlock(&data->mutex);
}

PmFilterRef::PmFilterRef(const PmFilterRef &other)
{
    setScene(other.sceneSrc());
    setItem(other.sceneItem());
    setFilter(other.filter());
}

void PmFilterRef::reset()
{
    if (m_filter) {
        obs_source_release(m_filter);
        m_filter = nullptr;
    }
    if (m_itemSrc) {
        obs_source_release(m_itemSrc);
        m_itemSrc = nullptr;
    }
    m_sceneItem = nullptr;
    if (m_scene) {
        obs_scene_release(m_scene);
        m_scene = nullptr;
    }
    if (m_sceneSrc) {
        obs_source_release(m_sceneSrc);
        m_sceneSrc = nullptr;
    }
}

pm_filter_data *PmFilterRef::filterData() const
{
    if (!m_filter) {
        return nullptr;
    } else {
        return static_cast<pm_filter_data*>(
            obs_obj_get_data(m_filter));
    }
}

bool PmFilterRef::isActive() const
{
    return m_itemSrc != nullptr && obs_source_active(m_itemSrc);
}

uint32_t PmFilterRef::filterSrcWidth() const
{
    return m_filter ? obs_source_get_width(m_filter) : 0;
}

uint32_t PmFilterRef::filterSrcHeight() const
{
    return m_filter ? obs_source_get_height(m_filter) : 0;
}

uint32_t PmFilterRef::filterDataWidth() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->cx;
        unlockData();
    }
    return ret;
}

uint32_t PmFilterRef::filterDataHeight() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->cy;
        unlockData();
    }
    return ret;
}

uint32_t PmFilterRef::numMatched() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->num_matched;
        unlockData();
    }
    return ret;
}
