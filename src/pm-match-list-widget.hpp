#pragma once

#include "pm-structs.hpp"

#include <QWidget>
#include <QSet>

class PmCore;
class QTableWidget;
class QPushButton;
class QComboBox;

class PmMatchListWidget : public QWidget
{
    Q_OBJECT

public:
    PmMatchListWidget(PmCore* core, QWidget* parent);

    int currentIndex() const;

signals:
    void sigChangedMatchConfig(size_t idx, PmMatchConfig cfg);
    
    void sigSelectMatchIndex(size_t matchIndex);
    void sigMoveMatchConfigUp(size_t matchIndex);
    void sigMoveMatchConfigDown(size_t matchIndex);
    void sigInsertMatchConfig(size_t matchIndex, PmMatchConfig cfg);
    void sigRemoveMatchConfig(size_t matchIndex);
    void sigResetMatchConfigs();

protected slots:
    // core event handlers
    void onScenesChanged(PmScenes);
    void onNewMatchResults(size_t idx, PmMatchResults results);
    void onNewMultiMatchConfigSize(size_t sz);
    void onChangedMatchConfig(size_t idx, PmMatchConfig cfg);
    void onSelectMatchIndex(size_t matchIndex, PmMatchConfig config);

    // local UI handlers
    void onRowSelected();
    void onConfigInsertReleased();
    void onConfigRemoveReleased();
    void onConfigMoveUpReleased();
    void onConfigMoveDownReleased();
    void onConfigClearReleased();

protected:
    enum class RowOrder : int { 
        EnableBox = 0, 
        ConfigName = 1, 
        SceneCombo = 2, 
        TransitionCombo = 3,
        Result = 4,
        NumRows = 5,
    };

    static const std::string k_dontSwitchStr;

    void constructRow(int idx);
    void updateAvailableButtons(size_t currIdx, size_t numConfigs);
    void updateSceneChoices(QComboBox* combo);

    void enableConfigToggled(int idx, bool enable);
    //void configRenamed(int idx, const QString& name);
    void matchSceneSelected(int idx, const QString &scene);
    void transitionSelected(int idx, const QString &transition);

    PmCore* m_core;
    QTableWidget *m_tableWidget;

    QPushButton* m_cfgMoveUpBtn;
    QPushButton* m_cfgMoveDownBtn;
    QPushButton* m_cfgInsertBtn;
    QPushButton* m_cfgRemoveBtn;
    QPushButton* m_cfgClearBtn;

    QSet<QString> m_sceneNames;
};