#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QStackWidget>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default>");

PmActionEntryWidget::PmActionEntryWidget(
    PmCore *core, size_t actionIndex, QWidget *parent)
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
    m_transitionsCombo->addItem(k_defaultTransitionStr, "");
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
}

void PmActionEntryWidget::updateAction(size_t actionIndex, PmAction action)
{
    if (m_actionIndex != actionIndex) return;

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
//	    m_targetCombo->setVisible(true);
//	    m_targetCombo->setCurrentText(action.m_targetElement);
//	    m_detailsStack->setCurrentWidget(m_toggleCombo);
//	    findIdx = m_toggleCombo->findData(action.m_actionCode);
//	    m_toggleCombo->setCurrentIndex(findIdx);
//        break;
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
