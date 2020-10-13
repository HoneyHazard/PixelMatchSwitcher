#include "pm-filter-ref.hpp"
#include "pm-filter.h"

void PmFilterRef::setFilter(OBSWeakSource filter)
{
    m_filter = filter;
}

void PmFilterRef::lockData() const
{
    auto data = filterData();
    if (data)
        pthread_mutex_lock(&data->mutex);
}

void PmFilterRef::unlockData() const
{
    auto data = filterData();
    if (data)
        pthread_mutex_unlock(&data->mutex);
}

obs_source_t* PmFilterRef::filter() const
{
    return obs_weak_source_get_source(m_filter); 
}

void PmFilterRef::reset()
{
    m_filter = nullptr;
}

pm_filter_data *PmFilterRef::filterData() const
{
    if (!m_filter) {
        return nullptr;
    } else {
        auto filter = obs_weak_source_get_source(m_filter);
        auto ret = static_cast<pm_filter_data*>(
            obs_obj_get_data(filter));
        obs_source_release(filter);
        return ret;
    }
}

uint32_t PmFilterRef::filterSrcWidth() const
{
    auto filter = obs_weak_source_get_source(m_filter);
    uint32_t ret = filter ? obs_source_get_width(filter) : 0;
    obs_source_release(filter);
    return ret;
}

uint32_t PmFilterRef::filterSrcHeight() const
{
    auto filter = obs_weak_source_get_source(m_filter);
    uint32_t ret = filter ? obs_source_get_height(filter) : 0;
    obs_source_release(filter);
    return ret;
}

uint32_t PmFilterRef::filterDataWidth() const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        ret = data->base_width;
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
        ret = data->base_height;
        unlockData();
    }
    return ret;
}

uint32_t PmFilterRef::numMatched(size_t matchIndex) const
{
    auto data = filterData();
    uint32_t ret = 0;
    if (data) {
        lockData();
        if (matchIndex < data->num_match_entries) {
            ret = data->match_entries[matchIndex].num_matched;
        }
        unlockData();
    }
    return ret;
}
