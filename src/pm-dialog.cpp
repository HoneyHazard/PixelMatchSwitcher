#include "pm-dialog.hpp"
#include "pm-core.hpp"
//#include "pm-match-tab.hpp"
//#include "pm-switch-tab.hpp"
#include "pm-match-list-widget.hpp"
#include "pm-match-config-widget.hpp"
#include "pm-preview-widget.hpp"
#include "pm-debug-tab.hpp"
#include "pm-filter.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>

#include <obs-module.h>
#include <obs-frontend-api.h>

PmDialog::PmDialog(PmCore *core, QWidget *parent)
: QDialog(parent)
, m_core(core)
{
    setWindowTitle(obs_module_text("Pixel Match Switcher"));
    setAttribute(Qt::WA_DeleteOnClose, true);

    // UI modules
    PmMatchListWidget* matchListWidget = new PmMatchListWidget(core, this);
    PmMatchConfigWidget *matchConfigWidget  
        = new PmMatchConfigWidget(core, this);
    PmPreviewWidget* previewWidget = new PmPreviewWidget(core, this);
    PmDebugTab* debugTab = new PmDebugTab(core, this);

    // main tab (splitter)
    QSplitter* mainTab = new QSplitter(Qt::Vertical, this);
    mainTab->addWidget(matchListWidget);
    mainTab->addWidget(matchConfigWidget);

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(mainTab, obs_module_text("Main"));
    //tabWidget->addTab(switchTab, obs_module_text("Switching"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    // top level layout
    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(tabWidget);
    topLevelLayout->addWidget(previewWidget);
    setLayout(topLevelLayout);
}

void PmDialog::closeEvent(QCloseEvent*)
{
    obs_frontend_save();
}
