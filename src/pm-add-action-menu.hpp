#pragma once

#include <QMenu>

#include "pm-structs.hpp"

class PmCore;
class QWidgetAction;
class QLabel;

class PmAddActionMenu : public QMenu
{
    Q_OBJECT

public:
    PmAddActionMenu(PmCore *core, QWidget *parent);

    void setTypeAndTarget(PmReactionTarget rTarget, PmReactionType rType);
    void setMatchIndex(size_t idx);

signals:
    void sigMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void sigNoMatchReactionChanged(PmReaction reaction);

protected slots:
    void updateActionsState();

protected:
    void itemTriggered(int actionIndex);
	PmReaction pullReaction() const;
	void pushReaction(const PmReaction &reaction);
	void updateTitle();

    PmCore *m_core;
    PmReactionTarget m_reactionTarget = PmReactionTarget::Global;
	PmReactionType m_reactionType = PmReactionType::Match;
    QWidgetAction *m_sceneAction = nullptr;
	QLabel *m_titleLabel = nullptr;
    size_t m_matchIndex = 0;
};

/**
 * @brief https://stackoverflow.com/questions/55086498/highlighting-custom-qwidgetaction-on-hover
 */
class PmAddActionMenuHelper : public QObject
{
	Q_OBJECT

public:
	PmAddActionMenuHelper(QMenu *menu, QObject *parent);

protected:
	bool eventFilter(QObject *target, QEvent *e) override;
	void highlight(QWidgetAction *qwa, bool h);

	QMenu *m_menu;
	QWidgetAction *m_lastQwa = nullptr;
};
