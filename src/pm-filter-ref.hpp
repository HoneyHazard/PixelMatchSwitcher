#pragma once

#include <obs.hpp>

struct pm_filter_data;

/*!
 * \brief Interface for interacting with the filter that holds references
 */
class PmFilterRef
{
public:
    PmFilterRef() {}

    OBSWeakSource filterWeakSource() const { return m_filter; }
    obs_source_t* filter() const;
    pm_filter_data *filterData() const;

    bool isValid() const { return m_filter ? true : false; }
    uint32_t filterSrcWidth() const;
    uint32_t filterSrcHeight() const;
    uint32_t filterDataWidth() const;
    uint32_t filterDataHeight() const;
    uint32_t numMatched(size_t matchIndex) const;

    void reset();
    void setFilter(OBSWeakSource filter);
    void lockData() const;
    void unlockData() const;

protected:
    OBSWeakSource m_filter;
};
