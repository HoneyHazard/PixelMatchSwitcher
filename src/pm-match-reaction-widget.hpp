#pragma once

#include <QGroupBox>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

#include <vector>

class PmCore;

class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QVBoxLayout;

class PmActionEntryWidget : public QWidget
{
    Q_OBJECT

public:
    PmActionEntryWidget(PmCore *core, size_t actionIndex, QWidget *parent);

    void updateSizeHints(QList<QSize> &columnSizes);

    void updateAction(size_t actionIndex, PmAction action);
    void updateHotkeys(); // TODO
    void updateFrontendEvents(); // TODO

    PmActionType actionType() const;

signals:
    void sigActionChanged(size_t actionIndex, PmAction action);

public slots:
    void onScenesChanged();

protected slots:
    void onActionTypeSelectionChanged();
    void onUiChanged();

protected:
    static const QString k_defaultTransitionStr;

    void prepareSelections();

    void updateScenes();
    void updateSceneItems();
    void updateFilters();
    void updateTransitons();

    QComboBox *m_actionTypeCombo;
    QComboBox *m_targetCombo;

    QComboBox *m_transitionsCombo;
    QComboBox *m_toggleCombo;
    QStackedWidget *m_detailsStack;

    PmCore *m_core;
    size_t m_actionIndex;
};

//----------------------------------------------------

class PmMatchReactionWidget : public QGroupBox
{
    Q_OBJECT

public:
    enum ReactionTarget { Entry, Anything };
    enum ReactionType { Match, Unmatch};

    PmMatchReactionWidget(
        PmCore *core,
        ReactionTarget reactionTarget, ReactionType reactionType,
        QWidget *parent);

signals:
    void sigMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void sigNoMatchReactionChanged(PmReaction reaction);

protected slots:
    // core events
    void onMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig cfg);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onNoMatchReactionChanged(PmReaction reaction);
    //void onActiveFilterChanged(PmFilterRef newAf);
    void onActionChanged(size_t actionIndex, PmAction action);

    // local UI events
    void onInsertReleased();

protected:
    QPushButton *prepareButton(
        const char *tooltip, const char *icoPath, const char *themeId);

    PmReaction pullReaction() const;
    void pushReaction(const PmReaction &reaction);
    void reactionToUi(const PmReaction &reaction);

    QListWidget *m_actionListWidget;
    QPushButton *m_insertActionButton;
    QPushButton *m_removeActionButton;
    //QVBoxLayout *m_actionLayout;

//  QLineEdit *m_lingerEdit;
//  QLineEdit *m_cooldownEdit;

    ReactionTarget m_reactionTarget;
    ReactionType m_reactionType;
    size_t m_matchIndex = (size_t)-1;
    size_t m_multiConfigSz = 0;
    //std::vector<QListWidgetItem*> m_actionItems;

    PmCore *m_core;
};
