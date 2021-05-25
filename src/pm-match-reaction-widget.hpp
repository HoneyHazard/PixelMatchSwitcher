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
class QStackedWidget;
class QVBoxLayout;

class PmActionEntryWidget : public QWidget
{
    Q_OBJECT

public:
    PmActionEntryWidget(size_t actionIndex, QWidget *parent);

    void updateScenes(const QList<std::string> &scenes);
    void updateSceneItems(const QList<std::string> &sceneItems);
    void updateFilters(const QList<std::string> &sceneFilters);
    void updateTransitons(const QList<std::string> &transitions);
    void updateSizeHints(QList<QSize> &columnSizes);

    void updateAction(size_t actionIndex, PmAction action);
    void updateHotkeys(); // TODO
    void updateFrontendEvents(); // TODO

signals:
    void sigActionChanged(size_t actionIndex, PmAction action);

public slots:

protected slots:
    void onUiChanged();

protected:
    static const QString k_defaultTransitionStr;

    QComboBox *m_actionTypeCombo;
    QComboBox *m_targetCombo;

    QComboBox *m_transitionsCombo;
    QComboBox *m_toggleCombo;
    QStackedWidget *m_detailsStack;

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
    void onMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig cfg);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onNoMatchReactionChanged(PmReaction reaction);
    //void onActiveFilterChanged(PmFilterRef newAf);
    void onScenesChanged(
        QList<std::string> scenes, QList<std::string> sceneItems);
    void onActionChanged(size_t actionIndex, PmAction action);

protected:
    QPushButton *prepareButton(
        const char *tooltip, const char *icoPath, const char *themeId);
    void updateReaction(const PmReaction &reaction);

    QListWidget *m_matchActionList;
    QPushButton *m_insertActionButton;
    QPushButton *m_removeActionButton;
    QVBoxLayout *m_actionLayout;

//  QLineEdit *m_lingerEdit;
//  QLineEdit *m_cooldownEdit;

    ReactionTarget m_reactionTarget;
    ReactionType m_reactionType;
    size_t m_matchIndex = (size_t)-1;
    size_t m_multiConfigSz = 0;
    std::vector<PmActionEntryWidget*> m_actionWidgets;

    PmCore *m_core;
};
