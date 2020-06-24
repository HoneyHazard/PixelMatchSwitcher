#include "pm-dialog.hpp"
#include "pm-core.hpp"
//#include "pm-match-tab.hpp"
//#include "pm-switch-tab.hpp"
#include "pm-match-list-widget.hpp"
#include "pm-match-config-widget.hpp"
#include "pm-debug-tab.hpp"
#include "pm-filter.h"

#include <QVBoxLayout>
#include <QTabWidget>

#include <obs-module.h>
#include <obs-frontend-api.h>

PmDialog::PmDialog(PmCore *core, QWidget *parent)
: QDialog(parent)
, m_core(core)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));
    setAttribute(Qt::WA_DeleteOnClose, true);

    // widgets
    PmMatchListWidget* matchListWidget 
        = new PmMatchListWidget(core, this);
    PmMatchConfigWidget *matchConfigWidget 
        = new PmMatchConfigWidget(core, this);

    // main widget
    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(matchListWidget);
    mainLayout->addWidget(matchConfigWidget);
    mainWidget->setLayout(mainLayout);

    // tab widget
    //PmSwitchTab *switchTab = new PmSwitchTab(pixelMatcher, this);
    PmDebugTab* debugTab = new PmDebugTab(core, this);

    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(mainWidget, obs_module_text("Main"));
    //tabWidget->addTab(switchTab, obs_module_text("Switching"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    // top level layout
    QVBoxLayout *topLevelLayout = new QVBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    setLayout(topLevelLayout);
}

void PmDialog::closeEvent(QCloseEvent*)
{
    obs_frontend_save();
}
