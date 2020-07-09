#include "pm-dialog.hpp"
#include "pm-core.hpp"
#include "pm-toggles-widget.hpp"
#include "pm-presets-widget.hpp"
#include "pm-match-list-widget.hpp"
#include "pm-match-config-widget.hpp"
#include "pm-match-results-widget.hpp"
#include "pm-preview-widget.hpp"
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
    PmTogglesWidget* togglesWidget = new PmTogglesWidget(core, this);
    PmPresetsWidget* presetsWidget = new PmPresetsWidget(core, this);
    PmMatchListWidget *listWidget = new PmMatchListWidget(core, this);
    PmMatchConfigWidget *configWidget = new PmMatchConfigWidget(core, this);
    PmMatchResultsWidget* resultsWidget = new PmMatchResultsWidget(core, this);
    PmPreviewWidget* previewWidget = new PmPreviewWidget(core, this);

    // left pane
    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(togglesWidget);
    leftLayout->addWidget(presetsWidget);
    leftLayout->addWidget(listWidget);
    leftLayout->addWidget(configWidget);
    leftLayout->addWidget(resultsWidget);
    QWidget* leftWidget = new QWidget(this);
    leftWidget->setLayout(leftLayout);

#if 0
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setStyleSheet(splitterStyle);
    for (int i = 0; i < splitter->count(); ++i) {
        splitter->setCollapsible(i, false);
    }

    // tab widget
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->addTab(splitter, obs_module_text("Main"));
#endif

    // top level splitter layout
    //static const char* splitterStyle
    //    = "QSplitter::handle{background: black; height: 2px}";
    QSplitter* topLevelSplitter = new QSplitter(Qt::Horizontal, this);
    //topLevelSplitter->setStyleSheet(splitterStyle);
    topLevelSplitter->addWidget(leftWidget);
    topLevelSplitter->addWidget(previewWidget);

    QHBoxLayout *topLevelLayout = new QHBoxLayout;
    topLevelLayout->addWidget(topLevelSplitter);
    setLayout(topLevelLayout);
}

void PmDialog::closeEvent(QCloseEvent*)
{
    obs_frontend_save();
}
