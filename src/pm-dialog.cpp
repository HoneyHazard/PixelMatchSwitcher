#include "pm-dialog.hpp"
#include "pm-core.hpp"
#include "pm-match-tab.hpp"
#include "pm-switch-tab.hpp"
#include "pm-debug-tab.hpp"
#include "pm-filter.h"

#include <QHBoxLayout>
#include <QTabWidget>

#include <obs-module.h>

PmDialog::PmDialog(PmCore *pixelMatcher, QWidget *parent)
: QDialog(parent)
, m_core(pixelMatcher)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));
    setAttribute(Qt::WA_DeleteOnClose, true);

    // tabs
    PmMatchTab *matchTab = new PmMatchTab(pixelMatcher, this);
    PmSwitchTab *switchTab = new PmSwitchTab(pixelMatcher, this);
    PmDebugTab *debugTab = new PmDebugTab(pixelMatcher, this);

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(matchTab, obs_module_text("Matching"));
    tabWidget->addTab(switchTab, obs_module_text("Switching"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    // top level layout
    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    setLayout(topLevelLayout);
}
