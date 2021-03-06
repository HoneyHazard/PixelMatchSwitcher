#include "pm-toggles-widget.hpp"
#include "pm-core.hpp"

#include <QCheckBox>
#include <QHBoxLayout>

#include <obs-module.h>

PmTogglesWidget::PmTogglesWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Toggle Plugin Activity"), parent)
, m_core(core)
{
    const Qt::ConnectionType qc = Qt::QueuedConnection;

    m_runningCheckbox = new QCheckBox(
        obs_module_text("Enable Matching"), this);
    connect(m_runningCheckbox, &QCheckBox::toggled,
        this, &PmTogglesWidget::sigRunningEnabledChanged, qc);

    m_switchingCheckbox = new QCheckBox(
        obs_module_text("Enable Switching"), this);
    connect(m_switchingCheckbox, &QCheckBox::toggled,
        this, &PmTogglesWidget::sigSwitchingEnabledChanged, qc);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(m_runningCheckbox);
    layout->addWidget(m_switchingCheckbox);
    setLayout(layout);

    // core event handlers
    connect(m_core, &PmCore::sigRunningEnabledChanged,
        this, &PmTogglesWidget::onRunningEnabledChanged, qc);
    connect(m_core, &PmCore::sigSwitchingEnabledChanged,
        this, &PmTogglesWidget::onSwitchingEnabledChanged, qc);

    // local signals -> core slots
    connect(this, &PmTogglesWidget::sigRunningEnabledChanged,
        m_core, &PmCore::onRunningEnabledChanged, qc);
    connect(this, &PmTogglesWidget::sigSwitchingEnabledChanged,
        m_core, &PmCore::onSwitchingEnabledChanged, qc);

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
