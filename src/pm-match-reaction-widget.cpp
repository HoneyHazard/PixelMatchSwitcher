#include "pm-match-reaction-widget.hpp"
#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"
#include "pm-core.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QStackWidget>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default>");

PmActionEntryWidget::PmActionEntryWidget(size_t actionIndex, QWidget *parent)
{
    m_actionTypeCombo = new QComboBox(this);
    m_actionTypeCombo->addItem(
        obs_module_text("<don't switch>"), int(PmActionType::None));
    m_actionTypeCombo->addItem(
        obs_module_text("Scene"), int(PmActionType::Scene));
    m_actionTypeCombo->addItem(
        obs_module_text("Filter"), int(PmActionType::Filter));
    m_actionTypeCombo->addItem(
        obs_module_text("Hotkey"), int(PmActionType::Hotkey));
    m_actionTypeCombo->addItem(
        obs_module_text("FrontEndEvent"), int(PmActionType::FrontEndEvent));

    m_targetCombo = new QComboBox(this);

    m_transitionsCombo = new QComboBox(this);
    m_detailsStack->addWidget(m_transitionsCombo);

    m_toggleCombo = new QComboBox(this);
    m_toggleCombo->addItem(obs_module_text("Show"), int(PmToggleCode::Show));
    m_toggleCombo->addItem(obs_module_text("Hide"), int(PmToggleCode::Hide));
    m_detailsStack->addWidget(m_toggleCombo);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addItem(m_actionTypeCombo);
    mainLayout->addItem(m_targetCombo);
    mainLayout->addItem(m_detailsStack);
    setLayout(mainLayout);
}

void PmActionEntryWidget::updateScenes(const QList<std::string> &scenes)
{
    // TODO: color
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData());
    if (actionType != PmActionType::Scene) return;

    m_targetCombo->clear();
    for (const std::string& scene : scenes) {
        m_targetCombo->addItem(scene.data());
    }
}

void PmActionEntryWidget::updateSceneItems(const QList<std::string> &sceneItems)
{
    // TODO: color
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData());
    if (actionType != PmActionType::SceneItem) return;

    m_targetCombo->clear();
    for (const std::string& sceneItem : sceneItems) {
        m_targetCombo->addItem(sceneItem.data());
    }
}

void PmActionEntryWidget::updateFilters(const QList<std::string> &sceneFilters)
{
    // TODO: color; group by scene items
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData());
    if (actionType != PmActionType::Filter) return;

    m_targetCombo->clear();
    for (const std::string& filter : sceneItems) {
        m_targetCombo->addItem(sceneItem.data());
    }
}

void PmActionEntryWidget::updateTransitons(
    const QList<std::string> &transitions)
{
    m_transitionsCombo->clear();
    m_transitionsCombo->addItem(k_defaultTransitionStr, "");
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
}

void PmActionEntryWidget::updateSizeHints(QList<QSize> &columnSizes)
{
    QSize hint = m_actionTypeCombo->sizeHint();
    columnSizes[0] = qMax(columnSizes[0], hint);

    hint = m_targetCombo->sizeHint();
    columnSizes[1] = qMax(columnSizes[1], hint);

    hint = m_detailsStack->sizeHint();
    columnSizes[2] = qMax(columnSizes[2], hint);
}

void PmActionEntryWidget::updateAction(PmAction action)
{
    int findIdx = m_actionTypeCombo->findData(int(action.m_actionCode));
    m_actionTypeCombo->setCurrentIndex(findIdx);

    switch (action.m_actionType) {
    case PmActionType::None:
        m_targetCombo->setVisible(false);
        break;
    case PmActionType::Scene:
        m_targetCombo->setVisible(true);
        m_targetCombo->setCurrentText(action.m_targetElement);
        m_detailsStack->setCurrentWidget(m_transitionsCombo);
        m_transitionCombo->setCurrentText(action.m_targetDetails);
        break;
    case PmActionType::SceneItem:
        m_targetCombo->setVisible(true);
        m_targetCombo->setCurrentText(action.m_targetElement);
        m_detailsStack->setCurrentWidget(m_toggleCombo);
        findIdx = m_toggleCombo->findData(action.m_actionCode);
        m_toggleCombo->setCurrentIndex(findIdx);
        break;
    case PmActionType::Filter:
        m_targetCombo->setVisible(true);
        m_targetCombo->setCurrentText(action.m_targetElement);
        m_detailsStack->setCurrentWidget(m_toggleCombo);
        findIdx = m_toggleCombo->findData(action.m_actionCode);
        m_toggleCombo->setCurrentIndex(findIdx);
        break;
    }
}

void PmActionEntryWidget::onUiChanged()
{
    PmAction action;
    action.m_actionCode = int(m_actionTypeCombo->currentData());

    switch (action.m_actionCode) {
    case PmActionType::None:
        break;
    case PmActionType::Scene:
        action.m_targetElement = m_targetCombo->currentText();
        action.m_targetDetails = std::string(
            QString(m_transitionCombo->currentData());
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        action.m_targetElement = m_targetCombo->currentText();
        action.m_actionCode = int(m_toggleCombo->currentData());
        break;
    }

    emit sigActionChanged(m_actionIndex, action);
}

//----------------------------------------------------

PmMatchReactionWidget::PmMatchReactionWidget(PmCore *core, QWidget *parent)
: QGroupBox(parent)
, m_core(core)
{
    const auto qc = Qt::QueuedConnection;
    
    connect(m_core, &PmCore::sigMatchConfigSelect,
            this, &PmMatchReactionWidget::onMatchConfigSelect, qc);
    connect(m_core, &PmCore::sigMatchConfigChanged,
            this, &PmMatchReactionWidget::onMatchConfigChanged, qc);
    connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
            this, &PmMatchReactionWidget::onMultiMatchConfigSizeChanged, qc);

    //----------------------------------

    //onActiveFilterChanged(m_core->activeFilterRef());
    onMultiMatchConfigChanged(m_core->multiMatchConfigSize());
    size_t idx = m_core->selectedConfigIndex();
    onMatchConfigSelect(idx, m_core->matchConfig(idx));
}

void PmMatchReactionWidget::onMatchConfigChanged(
    size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    setTitle(QString(obs_module_text("Match Reaction #%1: %2"))
        .arg(matchIdx + 1)
        .arg(cfg.label.data()));

    const PmReaction &reaction = cfg.reaction;
    size_t matchSz = reaction.matchSz();
    for (size_t i = 0; i < matchSz; ++i) {
        if (i < matchSz && i > m_matchActionWidgets.size()) {
            auto *entryWidget = new PmActionEntryWidget(i, parent);
        }
    }

    m_reactionSz = cfg.reaction.

    updateAction(cfg);
}

void PmMatchReactionWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_actionIndex = matchIndex;

    setEnabled(matchIndex < m_multiConfigSz);

    onMatchConfigChanged(matchIndex, cfg);
}

void PmMatchReactionWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    m_multiConfigSz = sz;
    setEnabled(m_matchIndex < m_multiConfigSz);
}

void PmMatchReactionWidget::onScenesChanged(
    QList<std::string> scenes, QList<std::string> sceneItems)
{
    QList<std::string> filterNames;
    for (const std::string siName : sceneItems) {
        filterNames.append(m_core->filters(siName));
    }

    for (auto *w : m_matchActionWidgets) {
        w->updateScenes(scenes);
        w->updateSceneItems(sceneItems);
        w->updateFilters(filterNames);
    }
    for (auto *w : m_unmatchActionWidgets) {
        w->updateScenes(scenes);
        w->updateSceneItems(sceneItems);
        w->updateFilters(filterNames);
    }


}


