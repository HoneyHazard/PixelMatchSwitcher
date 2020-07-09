#include "pm-dialog.hpp"
#include "pm-core.hpp"
#include "pm-toggles-widget.hpp"
#include "pm-presets-widget.hpp"
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
    static const char* splitterStyle
        = "QSplitter::handle{background: black; height: 2px}";

    setWindowTitle(obs_module_text("Pixel Match Switcher"));
    setAttribute(Qt::WA_DeleteOnClose, true);

    // UI modules
    
    PmTogglesWidget* togglesWidget = new PmTogglesWidget(core, this);
    PmPresetsWidget* presetsWidget = new PmPresetsWidget(core, this);
    PmMatchListWidget *matchListWidget = new PmMatchListWidget(core, this);
    PmMatchConfigWidget *matchConfigWidget  
        = new PmMatchConfigWidget(core, this);
    PmPreviewWidget* previewWidget = new PmPreviewWidget(core, this);
    PmDebugTab* debugTab = new PmDebugTab(core, this);

    // main tab splitter
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setStyleSheet(splitterStyle);
    splitter->addWidget(togglesWidget);
    splitter->addWidget(presetsWidget);
    splitter->addWidget(matchListWidget);
    splitter->addWidget(matchConfigWidget);
    for (size_t i = 0; i < splitter->count(); ++i) {
        splitter->setCollapsible(i, false);
    }

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(splitter, obs_module_text("Main"));
    tabWidget->addTab(debugTab, obs_module_text("Debug"));

    // top level splitter layout
    QSplitter* topLevelSplitter = new QSplitter(Qt::Horizontal, this);
    topLevelSplitter->setStyleSheet(splitterStyle);
    topLevelSplitter->addWidget(tabWidget);
    topLevelSplitter->addWidget(previewWidget);

    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(topLevelSplitter);
    setLayout(topLevelLayout);


}

void PmDialog::closeEvent(QCloseEvent*)
{
    obs_frontend_save();
}
