#pragma once

#include "pm-structs.hpp"

#include <QGroupBox>
#include <QSet>

class PmCore;
class QTableWidget;
class QPushButton;
class QComboBox;
class QTableWidgetItem;

/**
 * @brief Shows a list of match configuration entries and allows changing
 *        some of their parameters
  */
class PmMatchListWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmMatchListWidget(PmCore* core, QWidget* parent);

    int currentIndex() const;

signals:
    void sigMatchConfigChanged(size_t idx, PmMatchConfig cfg);
    
    void sigMatchConfigSelect(size_t matchIndex);
    void sigMatchConfigMoveUp(size_t matchIndex);
    void sigMatchConfigMoveDown(size_t matchIndex);
    void sigMatchConfigInsert(size_t matchIndex, PmMatchConfig cfg);
    void sigMatchConfigRemove(size_t matchIndex);
    void sigNoMatchSceneChanged(std::string noMatchScene);
    void sigNoMatchTransitionChanged(std::string transName);

protected slots:
    // core event handlers
    void onScenesChanged(PmScenes);
    void onNewMatchResults(size_t idx, PmMatchResults results);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onMatchConfigChanged(size_t idx, PmMatchConfig cfg);
    void onNoMatchSceneChanged(std::string sceneName);
    void onNoMatchTransitionChanged(std::string transName);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig config);

    // local UI handlers
    void onRowSelected();
    void onItemChanged(QTableWidgetItem* item);
    void onConfigInsertButtonReleased();
    void onConfigRemoveButtonReleased();
    void onConfigMoveUpButtonReleased();
    void onConfigMoveDownButtonReleased();
    void onNoMatchSceneSelected(QString str);
    void onNoMatchTransitionSelected(QString str);

protected:
    enum class ColOrder;
    static const QStringList k_columnLabels;
    static const QString k_dontSwitchStr;
    static const QString k_transpBgStyle;
    static const QString k_semiTranspBgStyle;

    QPushButton* prepareButton(
        const char *tooltip, const char* icoPath, const char* themeId);
    void constructRow(int idx);
    void updateAvailableButtons(size_t currIdx, size_t numConfigs);
    void updateSceneChoices(QComboBox* combo);
    void updateTransitionChoices(QComboBox* combo);

    void enableConfigToggled(int idx, bool enable);
    void matchSceneSelected(int idx, const QString &scene);
    void matchTransitionSelected(int idx, const QString& transName);
    void lingerDelayChanged(int idx, int lingerMs);

    void setMinWidth();
    bool eventFilter(QObject *obj, QEvent *event) override; // for display events

    PmCore* m_core;
    QTableWidget *m_tableWidget;

    QPushButton* m_cfgMoveUpBtn;
    QPushButton* m_cfgMoveDownBtn;
    QPushButton* m_cfgInsertBtn;
    QPushButton* m_cfgRemoveBtn;

    QComboBox* m_noMatchSceneCombo;
    QComboBox* m_noMatchTransitionCombo;

    QSet<std::string> m_sceneNames;
    int m_prevMatchIndex = 0;

    int m_leftMargin, m_rightMargin;
};
