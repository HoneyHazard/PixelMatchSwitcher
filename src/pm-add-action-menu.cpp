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

    connect(this, &QMenu::aboutToShow,
            this, &PmAddActionMenu::updateActionsState);

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

        if (i == (int)PmActionType::Scene)
            m_sceneAction = qwa;
	}
	new PmAddActionMenuHelper(this, this);
}

void PmAddActionMenu::itemTriggered(int actionIndex)
{
	PmAction newAction;
	newAction.m_actionType = (PmActionType)actionIndex;
	PmReaction reaction = pullReaction();

	if (m_reactionType == PmReactionType::Match) {
		if (newAction.m_actionType == PmActionType::Scene
         && reaction.hasMatchAction(PmActionType::Scene)) {
			return; // enforce one scene action
        }
		reaction.matchActions.push_back(newAction);
	} else { // Unmatch
		if (newAction.m_actionType == PmActionType::Scene
         || reaction.hasUnmatchAction(PmActionType::Scene)) {
			return; // enforce one scene action
		}
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

void PmAddActionMenu::updateActionsState()
{
	if (m_sceneAction) {
		bool enableSceneAction;
		if (m_reactionTarget == PmReactionTarget::Entry) {
			if (m_reactionType == PmReactionType::Match) {
				enableSceneAction = !m_core->hasMatchAction(
					m_matchIndex, PmActionType::Scene);
			} else { // Unmatch
				enableSceneAction = !m_core->hasUnmatchAction(
					m_matchIndex, PmActionType::Scene);
			}
		} else { // Global
			if (m_reactionType == PmReactionType::Match) {
				enableSceneAction =
					!m_core->hasGlobalMatchAction(PmActionType::Scene);
			} else { // Unmatch
				enableSceneAction =
					!m_core->hasGlobalUnmatchAction(PmActionType::Scene);
			}
		}
		m_sceneAction->setEnabled(enableSceneAction);
		QString colorStr;
		if (enableSceneAction) {
			colorStr = m_sceneAction->data().toString();
		} else {
            colorStr = palette().color(
                QPalette::Disabled, QPalette::Text).name();
        }
		QString styleSheetStr = QString("color: %1").arg(colorStr);
	    QLabel *label = (QLabel *)m_sceneAction->defaultWidget();
		label->setStyleSheet(styleSheetStr);
    }
}

//----------------------------------------------

PmAddActionMenuHelper::PmAddActionMenuHelper(QMenu *menu, QObject *parent)
	: QObject(parent), m_menu(menu)
{
	menu->installEventFilter(this);
}

bool PmAddActionMenuHelper::eventFilter(QObject *target, QEvent *e)
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

void PmAddActionMenuHelper::highlight(QWidgetAction *qwa, bool h)
{
	if (!qwa->isEnabled()) return;

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
