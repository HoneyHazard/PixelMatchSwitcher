#include "pm-linger-queue.hpp"

bool PmLingerCompare::operator()(const PmLingerInfo &left,
                   const PmLingerInfo &right) const
{
    return left.matchIndex > right.matchIndex;
}

void PmLingerQueue::removeExpired(const QTime &currTime)
{
    for (auto itr = c.begin(); itr != c.end();) {
        if (currTime > itr->endTime) {
            itr = c.erase(itr);
        } else {
            itr++;
        }
    }
}

void PmLingerQueue::removeByMatchIndex(size_t matchIndex)
{
    for (auto itr = c.begin(); itr != c.end(); itr++) {
        if (itr->matchIndex == matchIndex) {
            c.erase(itr);
            return;
        }
    }
}

void PmLingerQueue::removeAll()
{
    c.clear();
}
