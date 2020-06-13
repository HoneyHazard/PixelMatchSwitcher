#include <obs.hpp>

struct pm_filter_data;

/*!
 * \brief Interface for interacting with the filter that holds references
 */
class PmFilterRef
{
public:
    PmFilterRef() {}

    obs_scene* scene() const;
    obs_source_t* sceneSrc() const;
    obs_source_t* itemSrc() const;
    obs_source_t* filter() const;
    pm_filter_data *filterData() const;

    bool isValid() const { return m_filter ? true : false; }
    bool isActive() const;
    uint32_t filterSrcWidth() const;
    uint32_t filterSrcHeight() const;
    uint32_t filterDataWidth() const;
    uint32_t filterDataHeight() const;
    uint32_t numMatched(size_t matchIndex) const;

    void reset();
    void setScene(obs_source_t* scene);
    void setItem(obs_scene_item *item);
    void setFilter(obs_source_t *filter);
    void lockData() const;
    void unlockData() const;

protected:
    OBSWeakSource m_sceneSrc;
    OBSWeakSource m_itemSrc;
    OBSWeakSource m_filter;
};
