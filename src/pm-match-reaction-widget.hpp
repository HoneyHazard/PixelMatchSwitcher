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
class QStackWidget;
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

    void updateHotkeys(); // TODO
    void updateFrontendEvents(); // TODO

    void updateAction(PmAction action);

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

//----------------------------------------------------

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
    //void onActiveFilterChanged(PmFilterRef newAf);
    void onScenesChanged(
        QList<std::string> scenes, QList<std::string> sceneItems);

protected:
    QListWidget *m_matchActionList;
	QListWidget *m_unmatchActionList;
	QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QVBoxLayout *m_vertLayout;

    QLineEdit *m_lingerEdit;
    QLineEdit *m_cooldownEdit;

    size_t m_matchIndex = 0;
    size_t m_multiConfigSz = 0;
    std::vector<PmActionEntryWidget*> m_matchActionWidgets;
    std::vector<PmActionEntryWidget*> m_unmatchActionWidget;

    PmCore *m_core;
};
