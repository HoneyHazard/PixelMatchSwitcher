#pragma once

#include "pm-structs.hpp"

#include <QGroupBox>
#include <QSet>
#include <QLabel>

class PmCore;
class PmReactionWidget;

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
    void sigNoMatchReactionChanged(PmReaction noMatchReaction);

protected slots:
    // core event handlers
    void onScenesChanged(
        QList<std::string> scenes, QList<std::string> sceneItems);
    void onNewMatchResults(size_t idx, PmMatchResults results);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onMatchConfigChanged(size_t idx, PmMatchConfig cfg);
    void onNoMatchReactionChanged(PmReaction noMatchReaction);
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
    void onCellChanged(int row, int col);

protected:
    enum class ColOrder;
    enum class ActionStackOrder;
    static const QStringList k_columnLabels;

    static const QString k_dontSwitchStr;
    static const QString k_sceneItemsLabelStr;
    static const QString k_scenesLabelStr;
    static const QString k_showLabelStr;
    static const QString k_hideLabelStr;

    static const QColor k_dontSwitchColor;
    static const QColor k_scenesLabelColor;
    static const QColor k_scenesColor;
    static const QColor k_sceneItemsLabelColor;
    static const QColor k_sceneItemsColor;

    static const QString k_transpBgStyle;
    static const QString k_semiTranspBgStyle;

    QPushButton* prepareButton(
        const char *tooltip, const char* icoPath, const char* themeId);
    void updateAvailableButtons(size_t currIdx, size_t numConfigs);
    void constructRow(int idx);
    //    const QList<std::string> &sceneItems);
    //void updateTargetChoices(QComboBox* combo,
    //    const QList<std::string> &scenes,
    //    const QList<std::string> &sceneItems);
    //    void updateTargetSelection(
    //        QComboBox *combo, const PmReaction &reaction, bool transparent = false);
    //void updateTransitionChoices(QComboBox* combo);

    void enableConfigToggled(int idx, bool enable);
    void targetSelected(int idx, QComboBox *box);
    void sceneTransitionSelected(int idx, const QString &transTr);
    void sceneItemActionSelected(int idx, const QString &actionStr);
    
    void lingerDelayChanged(int idx, int lingerMs);

    void setMinWidth();
    bool selectRowAtGlobalPos(QPoint globalPos);
    bool eventFilter(QObject *obj, QEvent *event) override; // for display events

    PmCore* m_core;
    QTableWidget *m_tableWidget;

    QPushButton* m_cfgMoveUpBtn;
    QPushButton* m_cfgMoveDownBtn;
    QPushButton* m_cfgInsertBtn;
    QPushButton* m_cfgRemoveBtn;

    //QComboBox* m_noMatchSceneCombo;
    //QComboBox* m_noMatchTransitionCombo;
    PmReactionWidget *m_noMatchReactionWidget;

    int m_prevMatchIndex = 0;
};

class PmResultsLabel : public QLabel
{
    Q_OBJECT

public:
    PmResultsLabel(const QString &text, QWidget *parentWidget);
    QSize sizeHint() const override;

protected:
    int m_resultWidth;
};

class PmReactionLabel : public QLabel
{
    Q_OBJECT

public:
    PmReactionLabel(size_t matchIdx, QWidget *parent);
	void updateReaction(const PmReaction &reaction);

protected:
    size_t matchIdx;
};
