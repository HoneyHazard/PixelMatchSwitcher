#pragma once

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"
#include "pm-spoiler-widget.hpp"

#include <vector>

class PmCore;

class QLineEdit;
class QPushButton;
class QToolButton;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QVBoxLayout;
class QScrollArea;
class QWidgetAction;
class QEvent;

class PmActionEntryWidget : public QWidget
{
    Q_OBJECT

public:
    PmActionEntryWidget(PmCore *core, size_t actionIndex, QWidget *parent);

    void updateSizeHints(QList<QSize> &columnSizes);

    void updateAction(size_t actionIndex, PmAction action);
    void updateHotkeys(); // TODO
    void updateFrontendEvents(); // TODO

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
    void updateTransitons();
    void updateUiStyle(const PmAction& action);

    QComboBox *m_targetCombo;

    QComboBox *m_transitionsCombo;
    QComboBox *m_toggleCombo;
    QStackedWidget *m_detailsStack;

    PmCore *m_core;
    size_t m_actionIndex;
    PmActionType m_actionType = PmActionType::None;
};

//----------------------------------------------------

class PmMatchReactionWidget : public PmSpoilerWidget
{
    Q_OBJECT

public:
    PmMatchReactionWidget(
        PmCore *core,
        PmReactionTarget reactionTarget, PmReactionType reactionType,
        QWidget *parent);

signals:
    void sigMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void sigNoMatchReactionChanged(PmReaction reaction);

public slots:
    void toggleExpand(bool on) override;

protected slots:
    // core events
    void onMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void onMatchConfigSelect(size_t matchIndex, PmMatchConfig cfg);
    void onMultiMatchConfigSizeChanged(size_t sz);
    void onNoMatchReactionChanged(PmReaction reaction);
    void onActionChanged(size_t actionIndex, PmAction action);

    // local UI events
    void onInsertReleased(int actionTypeIndex);
    void onRemoveReleased();
    void updateButtonsState();
    void updateTitle();

protected:
    PmReaction pullReaction() const;
    void pushReaction(const PmReaction &reaction);
    void reactionToUi(const PmReaction &reaction);
    int maxContentHeight() const override;

    //QScrollArea *m_scrollArea;
    QListWidget *m_actionListWidget;
    QPushButton *m_insertActionButton;
    QPushButton *m_removeActionButton;

    PmReactionTarget m_reactionTarget;
    PmReactionType m_reactionType;
    size_t m_matchIndex = (size_t)-1;
    size_t m_multiConfigSz = 0;
    int m_lastActionCount = -1;

    PmCore *m_core;
};


/**
 * @brief https://stackoverflow.com/questions/55086498/highlighting-custom-qwidgetaction-on-hover
 */
class PmMenuHelper : public QObject
{
    Q_OBJECT

public:
    PmMenuHelper(QMenu *menu, QObject *parent);

protected:
    bool eventFilter(QObject *target, QEvent *e) override;
    void highlight(QWidgetAction *qwa, bool h);

	QMenu *m_menu;
	QWidgetAction *m_lastQwa = nullptr;
};
