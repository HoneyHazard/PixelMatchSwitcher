#pragma once

#include "pm-structs.hpp"

#include <QWidget>

class PmCore;
class QListWidget;

class PmMatchListWidget : public QWidget
{
    Q_OBJECT

public:
    PmMatchListWidget(PmCore* core, QWidget* parent);

signals:
    void sigChangedMatchConfig(size_t idx, PmMatchConfig cfg);
    
    void sigSelectMatchIndex(size_t matchIndex);
    void sigMoveMatchConfigUp(size_t matchIndex);
    void sigMoveMatchConfigDown(size_t matchIndex);
    void sigInsertMatchConfig(size_t matchIndex, PmMatchConfig cfg);
    void sigRemoveMatchConfig(size_t matchIndex);
    void sigResetMatchConfigs();

protected slots:
    void onScenesChanged(PmScenes);
    void onNewMatchResults(size_t idx, PmMatchResults results);
    void onNewMultiMatchConfigSize(size_t sz);
    void onChangedMatchConfig(size_t idx, PmMatchConfig cfg);
    void onSelectMatchIndex(size_t matchIndex, PmMatchConfig config);

protected:
    PmCore* m_core;
    QListWidget *m_listWidget;
};