#include "pm-linger-queue.hpp"

bool PmLingerCompare::operator()(const PmLingerInfo &left,
                   const PmLingerInfo &right) const
{
    return left.matchIndex > right.matchIndex;
}

QSet<size_t> PmLingerQueue::removeExpired(const QTime &currTime)
{
    QSet<size_t> ret;
    for (auto itr = c.begin(); itr != c.end();) {
        if (currTime > itr->endTime) {
            ret.insert(itr->matchIndex);
            itr = c.erase(itr);
        } else {
            itr++;
        }
    }
    return ret;
}

bool PmLingerQueue::removeByMatchIndex(size_t matchIndex)
{
    for (auto itr = c.begin(); itr != c.end(); itr++) {
        if (itr->matchIndex == matchIndex) {
            c.erase(itr);
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------

QSet<size_t> PmLingerList::removeExpired(const QTime &currTime)
{
    QSet<size_t> ret;
    for (auto itr = begin(); itr != end();) {
        if (currTime > itr->endTime) {
            ret.insert(itr->matchIndex);
            itr = erase(itr);
        } else {
            itr++;
        }
    }
    return ret;
}

bool PmLingerList::removeByMatchIndex(size_t matchIndex)
{
    for (auto itr = begin(); itr != end(); itr++) {
        if (itr->matchIndex == matchIndex) {
            erase(itr);
            return true;
        }
    }
    return false;
}

bool PmLingerList::contains(const size_t matchIdx) const
{
    for (auto itr = begin(); itr != end(); itr++) {
        if (itr->matchIndex == matchIdx) {
            return true;
        }
    }
    return false;
}
