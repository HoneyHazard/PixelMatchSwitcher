#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"
#include "pm-core.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QListWidget>
#include <QStandardItemModel>
#include <QWidgetAction>
#include <QMenu>
#include <QLabel>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default transition>");

PmActionEntryWidget::PmActionEntryWidget(
    PmCore* core, size_t actionIndex, QWidget *parent)
: QWidget(parent)
, m_core(core)
, m_actionIndex(actionIndex)
{
    m_targetCombo = new QComboBox(this);

    m_detailsStack = new QStackedWidget(this);

    m_transitionsCombo = new QComboBox(this);
    m_detailsStack->addWidget(m_transitionsCombo);

    m_toggleCombo = new QComboBox(this);
    m_toggleCombo->addItem(obs_module_text("Show"), int(PmToggleCode::Show));
    m_toggleCombo->addItem(obs_module_text("Hide"), int(PmToggleCode::Hide));
    m_detailsStack->addWidget(m_toggleCombo);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    //mainLayout->addWidget(m_actionTypeCombo);
    mainLayout->addWidget(m_targetCombo);
    mainLayout->addWidget(m_detailsStack);
    setLayout(mainLayout);

    // local UI connections
    //connect(m_actionTypeCombo, &QComboBox::currentTextChanged,
    //        this, &PmActionEntryWidget::onActionTypeSelectionChanged);
    connect(m_targetCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_transitionsCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_toggleCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);

    onScenesChanged();
}

void PmActionEntryWidget::updateScenes()
{
	QBrush sceneBrush
        = PmAction::dimmedColor(PmActionType::Scene, m_actionType);
    QBrush siBrush = PmAction::dimmedColor(
        PmActionType::SceneItem, m_actionType);
	QBrush fiBrush = PmAction::dimmedColor(
        PmActionType::Filter, m_actionType);

    QList<std::string> scenes = m_core->sceneNames();
    auto model = dynamic_cast<QStandardItemModel *>(m_targetCombo->model());
    int idx;

    PmActionType aType = m_actionType;
    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    QString selStr = QString("<select %1>").arg(PmAction::actionStr(aType));
    m_targetCombo->addItem(selStr, "");
    model->item(0)->setEnabled(false);
    for (const std::string &scene : scenes) {
	    m_targetCombo->addItem(scene.data(), scene.data());
	    idx = m_targetCombo->count() - 1; 
	    m_targetCombo->setItemData(idx, sceneBrush, Qt::TextColorRole);
	    if (aType != PmActionType::Scene) {
		    QList<std::string> siNames = m_core->sceneItemNames(scene);
		    if (siNames.empty()) {
			    m_targetCombo->removeItem(idx);
			    continue;
            }
		    model->item(idx)->setEnabled(false);
		    for (const std::string &siName : siNames) {
			    m_targetCombo->addItem(
                    QString("  ") + siName.data(), siName.data());
			    idx = m_targetCombo->count() - 1;
                m_targetCombo->setItemData(idx, siBrush, Qt::TextColorRole);
			    if (aType != PmActionType::SceneItem) {
                    QList<std::string> fiNames = m_core->filterNames(siName);
				    if (fiNames.empty()) {
                        m_targetCombo->removeItem(idx);
					    continue;
                    }
				    model->item(idx)->setEnabled(false);
                    for (const std::string& fiName : fiNames) {
                        m_targetCombo->addItem(
                            QString("    ") + fiName.data(), fiName.data());
			            idx = m_targetCombo->count() - 1;
                        m_targetCombo->setItemData(
                            idx, fiBrush, Qt::TextColorRole);

                    }
                }
            }
	    }
    }
    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateTransitons()
{
    QList<std::string> transitions = m_core->availableTransitions();

    m_transitionsCombo->blockSignals(true);
    m_transitionsCombo->clear();
    m_transitionsCombo->addItem(k_defaultTransitionStr, QVariant(QString()));
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
    m_transitionsCombo->blockSignals(false);
}

void PmActionEntryWidget::updateSizeHints(QList<QSize> &columnSizes)
{
    QList<QSize> sizes = {
        //m_actionTypeCombo->sizeHint(),
        m_targetCombo->sizeHint(),
        m_detailsStack->sizeHint()
    };
    while (columnSizes.size() < 2) {
        columnSizes.append({0, 0});
    }
    for (int i = 0; i < columnSizes.size(); ++i) {
        columnSizes[i] = {
            qMax(columnSizes[i].width(), sizes[i].width()),
            qMax(columnSizes[i].height(), sizes[i].height())
        };
    }
}

void PmActionEntryWidget::updateAction(size_t actionIndex, PmAction action)
{
    if (m_actionIndex != actionIndex) return;

    PmActionType prevType = m_actionType;
    if (prevType != action.m_actionType) {
	    m_actionType = action.m_actionType;
        prepareSelections();
    }

    switch (action.m_actionType) {
    case PmActionType::None:
        m_targetCombo->setVisible(false);
        m_detailsStack->setVisible(false);
        break;
    case PmActionType::Scene:
        m_targetCombo->setVisible(true);
        m_targetCombo->blockSignals(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_targetCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
        m_detailsStack->setCurrentWidget(m_transitionsCombo);
        m_transitionsCombo->blockSignals(true);
        m_transitionsCombo->setCurrentText(action.m_targetDetails.data());
        m_transitionsCombo->blockSignals(false);
        break;
    case PmActionType::SceneItem:
        m_targetCombo->setVisible(true);
        m_targetCombo->blockSignals(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_targetCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
        m_detailsStack->setCurrentWidget(m_toggleCombo);
        m_toggleCombo->blockSignals(true);
        m_toggleCombo->setCurrentIndex(
            m_toggleCombo->findData(action.m_actionCode));
        m_toggleCombo->blockSignals(false);
        break;
    case PmActionType::Filter:
        m_targetCombo->setVisible(true);
        m_targetCombo->blockSignals(true);
        m_targetCombo->setCurrentText(action.m_targetElement.data());
        m_targetCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
        m_detailsStack->setCurrentWidget(m_toggleCombo);
        m_toggleCombo->blockSignals(true);
        m_toggleCombo->setCurrentIndex(
            m_toggleCombo->findData(action.m_actionCode));
        m_toggleCombo->blockSignals(false);
        break;
    }

    updateUiStyle(action);
}

void PmActionEntryWidget::prepareSelections()
{
    switch (m_actionType) {
    case PmActionType::Scene:
        updateScenes();
        updateTransitons();
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        updateScenes();
        break;
    default:
	    break;
    }
}

void PmActionEntryWidget::onActionTypeSelectionChanged()
{
    prepareSelections();
    onUiChanged();
}

void PmActionEntryWidget::onUiChanged()
{
    PmAction action;
	action.m_actionType = m_actionType;

    switch (PmActionType(m_actionType)) {
    case PmActionType::None:
        break;
    case PmActionType::Scene:
        action.m_targetElement
            = m_targetCombo->currentData().toString().toUtf8().data();
        action.m_targetDetails
            = m_transitionsCombo->currentData().toString().toUtf8().data();
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        action.m_targetElement
            = m_targetCombo->currentData().toString().toUtf8().data();
        action.m_actionCode = int(m_toggleCombo->currentData().toInt());
        break;
    }

    emit sigActionChanged(m_actionIndex, action);

    updateUiStyle(action);
}

void PmActionEntryWidget::onScenesChanged()
{
	prepareSelections();
}

void PmActionEntryWidget::updateUiStyle(const PmAction &action)
{
	QString comboStyle = QString("color: %1").arg(action.actionColorStr());

	//m_actionTypeCombo->setStyleSheet(comboStyle);
	m_targetCombo->setStyleSheet(comboStyle);
	m_transitionsCombo->setStyleSheet(comboStyle);
	m_toggleCombo->setStyleSheet(comboStyle);
}

//----------------------------------------------------

PmMatchReactionWidget::PmMatchReactionWidget(
    PmCore *core,
    PmReactionTarget reactionTarget, PmReactionType reactionType,
    QWidget *parent)
: QGroupBox(parent)
, m_core(core)
, m_reactionTarget(reactionTarget)
, m_reactionType(reactionType)
{
	QMenu *insertMenu = new QMenu(this);
	int typeStart = (int)PmActionType::Scene;
	int typeEnd = (int)PmActionType::FrontEndEvent;
	for (int i = typeStart; i <= typeEnd; i++) {
		PmActionType actType = PmActionType(i);
		QLabel *label = new QLabel(this);
		label->setText(PmAction::actionStr(actType));
		label->setStyleSheet(QString("color: %1").arg(
            PmAction::actionColorStr(actType)));
		label->setMouseTracking(true);

        QWidgetAction *action = new QWidgetAction(this);
        action->setDefaultWidget(label);
		connect(action, &QWidgetAction::triggered,
			    [this, i]() { onInsertReleased(i); });
		insertMenu->addAction(action);
		if (i != typeEnd)
			insertMenu->addSeparator();
	}

	QIcon insertIcon;
	insertIcon.addFile(
        ":/res/images/add.png", QSize(), QIcon::Normal, QIcon::Off);
	m_insertActionButton = new QToolButton(this);
	m_insertActionButton->setIcon(insertIcon);
	m_insertActionButton->setIconSize(QSize(16, 16));
	m_insertActionButton->setMaximumSize(22, 22);
	m_insertActionButton->setProperty("themeID", QVariant("addIconSmall"));
	m_insertActionButton->setToolTip(obs_module_text("Insert New Match Entry"));
	//m_insertActionButton->setFocusPolicy(Qt::NoFocus);
	m_insertActionButton->setPopupMode(QToolButton::InstantPopup);
	m_insertActionButton->setMenu(insertMenu);
	m_insertActionButton->setStyleSheet(
        "QToolButton { border: 0; } "
        "QToolButton::menu-indicator { image: none; }");

    QIcon removeIcon;
	removeIcon.addFile(
        ":/res/images/add.png", QSize(), QIcon::Normal, QIcon::Off);
    m_removeActionButton = new QPushButton(this);
	m_removeActionButton->setIcon(removeIcon);
    m_removeActionButton->setIconSize(QSize(16, 16));
	m_removeActionButton->setMaximumSize(22, 22);
    m_removeActionButton->setFlat(true);
    m_removeActionButton->setProperty("themeID", QVariant("removeIconSmall"));
    m_removeActionButton->setToolTip(obs_module_text("Remove Match Entry"));
    m_removeActionButton->setFocusPolicy(Qt::NoFocus);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_insertActionButton);
    buttonLayout->addWidget(m_removeActionButton);

    m_actionListWidget = new QListWidget();

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_actionListWidget);
    setLayout(mainLayout);

    // local UI events
    //connect(m_insertActionButton, &QPushButton::released,
    //        this, &PmMatchReactionWidget::onInsertReleased);
    connect(m_removeActionButton, &QPushButton::released,
            this, &PmMatchReactionWidget::onRemoveReleased);

    // entry vs no-match specific
    const auto qc = Qt::QueuedConnection;

    if (reactionTarget == PmReactionTarget::Entry) {
        connect(m_core, &PmCore::sigMatchConfigSelect,
                this, &PmMatchReactionWidget::onMatchConfigSelect, qc);
        connect(m_core, &PmCore::sigMatchConfigChanged,
                this, &PmMatchReactionWidget::onMatchConfigChanged, qc);
        connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
                this, &PmMatchReactionWidget::onMultiMatchConfigSizeChanged, qc);

        connect(this, &PmMatchReactionWidget::sigMatchConfigChanged,
                m_core, &PmCore::onMatchConfigChanged);

        onMultiMatchConfigSizeChanged(m_core->multiMatchConfigSize());
        size_t idx = m_core->selectedConfigIndex();
        onMatchConfigSelect(idx, m_core->matchConfig(idx));
    } else { // Anything
        connect(m_core, &PmCore::sigNoMatchReactionChanged,
                this, &PmMatchReactionWidget::onNoMatchReactionChanged, qc);
        connect(this, &PmMatchReactionWidget::sigNoMatchReactionChanged,
                m_core, &PmCore::onNoMatchReactionChanged, qc);
        setTitle((m_reactionType == PmReactionType::Match)
            ? obs_module_text("Anything Matched")
            : obs_module_text("Nothing Matched"));
        onNoMatchReactionChanged(m_core->noMatchReaction());
    }
}

void PmMatchReactionWidget::onMatchConfigChanged(
    size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    setTitle(QString(obs_module_text("%1 Actions #%2: %3"))
        .arg((m_reactionType == PmReactionType::Match) ?
                obs_module_text("Match") : obs_module_text("Unmatch"))
        .arg(matchIdx + 1)
        .arg(cfg.label.data()));

    const PmReaction &reaction = cfg.reaction;
    reactionToUi(reaction);
}

void PmMatchReactionWidget::onNoMatchReactionChanged(PmReaction reaction)
{
    reactionToUi(reaction);
}

PmReaction PmMatchReactionWidget::pullReaction() const
{
    return m_reactionTarget == PmReactionTarget::Entry
        ? m_core->reaction(m_matchIndex) : m_core->noMatchReaction();
}

void PmMatchReactionWidget::pushReaction(const PmReaction &reaction)
{
    if (m_reactionTarget == PmReactionTarget::Entry) {
        PmMatchConfig cfg = m_core->matchConfig(m_matchIndex);
        cfg.reaction = reaction;
        emit sigMatchConfigChanged(m_matchIndex, cfg);
    } else {
        emit sigNoMatchReactionChanged(reaction);
    }
}

void PmMatchReactionWidget::reactionToUi(const PmReaction &reaction)
{
    const auto &actionList = (m_reactionType == PmReactionType::Match) ?
        reaction.matchActions : reaction.unmatchActions;

    size_t listSz = actionList.size();
    for (size_t i = 0; i < listSz; ++i) {
        PmActionEntryWidget *entryWidget = nullptr;
        if (i >= m_actionListWidget->count()) {
            const auto qc = Qt::QueuedConnection;
            entryWidget = new PmActionEntryWidget(m_core, i, this);
            connect(m_core, &PmCore::sigScenesChanged,
                    entryWidget, &PmActionEntryWidget::onScenesChanged, qc);
            connect(entryWidget, &PmActionEntryWidget::sigActionChanged,
                    this, &PmMatchReactionWidget::onActionChanged, qc);

            QListWidgetItem *item = new QListWidgetItem(m_actionListWidget);
            m_actionListWidget->addItem(item);
            item->setSizeHint(entryWidget->sizeHint());
            m_actionListWidget->setItemWidget(item, entryWidget);
        } else {
            QListWidgetItem* item = m_actionListWidget->item(int(i));
            entryWidget
                = (PmActionEntryWidget*) m_actionListWidget->itemWidget(item);
        }
        entryWidget->updateAction(i, actionList[i]);
    }
    while (m_actionListWidget->count() > listSz) {
        QListWidgetItem *item = m_actionListWidget->takeItem(
            m_actionListWidget->count() - 1);
        delete item;
    }
}

void PmMatchReactionWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;

    setEnabled(matchIndex < m_multiConfigSz);

    onMatchConfigChanged(matchIndex, cfg);
}

void PmMatchReactionWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    m_multiConfigSz = sz;
    setEnabled(m_matchIndex < m_multiConfigSz);
}

void PmMatchReactionWidget::onActionChanged(size_t actionIndex, PmAction action)
{
    PmReaction reaction = pullReaction();

    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions[actionIndex] = action;
    } else {
        reaction.unmatchActions[actionIndex] = action;
    }

    pushReaction(reaction);
}

void PmMatchReactionWidget::onInsertReleased(int actionIdx)
{
    size_t idx = 0;
    if (m_actionListWidget->count() > 0) {
        int currRow = m_actionListWidget->currentRow();
        if (currRow == -1) {
            idx = m_actionListWidget->count();
        } else {
            idx = (size_t)currRow;
        }
    }

    PmAction newAction;
    newAction.m_actionType = (PmActionType)actionIdx;

    PmReaction reaction = pullReaction();
    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions.insert(
            reaction.matchActions.begin() + idx, newAction);
    } else { // Unmatch
        reaction.unmatchActions.insert(
            reaction.unmatchActions.begin() + idx, newAction);
    }
    pushReaction(reaction);
}

void PmMatchReactionWidget::onRemoveReleased()
{
    int idx = 0;
    if (m_actionListWidget->count() > 0) {
        idx = (size_t)m_actionListWidget->currentRow();
        if (idx < 0)
            return;
    } else {                       
        return;
    }

    PmReaction reaction = pullReaction();
    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions.erase(reaction.matchActions.begin() + idx);
    } else { // Unmatch
        reaction.unmatchActions.erase(reaction.unmatchActions.begin() + idx);
    }
    pushReaction(reaction);
}


