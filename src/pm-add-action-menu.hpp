#pragma once

#include <QMenu>

#include "pm-structs.hpp"

class PmCore;
class QWidgetAction;

class PmAddActionMenu : public QMenu
{
    Q_OBJECT

public:
    PmAddActionMenu(PmCore *core, QWidget *parent);

    void setTypeAndTarget(PmReactionTarget rTarget, PmReactionType rType)
        { m_reactionTarget = rTarget; m_reactionType = rType; }
    void setMatchIndex(size_t idx) { m_matchIndex = idx; }

signals:
    void sigMatchConfigChanged(size_t matchIdx, PmMatchConfig cfg);
    void sigNoMatchReactionChanged(PmReaction reaction);

protected:
    void itemTriggered(int actionIndex);
	PmReaction pullReaction() const;
	void pushReaction(const PmReaction &reaction);

    PmCore *m_core;
    PmReactionTarget m_reactionTarget = PmReactionTarget::Anything;
	PmReactionType m_reactionType = PmReactionType::Match;
    size_t m_matchIndex = 0;
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
