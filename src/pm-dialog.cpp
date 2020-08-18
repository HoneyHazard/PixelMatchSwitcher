#include "pm-dialog.hpp"
#include "pm-core.hpp"
#include "pm-toggles-widget.hpp"
#include "pm-presets-widget.hpp"
#include "pm-match-list-widget.hpp"
#include "pm-match-config-widget.hpp"
#include "pm-match-results-widget.hpp"
#include "pm-preview-config-widget.hpp"
#include "pm-preview-display-widget.hpp"
#include "pm-match-image-dialog.hpp"
#include "pm-help-widget.hpp"
#include "pm-filter.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QSplitter>

#include <QFileDialog>
#include <QMessageBox>

#include <obs-module.h>
#include <obs-frontend-api.h>

PmDialog::PmDialog(PmCore *core, QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint 
                | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint)
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
    leftLayout->setStretch(0, 1);
    leftLayout->setStretch(1, 1000);
    leftLayout->setStretch(2, 1);
    leftLayout->setStretch(3, 1);
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

    setMinimumSize(200, 200);

    // connections
    connect(m_core, &PmCore::sigCaptureStateChanged,
            this, &PmDialog::onCaptureStateChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigCapturedMatchImage,
            this, &PmDialog::onCapturedMatchImage, Qt::QueuedConnection);

    connect(this, &PmDialog::sigCaptureStateChanged,
            m_core, &PmCore::onCaptureStateChanged, Qt::QueuedConnection);
    connect(this, &PmDialog::sigChangedMatchConfig,
            m_core, &PmCore::onChangedMatchConfig, Qt::QueuedConnection);
}

void PmDialog::onCaptureStateChanged(PmCaptureState state, int x, int y)
{
    if (state == PmCaptureState::Inactive) {
        unsetCursor();
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void PmDialog::onCapturedMatchImage(QImage matchImg, int roiLeft, int roiBottom)
{
    PmMatchImageDialog* mid = new PmMatchImageDialog(matchImg, this);
    mid->exec();

    return;

    QString saveFilename = QFileDialog::getSaveFileName(
        this,
        obs_module_text("Save New Match Image"),
        "",
        PmConstants::k_imageFilenameFilter);
    
    if (saveFilename.size()) {
        bool ok = matchImg.save(saveFilename);
        if (ok) {
            size_t matchIndex = m_core->selectedConfigIndex();
            auto matchCfg = m_core->matchConfig(matchIndex);

            // force image reload in case filenames are same:
            matchCfg.matchImgFilename = ""; 
            emit sigChangedMatchConfig(matchIndex, matchCfg);

            matchCfg.matchImgFilename = saveFilename.toUtf8().data();
            matchCfg.filterCfg.roi_left = roiLeft;
            matchCfg.filterCfg.roi_bottom = roiBottom;
            emit sigChangedMatchConfig(matchIndex, matchCfg);
            emit sigCaptureStateChanged(PmCaptureState::Inactive);
        } else {
            QString errMsg = QString(
                obs_module_text("Unable to save file: %1")).arg(saveFilename);
            QMessageBox::critical(
                this, obs_module_text("Error"), errMsg);
            
            // fallback to SelectFinished state
            int x, y;
            m_core->getCaptureEndXY(x, y);
            emit sigCaptureStateChanged(PmCaptureState::SelectFinished, x, y);
        }
    } else {
        // fallback to SelectFinished state
        int x, y;
        m_core->getCaptureEndXY(x, y);
        emit sigCaptureStateChanged(PmCaptureState::SelectFinished, x, y);
    }
}

void PmDialog::closeEvent(QCloseEvent*)
{
    emit sigCaptureStateChanged(PmCaptureState::Inactive);

    obs_frontend_save();
}
