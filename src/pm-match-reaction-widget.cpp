#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"

#include <obs-module.h>

#include <QComboBox>

PmActionEntryWidget::PmActionEntryWidget(
    PmCore *core, size_t actionIndex, QWidget *parent)
{
    m_targetCombo = QComboBox(this);
    m_actionDetailsCombo = QComboBox(this);

    m_actionTypeCombo = QComboBox(this);
    m_actionTypeCombo->addItem(
        obs_module_text("<don't switch>"), int(PmActionType::None));
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

void PmActionEntryWidget::updateScenes(const QList<std::string> scenes)
{
    PmActionType actionType = PmActionType(
        int(m_actionTypeCombo->currentData());
    if (actionType != PmActionType::Scene) return;

    m_targetCombo->clear();
}

