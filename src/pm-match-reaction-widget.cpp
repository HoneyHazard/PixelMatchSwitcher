#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"
#include "pm-core.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default transition>");

PmActionEntryWidget::PmActionEntryWidget(
    PmCore* core, size_t actionIndex, QWidget *parent)
: QWidget(parent)
, m_core(core)
, m_actionIndex(actionIndex)
{
    m_actionTypeCombo = new QComboBox(this);
    m_actionTypeCombo->addItem(
        obs_module_text("<don't switch>"), int(PmActionType::None));
    m_actionTypeCombo->addItem(
        obs_module_text("Scene"), int(PmActionType::Scene));
    m_actionTypeCombo->addItem(
        obs_module_text("SceneItem"), int(PmActionType::SceneItem));
    m_actionTypeCombo->addItem(
        obs_module_text("Filter"), int(PmActionType::Filter));
    m_actionTypeCombo->addItem(
        obs_module_text("Hotkey"), int(PmActionType::Hotkey));
    m_actionTypeCombo->addItem(
        obs_module_text("FrontEndEvent"), int(PmActionType::FrontEndEvent));

    m_targetCombo = new QComboBox(this);

    m_detailsStack = new QStackedWidget(this);

    m_transitionsCombo = new QComboBox(this);
    m_detailsStack->addWidget(m_transitionsCombo);

    m_toggleCombo = new QComboBox(this);
    m_toggleCombo->addItem(obs_module_text("Show"), int(PmToggleCode::Show));
    m_toggleCombo->addItem(obs_module_text("Hide"), int(PmToggleCode::Hide));
    m_detailsStack->addWidget(m_toggleCombo);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_actionTypeCombo);
    mainLayout->addWidget(m_targetCombo);
    mainLayout->addWidget(m_detailsStack);
    setLayout(mainLayout);

    // local UI connections
    connect(m_actionTypeCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onActionTypeSelectionChanged);
    connect(m_targetCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_transitionsCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_toggleCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
}

void PmActionEntryWidget::updateScenes()
{
    QList<std::string> scenes = m_core->sceneNames();

    // TODO: color
    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    for (const std::string& scene : scenes) {
        m_targetCombo->addItem(scene.data());
    }
    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateSceneItems()
{
    QList<std::string> sceneItems = m_core->sceneItemNames();

    // TODO: color
    m_targetCombo->clear();
    m_targetCombo->blockSignals(true);
    for (const std::string& sceneItem : sceneItems) {
        m_targetCombo->addItem(sceneItem.data());
    }
    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateFilters()
{
    QList<std::string> sceneFilters = m_core->allFilters();
    // TODO: color; group by scene items
    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    for (const std::string& filter : sceneFilters) {
        m_targetCombo->addItem(filter.data());
    }
    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateTransitons()
{
    QList<std::string> transitions = m_core->availableTransitions();

    m_transitionsCombo->blockSignals(true);
    m_transitionsCombo->clear();
    m_transitionsCombo->addItem(k_defaultTransitionStr, "");
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
    m_transitionsCombo->blockSignals(false);
}

void PmActionEntryWidget::updateSizeHints(QList<QSize> &columnSizes)
{
    QList<QSize> sizes = {
        m_actionTypeCombo->sizeHint(),
        m_targetCombo->sizeHint(),
        m_detailsStack->sizeHint()
    };
    while (columnSizes.size() < 3) {
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

    PmActionType prevType = actionType();
    if (prevType != action.m_actionType) {
        int findIdx = m_actionTypeCombo->findData(int(action.m_actionType));
        m_actionTypeCombo->blockSignals(true);
        m_actionTypeCombo->setCurrentIndex(findIdx);
        m_actionTypeCombo->blockSignals(false);

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
}

PmActionType PmActionEntryWidget::actionType() const
{
    return (PmActionType)m_actionTypeCombo->currentData().toInt();
}

void PmActionEntryWidget::prepareSelections()
{
    PmActionType actionType
        = PmActionType(m_actionTypeCombo->currentData().toInt());

    switch (actionType) {
    case PmActionType::Scene:
        updateScenes();
        updateTransitons();
        break;
    case PmActionType::SceneItem:
        updateSceneItems();
        break;
    case PmActionType::Filter:
        updateFilters();
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
    action.m_actionType = PmActionType(m_actionTypeCombo->currentData().toInt());

    switch (PmActionType(action.m_actionType)) {
    case PmActionType::None:
        break;
    case PmActionType::Scene:
        action.m_targetElement = m_targetCombo->currentText().toUtf8().data();
        action.m_targetDetails = std::string(
            m_transitionsCombo->currentText().toUtf8().data());
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        action.m_targetElement = m_targetCombo->currentText().toUtf8().data();
        action.m_actionCode = int(m_toggleCombo->currentData().toInt());
        break;
    }

    emit sigActionChanged(m_actionIndex, action);
}

void PmActionEntryWidget::onScenesChanged()
{
    switch (actionType()) {
    case PmActionType::Scene: updateScenes(); updateTransitons(); break;
    case PmActionType::SceneItem: updateSceneItems(); break;
    case PmActionType::Filter: updateFilters(); break;
    }
}

//----------------------------------------------------

PmMatchReactionWidget::PmMatchReactionWidget(
    PmCore *core,
    ReactionTarget reactionTarget, ReactionType reactionType,
    QWidget *parent)
: QGroupBox(parent)
, m_core(core)
, m_reactionTarget(reactionTarget)
, m_reactionType(reactionType)
{
    m_insertActionButton = prepareButton(obs_module_text("Insert New Match Entry"),
        ":/res/images/add.png", "addIconSmall");
    m_removeActionButton = prepareButton(obs_module_text("Remove Match Entry"),
        ":/res/images/list_remove.png",
        "removeIconSmall");
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_insertActionButton);
    buttonLayout->addWidget(m_removeActionButton);

    m_actionListWidget = new QListWidget();

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_actionListWidget);
    setLayout(mainLayout);

    // local UI events
    connect(m_insertActionButton, &QPushButton::released,
            this, &PmMatchReactionWidget::onInsertReleased);

    // entry vs no-match specific
    const auto qc = Qt::QueuedConnection;

    if (reactionTarget == Entry) {
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
        setTitle((m_reactionType == Match)
            ? obs_module_text("Anything Matched")
            : obs_module_text("Nothing Matched")); 
    }
}

void PmMatchReactionWidget::onMatchConfigChanged(
    size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    setTitle(QString(obs_module_text("%1 Actions #%2: %3"))
        .arg((m_reactionType == Match) ?
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
	return m_reactionTarget == Entry
        ? m_core->reaction(m_matchIndex) : m_core->noMatchReaction();
}

void PmMatchReactionWidget::pushReaction(const PmReaction &reaction)
{
    if (m_reactionTarget == Entry) {
        PmMatchConfig cfg = m_core->matchConfig(m_matchIndex);
        cfg.reaction = reaction;
        emit sigMatchConfigChanged(m_matchIndex, cfg);
    } else {
        emit sigNoMatchReactionChanged(reaction);
    }
}

void PmMatchReactionWidget::reactionToUi(const PmReaction &reaction)
{
    const auto &actionList = (m_reactionType == Match) ?
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

    if (m_reactionType == Match) {
        reaction.matchActions[actionIndex] = action;
    } else {
        reaction.unmatchActions[actionIndex] = action;
    }

    pushReaction(reaction);
}

void PmMatchReactionWidget::onInsertReleased()
{
    // TODO: insert at currently selected index
    PmReaction reaction = pullReaction();
    if (m_reactionType == Match) {
        reaction.matchActions.resize(reaction.matchActions.size() + 1);
    } else { // Unmatch
        reaction.unmatchActions.resize(reaction.unmatchActions.size() + 1);
    }
    pushReaction(reaction);
}

QPushButton *PmMatchReactionWidget::prepareButton(
    const char *tooltip, const char *icoPath, const char *themeId)
{
    QIcon icon;
    icon.addFile(icoPath, QSize(), QIcon::Normal, QIcon::Off);

    QPushButton *ret = new QPushButton(icon, "", this);
    ret->setToolTip(tooltip);
    ret->setIcon(icon);
    ret->setIconSize(QSize(16, 16));
    ret->setMaximumSize(22, 22);
    ret->setFlat(true);
    ret->setProperty("themeID", QVariant(themeId));
    ret->setFocusPolicy(Qt::NoFocus);

    return ret;
}
