#include "pm-help-widget.hpp"

#include "pm-debug-tab.hpp"
#include "pm-about-box.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QPushButton>
#include <QHBoxLayout>
#include <QMainWindow>

PmHelpWidget::PmHelpWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Help"), parent)
, m_core(core)
{
    QPushButton* showDebugButton = new QPushButton(
        obs_module_text("Debug"), this);
    showDebugButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    connect(showDebugButton, &QPushButton::released,
        this, &PmHelpWidget::onShowDebug, Qt::QueuedConnection);

    QPushButton* aboutButton = new QPushButton(
        obs_module_text("About"), this);
    aboutButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    connect(aboutButton, &QPushButton::released,
        this, &PmHelpWidget::onShowAbout, Qt::QueuedConnection);

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(aboutButton);
    layout->addWidget(showDebugButton);
    setLayout(layout);
}

void PmHelpWidget::onShowDebug()
{
    auto mainWindow
        = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    PmDebugTab* debugTab = new PmDebugTab(m_core, mainWindow);
    setAttribute(Qt::WA_DeleteOnClose, true);
    debugTab->show();
}

void PmHelpWidget::onShowAbout()
{
    PmAboutBox* box = new PmAboutBox(this);
}
