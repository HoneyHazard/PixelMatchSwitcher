#include "pm-toggles-widget.hpp"
#include "pm-core.hpp"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QGroupBox>

#include <obs-module.h>

PmTogglesWidget::PmTogglesWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    // global toggles
    QHBoxLayout* togglesLayout = new QHBoxLayout;

    m_runningCheckbox = new QCheckBox(
        obs_module_text("Enable Pixel Match Switcher"), this);
    connect(m_runningCheckbox, &QCheckBox::toggled,
        this, &PmTogglesWidget::sigRunningEnabledChanged, Qt::QueuedConnection);

    m_switchingCheckbox = new QCheckBox(
        obs_module_text("Enable Switching"), this);
    connect(m_switchingCheckbox, &QCheckBox::toggled,
        this, &PmTogglesWidget::sigSwitchingEnabledChanged, Qt::QueuedConnection);

    togglesLayout->addWidget(m_runningCheckbox);
    togglesLayout->addWidget(m_switchingCheckbox);
    setLayout(togglesLayout);


    // core event handlers
    connect(m_core, &PmCore::sigRunningEnabledChanged,
        this, &PmTogglesWidget::onRunningEnabledChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSwitchingEnabledChanged,
        this, &PmTogglesWidget::onSwitchingEnabledChanged, Qt::QueuedConnection);

    // local signals -> core slots
    connect(this, &PmTogglesWidget::sigRunningEnabledChanged,
        m_core, &PmCore::onRunningEnabledChanged, Qt::QueuedConnection);
    connect(this, &PmTogglesWidget::sigSwitchingEnabledChanged,
        m_core, &PmCore::onSwitchingEnabledChanged, Qt::QueuedConnection);

    // finish init state
    onRunningEnabledChanged(m_core->runningEnabled());
    onSwitchingEnabledChanged(m_core->switchingEnabled());
}

void PmTogglesWidget::onRunningEnabledChanged(bool enable)
{
    m_runningCheckbox->blockSignals(true);
    m_runningCheckbox->setChecked(enable);
    m_runningCheckbox->blockSignals(false);

    //m_switchingCheckbox->setEnabled(enable);
}

void PmTogglesWidget::onSwitchingEnabledChanged(bool enable)
{
    m_switchingCheckbox->blockSignals(true);
    m_switchingCheckbox->setChecked(enable);
    m_switchingCheckbox->blockSignals(false);
}