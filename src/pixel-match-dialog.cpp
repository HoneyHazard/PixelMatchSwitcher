#include "pixel-match-dialog.hpp"
#include "pixel-match-core.hpp"
#include "pixel-match-matching-tab.hpp"
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
    PmMatchingTab *mainTab = new PmMatchingTab(pixelMatcher, this);
    PmDebugTab *debugTab = new PmDebugTab(pixelMatcher, this);

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(mainTab, obs_module_text("Main"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    setLayout(topLevelLayout);
}
