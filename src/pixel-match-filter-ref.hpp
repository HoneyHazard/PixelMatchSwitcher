#include <obs.h>

struct pm_filter_data;

/*!
 * \brief Interface for interacting with the filter that holds references
 */
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
    pm_filter_data *filterData() const;

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
    obs_scene *m_scene = nullptr;
    obs_source_t *m_sceneSrc = nullptr;

    obs_scene_item *m_sceneItem = nullptr;
    obs_source_t *m_itemSrc = nullptr;

    obs_source_t *m_filter = nullptr;
};
