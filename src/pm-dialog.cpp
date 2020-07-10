#include "pm-dialog.hpp"
#include "pm-core.hpp"
#include "pm-toggles-widget.hpp"
#include "pm-presets-widget.hpp"
#include "pm-match-list-widget.hpp"
#include "pm-match-config-widget.hpp"
#include "pm-match-results-widget.hpp"
#include "pm-preview-config-widget.hpp"
#include "pm-preview-display-widget.hpp"
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
    PmPreviewConfigWidget* previewCfgWidget = new PmPreviewConfigWidget(core, this);
    PmPreviewDisplayWidget* previewWidget = new PmPreviewDisplayWidget(core, this);

    // left pane
    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(presetsWidget);
    leftLayout->addWidget(listWidget);
    leftLayout->addWidget(configWidget);
    leftLayout->addWidget(resultsWidget);
    QWidget* leftWidget = new QWidget(this);
    leftWidget->setLayout(leftLayout);

    // top layout
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setAlignment(Qt::AlignLeft);
    topLayout->addWidget(togglesWidget);
    topLayout->addWidget(previewCfgWidget);
    togglesWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    previewCfgWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(topLayout);
    rightLayout->addWidget(previewWidget);
    QWidget* rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    // top level splitter layout
    QSplitter* horizSplitter = new QSplitter(Qt::Horizontal, this);
    horizSplitter->addWidget(leftWidget);
    horizSplitter->addWidget(rightWidget);
    horizSplitter->setCollapsible(1, false);

    //static const char* splitterStyle
    //    = "QSplitter::handle{background: black; height: 2px}";
    //topLevelSplitter->setStyleSheet(splitterStyle);

    QVBoxLayout *topLevelLayout = new QVBoxLayout;
    //topLevelLayout->addLayout(topLayout);
    topLevelLayout->addWidget(horizSplitter);
    setLayout(topLevelLayout);
}

void PmDialog::closeEvent(QCloseEvent*)
{
    obs_frontend_save();
}
