#include "pm-dialog.hpp"
#include "pm-core.hpp"
#include "pm-toggles-widget.hpp"
#include "pm-presets-widget.hpp"
#include "pm-match-list-widget.hpp"
#include "pm-match-config-widget.hpp"
#include "pm-match-results-widget.hpp"
#include "pm-preview-config-widget.hpp"
#include "pm-preview-display-widget.hpp"
#include "pm-help-widget.hpp"
#include "pm-filter.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>

#include <obs-module.h>
#include <obs-frontend-api.h>

PmDialog::PmDialog(PmCore *core, QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint 
                | Qt::WindowCloseButtonHint)
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
    PmHelpWidget* helpWidget = new PmHelpWidget(core, this);

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
    topLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    topLayout->addWidget(togglesWidget);
    topLayout->addWidget(previewCfgWidget);
    topLayout->addWidget(helpWidget);
    togglesWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    previewCfgWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    helpWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(topLayout);
    rightLayout->addWidget(previewWidget);
    QWidget* rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);

    // top level splitter layout
    QSplitter* horizSplitter = new QSplitter(Qt::Horizontal, this);
    horizSplitter->addWidget(leftWidget);
    horizSplitter->addWidget(rightWidget);
    horizSplitter->setStretchFactor(1, 1);
    horizSplitter->setStretchFactor(1, 1000);
    horizSplitter->setCollapsible(1, false);

    QVBoxLayout *topLevelLayout = new QVBoxLayout;
    topLevelLayout->addWidget(horizSplitter);
    setLayout(topLevelLayout);

    // connections
    connect(m_core, &PmCore::sigCaptureStateChanged,
            this, &PmDialog::onCaptureStateChanged, Qt::QueuedConnection);
}

void PmDialog::onCaptureStateChanged(PmCaptureState state)
{
    if (state == PmCaptureState::Inactive) {
        unsetCursor();
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void PmDialog::closeEvent(QCloseEvent*)
{
    obs_frontend_save();
}
