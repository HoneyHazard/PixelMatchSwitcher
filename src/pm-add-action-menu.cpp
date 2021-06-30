#include "pm-add-action-menu.hpp"
#include "pm-core.hpp"

#include <QLabel>
#include <QWidgetAction>
#include <QMouseEvent>

#include <sstream>

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

    m_titleLabel = new QLabel(this);
    QWidgetAction *labelQwa = new QWidgetAction(this);
    labelQwa->setDefaultWidget(m_titleLabel);
    labelQwa->setEnabled(false);
    addAction(labelQwa);
    addSeparator();

    int typeStart = (int)PmActionType::Scene;
    int typeEnd = (int)PmActionType::ANY - 1;
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

        if (i == (int)PmActionType::Scene)
            m_sceneAction = qwa;
	}
	new PmAddActionMenuHelper(this, this);
}

void PmAddActionMenu::setTypeAndTarget(PmReactionTarget rTarget,
				       PmReactionType rType)
{
	m_reactionTarget = rTarget;
	m_reactionType = rType;
	updateTitle();
}

void PmAddActionMenu::setMatchIndex(size_t idx)
{
	m_matchIndex = idx;
	updateTitle();
}

void PmAddActionMenu::itemTriggered(int actionIndex)
{
	PmAction newAction;
	newAction.actionType = (PmActionType)actionIndex;
	if (newAction.actionType == PmActionType::File) {
		newAction.actionCode = (size_t)PmFileActionType::WriteAppend;
		std::ostringstream oss;
		oss << PmAction::k_timeMarker << ' '
		    << PmAction::k_labelMarker << ' '
            << ": "
		    << (m_reactionType == PmReactionType::Match ? "match" : "unmatch")
            << " text placeholder";
		newAction.targetDetails = oss.str();
		newAction.timeFormat = PmAction::k_defaultFileTimeFormat;
    }
	PmReaction reaction = pullReaction();

	if (m_reactionType == PmReactionType::Match) {
		if (newAction.actionType == PmActionType::Scene
         && reaction.hasMatchAction(PmActionType::Scene)) {
			return; // enforce one scene action
        }
		reaction.matchActions.push_back(newAction);
	} else { // Unmatch
		if (newAction.actionType == PmActionType::Scene
         && reaction.hasUnmatchAction(PmActionType::Scene)) {
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

void PmAddActionMenu::updateTitle()
{
	QString typeStr = (m_reactionType == PmReactionType::Match) ?
        obs_module_text("Match") : obs_module_text("Unmatch");
	QString str;
	if (m_reactionTarget == PmReactionTarget::Global) {
		str = QString(obs_module_text("Add Global %1 Action:")).arg(typeStr);
	} else {
		str = QString(obs_module_text("Add %1 Action to #%2: "))
            .arg(typeStr).arg(m_matchIndex+1);
    }
	m_titleLabel->setText(str);
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

    QString bgColorStr = m_menu->palette().color(QPalette::Window).name();
	QString colorStr = qwa->data().toString();

	QLabel *label = dynamic_cast<QLabel*>(qwa->defaultWidget());
	if (label) {
	    QString styleSheet = QString("color: %1; background: %2%3")
				         .arg(h ? bgColorStr : colorStr)
                         .arg(h ? colorStr : bgColorStr)
				         .arg(h ? "; font-weight: bold" : "");
	    label->setStyleSheet(styleSheet);
    }
}
