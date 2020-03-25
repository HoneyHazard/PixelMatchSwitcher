#include "pixel-match-dialog.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-matching-tab.hpp"
#include "pixel-match-scenes-tab.hpp"
#include "pixel-match-debug-tab.hpp"
#include "pixel-match-filter.h"

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
    PmMatchingTab *matchingTab = new PmMatchingTab(pixelMatcher, this);
    PmScenesTab *scenesTab = new PmScenesTab(pixelMatcher, this);
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
