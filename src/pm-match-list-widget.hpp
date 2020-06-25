#pragma once

#include "pm-structs.hpp"

#include <QWidget>

class PmCore;
class QTableWidget;

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

    void onRowSelected();

protected:
    enum class RowOrder : int { 
        EnableBox = 0, 
        ConfigName = 1, 
        SceneCombo = 2, 
        TransitionCombo = 3,
        Result = 4
    };

    void constructRow(int idx);

    void enableConfigToggled(size_t idx, bool enable);
    void configRenamed(size_t idx, const QString& name);
    void matchSceneSelected(size_t idx, const QString &scene);
    void transitionSelected(size_t idx, const QString& transition);

    PmCore* m_core;
    QTableWidget *m_tableWidget;
};