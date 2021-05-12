#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"

#include <obs-module.h>

#include <QComboBox>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default>");

PmActionEntryWidget::PmActionEntryWidget(
    PmCore *core, size_t actionIndex, QWidget *parent)
{
    m_targetCombo = QComboBox(this);
    m_actionDetailsCombo = QComboBox(this);

    m_actionTypeCombo = QComboBox(this);
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

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addItem(m_actionTypeCombo);
    mainLayout->addItem(m_targetCombo);
    mainLayout->addItem(m_actionDetailsCombo);
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
	// TODO: color
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

}

void PmActionEntryWidget::updateAction(size_t actionIndex, PmAction action)
{
    if (m_actionIndex != actionIndex) return;

    switch (action.m_actionType) {
    case PmActionType::Scene:
	    m_targetCombo->setCurrentText(action.m_targetElement);
	    break;
    }
}
