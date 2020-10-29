# pragma once

#include <queue>
#include <deque>
#include <QTime>

/**
 * @brief Decribes an active linger information for a match entry
 */
struct PmLingerInfo {
    size_t matchIndex = 0;
    QTime endTime;
};

/**
 * @brief So we can order PmLingerInfo in the order of matchIndex
 */
struct PmLingerCompare {
    bool operator()(const PmLingerInfo &left, const PmLingerInfo &right) const;
};

/**
 * @brief Manages linger information for multiple entries; sorted by match index
 */
class PmLingerQueue final
    : public std::priority_queue<PmLingerInfo, std::deque<PmLingerInfo>,
                     PmLingerCompare> {
public:
    PmLingerQueue() {}
    void removeExpired(const QTime &currTime);
    void removeByMatchIndex(size_t matchIndex);
    void removeAll();
};
