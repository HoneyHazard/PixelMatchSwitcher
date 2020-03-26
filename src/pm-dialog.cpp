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
    PmMatchTab *matchingTab = new PmMatchTab(pixelMatcher, this);
    PmSwitchTab *scenesTab = new PmSwitchTab(pixelMatcher, this);
    PmDebugTab *debugTab = new PmDebugTab(pixelMatcher, this);

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(matchingTab, obs_module_text("Matching"));
    tabWidget->addTab(scenesTab, obs_module_text("Scenes"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    // top level layout
    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    setLayout(topLevelLayout);
}
