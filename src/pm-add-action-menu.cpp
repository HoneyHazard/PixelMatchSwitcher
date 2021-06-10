#include "pm-add-action-menu.hpp"
#include "pm-core.hpp"

#include <QLabel>
#include <QWidgetAction>
#include <QMouseEvent>

PmAddActionMenu::PmAddActionMenu(PmCore *core, QWidget *parent)
: QMenu(parent)
, m_core(core)
{
	const auto qc = Qt::QueuedConnection;

	connect(this, &PmAddActionMenu::sigMatchConfigChanged,
		m_core, &PmCore::onMatchConfigChanged, qc);
	connect(this, &PmAddActionMenu::sigNoMatchReactionChanged,
			m_core, &PmCore::onNoMatchReactionChanged, qc);

    int typeStart = (int)PmActionType::Scene;
	int typeEnd = (int)PmActionType::FrontEndEvent;
	for (int i = typeStart; i <= typeEnd; i++) {
		PmActionType actType = PmActionType(i);
		QString labelText = PmAction::actionStr(actType);
		QLabel *itemLabel = new QLabel(labelText, this);
		QString colorStr = PmAction::actionColorStr(actType);
		itemLabel->setStyleSheet(QString("color: ") + colorStr);
		itemLabel->setMouseTracking(true);
		QWidgetAction *qwa = new QWidgetAction(this);
		qwa->setData(colorStr);
		qwa->setDefaultWidget(itemLabel);
		connect(qwa, &QWidgetAction::triggered,
			[this, i]() { itemTriggered(i); });
		addAction(qwa);

		if (i != typeEnd)
            addSeparator();
	}
	PmMenuHelper *menuHelper = new PmMenuHelper(this, this);
}

void PmAddActionMenu::itemTriggered(int actionIndex)
{
	PmAction newAction;
	newAction.m_actionType = (PmActionType)actionIndex;
	PmReaction reaction = pullReaction();
	if (m_reactionType == PmReactionType::Match) {
		reaction.matchActions.push_back(newAction);
	} else { // Unmatch
		reaction.unmatchActions.push_back(newAction);
	}
	pushReaction(reaction);
}

PmReaction PmAddActionMenu::pullReaction() const
{
	return m_reactionTarget == PmReactionTarget::Entry
		? m_core->reaction(m_matchIndex)
		: m_core->noMatchReaction();
}

void PmAddActionMenu::pushReaction(const PmReaction &reaction)
{
	if (m_reactionTarget == PmReactionTarget::Entry) {
		PmMatchConfig cfg = m_core->matchConfig(m_matchIndex);
		cfg.reaction = reaction;
		emit sigMatchConfigChanged(m_matchIndex, cfg);
	} else {
		emit sigNoMatchReactionChanged(reaction);
	}
}

//----------------------------------------------

PmMenuHelper::PmMenuHelper(QMenu *menu, QObject *parent)
	: QObject(parent), m_menu(menu)
{
	menu->installEventFilter(this);
}

bool PmMenuHelper::eventFilter(QObject *target, QEvent *e)
{
	if (e->type() == QEvent::Type::MouseMove) {
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
		QAction *a = static_cast<QMenu *>(target)->actionAt(
			mouseEvent->pos());
		QWidgetAction *qwa = dynamic_cast<QWidgetAction *>(a);
		if (qwa) {
			if (m_lastQwa && qwa != m_lastQwa) {
				highlight(m_lastQwa, false);
			}
			highlight(qwa, true);
			m_lastQwa = qwa;
		} else {
			if (m_lastQwa) {
				highlight(m_lastQwa, false);
				m_lastQwa = nullptr;
			}
		}
	}
	return QObject::eventFilter(target, e);
}

void PmMenuHelper::highlight(QWidgetAction *qwa, bool h)
{
	QString colorStr = qwa->data().toString();
	QLabel *label = (QLabel *)qwa->defaultWidget();
	QString styleSheet = QString("color: %1 %2")
				     .arg(colorStr)
				     .arg(h ? "; font-weight: bold" : "");
	label->setStyleSheet(styleSheet);

#if 0
	mainWidget->setBackgroundRole(h ? QPalette::Highlight
					: QPalette::Window);
	mainWidget->setAutoFillBackground(h);
#endif
}
