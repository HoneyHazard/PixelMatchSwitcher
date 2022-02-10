# pragma once

#include <queue>
#include <deque>
#include <QTime>
#include <QSet>

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
 * @brief Manages linger information for multiple scenes; 
 *        prioritized by match index
 */
class PmLingerQueue final
    : public std::priority_queue<PmLingerInfo, std::deque<PmLingerInfo>,
                     PmLingerCompare> {
public:
    PmLingerQueue() {}
    QSet<size_t> removeExpired(const QTime &currTime);
    bool removeByMatchIndex(size_t matchIndex);
    void removeAll() { c.clear(); }
};

/**
 * @brief Manages linger information for multiple sceneItems; no priority
 */
class PmLingerList final : public std::vector<PmLingerInfo>
{
public:
    PmLingerList() {}
    QSet<size_t> removeExpired(const QTime &currTime);
    bool removeByMatchIndex(size_t matchIndex);
    void removeAll() { clear(); }
    bool contains(const size_t matchIdx) const;
};
