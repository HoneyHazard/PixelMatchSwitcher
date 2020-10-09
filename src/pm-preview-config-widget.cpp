#include "pm-preview-config-widget.hpp"
#include "pm-core.hpp"

#include <obs-module.h>

#include <QHBoxLayout>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QRadioButton>
#include <QComboBox>
#include <QLabel>

PmPreviewConfigWidget::PmPreviewConfigWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Preview Mode"), parent)
, m_core(core)
{
    const Qt::ConnectionType qc = Qt::QueuedConnection;

    // main layout
    QHBoxLayout* mainLayout = new QHBoxLayout;
    setLayout(mainLayout);

    m_previewModeButtons = new QButtonGroup(this);
    m_previewModeButtons->setExclusive(true);
    connect(m_previewModeButtons, SIGNAL(buttonReleased(int)),
        this, SLOT(onPreviewModeRadioChanged()), qc);

    QRadioButton* videoModeRadio
        = new QRadioButton(obs_module_text("Video"), this);
    m_previewModeButtons->addButton(
        videoModeRadio, int(PmPreviewMode::Video));
    mainLayout->addWidget(videoModeRadio);

    QRadioButton* regionModeRadio
        = new QRadioButton(obs_module_text("Region"), this);
    m_previewModeButtons->addButton(
        regionModeRadio, int(PmPreviewMode::Region));
    mainLayout->addWidget(regionModeRadio);

    QRadioButton* matchImgRadio
        = new QRadioButton(obs_module_text("Match Image"), this);
    m_previewModeButtons->addButton(
        matchImgRadio, int(PmPreviewMode::MatchImage));
    mainLayout->addWidget(matchImgRadio);

    // core event handlers
    connect(m_core, &PmCore::sigMatchConfigSelect,
           this, &PmPreviewConfigWidget::onMatchConfigSelect, qc);
    connect(m_core, &PmCore::sigMatchImageLoadSuccess,
            this, &PmPreviewConfigWidget::onMatchImageLoadSuccess, qc);
    connect(m_core, &PmCore::sigMatchImageLoadFailed,
            this, &PmPreviewConfigWidget::onMatchImageLoadFailed, qc);
    connect(m_core, &PmCore::sigPreviewConfigChanged,
            this, &PmPreviewConfigWidget::onPreviewConfigChanged, qc);
    connect(m_core, &PmCore::sigRunningEnabledChanged,
            this, &PmPreviewConfigWidget::onRunningEnabledChanged, qc);

    // events sent to core
    connect(this, &PmPreviewConfigWidget::sigPreviewConfigChanged,
            m_core, &PmCore::onPreviewConfigChanged, qc);

    // finish init
    onRunningEnabledChanged(m_core->runningEnabled());
    size_t selIdx = m_core->selectedConfigIndex();
    onMatchConfigSelect(selIdx, m_core->matchConfig(selIdx));
    onPreviewConfigChanged(m_core->previewConfig());
}

void PmPreviewConfigWidget::onPreviewConfigChanged(PmPreviewConfig cfg)
{
    int previewModeIdx = int(cfg.previewMode);

    m_previewModeButtons->blockSignals(true);
    m_previewModeButtons->button(previewModeIdx)->setChecked(true);
    m_previewModeButtons->blockSignals(false);
}

void PmPreviewConfigWidget::onRunningEnabledChanged(bool enable)
{
    enableRegionViews(enable && m_core->matchImageLoaded(m_matchIndex));
}

void PmPreviewConfigWidget::onMatchImageLoadSuccess(size_t matchIndex)
{
    if (matchIndex != m_matchIndex) return;

    enableRegionViews(m_core->runningEnabled());
}

void PmPreviewConfigWidget::onMatchImageLoadFailed(size_t matchIndex)
{
    if (matchIndex != m_matchIndex) return;

    enableRegionViews(false);
}

void PmPreviewConfigWidget::onPreviewModeRadioChanged()
{
    PmPreviewConfig config = m_core->previewConfig();

    int previewModeIdx = m_previewModeButtons->checkedId();
    config.previewMode = PmPreviewMode(previewModeIdx);

    emit sigPreviewConfigChanged(config);
}

void PmPreviewConfigWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;
    if (m_core->matchImageLoaded(matchIndex)) {
        onMatchImageLoadSuccess(m_matchIndex);
    } else {
        onMatchImageLoadFailed(m_matchIndex);
    }

    UNUSED_PARAMETER(cfg);
}

void PmPreviewConfigWidget::enableRegionViews(bool enable)
{
    auto regButton = m_previewModeButtons->button((int)PmPreviewMode::Region);
    regButton->setVisible(enable);
    regButton->setEnabled(enable);
    auto imgButton 
        = m_previewModeButtons->button((int)PmPreviewMode::MatchImage);
    imgButton->setVisible(enable);
    imgButton->setEnabled(enable);
}
