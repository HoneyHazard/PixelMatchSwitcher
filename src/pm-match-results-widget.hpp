#pragma once

#include <QGroupBox>

#include "pm-structs.hpp"

class PmCore;
class QLabel;

class PmMatchResultsWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmMatchResultsWidget(PmCore* core, QWidget* parent);

protected slots:
    // core interaction
    void onNewMatchResults(size_t matchIdx, PmMatchResults results);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig cfg);

protected:
    PmCore* m_core;
    size_t m_matchIndex = 0;

    QLabel* m_matchResultDisplay;
};
