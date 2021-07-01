#include "pm-linger-queue.hpp"

bool PmLingerCompare::operator()(const PmLingerInfo &left,
                   const PmLingerInfo &right) const
{
    return left.matchIndex > right.matchIndex;
}

std::vector<size_t> PmLingerQueue::removeExpired(const QTime &currTime)
{
    std::vector<size_t> ret;
    for (auto itr = c.begin(); itr != c.end();) {
        if (currTime > itr->endTime) {
            ret.push_back(itr->matchIndex);
            itr = c.erase(itr);
        } else {
            itr++;
        }
    }
    return ret;
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

//--------------------------------------------------------

std::vector<size_t> PmLingerList::removeExpired(const QTime &currTime)
{
    std::vector<size_t> ret;
    for (auto itr = begin(); itr != end();) {
        if (currTime > itr->endTime) {
            ret.push_back(itr->matchIndex);
            itr = erase(itr);
        } else {
            itr++;
        }
    }
    return ret;
}

void PmLingerList::removeByMatchIndex(size_t matchIndex)
{
    for (auto itr = begin(); itr != end(); itr++) {
        if (itr->matchIndex == matchIndex) {
            erase(itr);
            return;
        }
    }
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
