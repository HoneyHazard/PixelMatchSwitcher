#include "pm-toggles-widget.hpp"
#include "pm-debug-tab.hpp"
#include "pm-core.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>

#include <obs-module.h>
#include <obs-frontend-api.h>

PmTogglesWidget::PmTogglesWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Toggle Plugin Activity"), parent)
, m_core(core)
{
    m_runningCheckbox = new QCheckBox(
        obs_module_text("Enable Matching"), this);
    connect(m_runningCheckbox, &QCheckBox::toggled,
        this, &PmTogglesWidget::sigRunningEnabledChanged, Qt::QueuedConnection);

    m_switchingCheckbox = new QCheckBox(
        obs_module_text("Enable Switching"), this);
    connect(m_switchingCheckbox, &QCheckBox::toggled,
        this, &PmTogglesWidget::sigSwitchingEnabledChanged, Qt::QueuedConnection);

    QPushButton* showDebugButton = new QPushButton(
        obs_module_text("Debug"), this);
    showDebugButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    connect(showDebugButton, &QPushButton::released,
        this, &PmTogglesWidget::onShowDebug, Qt::QueuedConnection);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_runningCheckbox);
    mainLayout->addWidget(m_switchingCheckbox);
    mainLayout->addWidget(showDebugButton);
    setLayout(mainLayout);

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

void PmTogglesWidget::onShowDebug()
{
    auto mainWindow
        = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    PmDebugTab* debugTab = new PmDebugTab(m_core, mainWindow);
    setAttribute(Qt::WA_DeleteOnClose, true);
    debugTab->show();
}
