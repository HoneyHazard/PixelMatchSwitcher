#pragma once

#include <QGroupBox>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

class PmCore;

class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QStackWidget;

class PmActionEntryWidget : public QWidget
{
    Q_OBJECT

public:
    PmActionEntryWidget(PmCore *core, size_t actionIndex, QWidget *parent);

    void updateScenes(const QList<std::string> &scenes);
    void updateSceneItems(const QList<std::string> &sceneItems);
    void updateFilters(const QList<std::string> &sceneFilters);
    void updateTransitons(const QList<std::string> &transitions);

    void updateHotkeys(); // TODO
    void updateFrontendEvents(); // TODO

    void updateAction(size_t actionIndex, PmAction action);

signals:
    void sigActionChanged(size_t actionIndex, PmAction action);

protected slots:
    void onUiChanged();

protected:
    static const QString k_defaultTransitionStr;

    QComboBox *m_actionTypeCombo;
    QComboBox *m_targetCombo;

    QComboBox *m_transitionsCombo;
    QComboBox *m_toggleCombo;
    QStackWidget *m_detailsStack;

    size_t m_actionIndex;
};

class PmMatchReactionWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmMatchReactionWidget(PmCore *core, QWidget *parent);

signals:
    void sigMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);

protected slots:
    void onMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig cfg);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onActiveFilterChanged(PmFilterRef newAf);

protected:
    QListWidget *m_actionList;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;

    QLineEdit *m_lingerEdit;
    QLineEdit *m_cooldownEdit;

    size_t m_matchIndex = 0;
    size_t m_multiConfigSz = 0;

    PmCore *m_core;
};
