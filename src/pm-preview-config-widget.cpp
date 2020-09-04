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
    // main layout
    QHBoxLayout* mainLayout = new QHBoxLayout;
    setLayout(mainLayout);

    // preview mode
    //QLabel* modeLabel = new QLabel(obs_module_text("Mode: "), this);
    //mainLayout->addWidget(modeLabel);

    m_previewModeButtons = new QButtonGroup(this);
    m_previewModeButtons->setExclusive(true);
    connect(m_previewModeButtons, SIGNAL(buttonReleased(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    QRadioButton* videoModeRadio
        = new QRadioButton(obs_module_text("Video"), this);
    m_previewModeButtons->addButton(videoModeRadio, int(PmPreviewMode::Video));
    mainLayout->addWidget(videoModeRadio);

    QRadioButton* regionModeRadio
        = new QRadioButton(obs_module_text("Region"), this);
    m_previewModeButtons->addButton(regionModeRadio, int(PmPreviewMode::Region));
    mainLayout->addWidget(regionModeRadio);

    QRadioButton* matchImgRadio
        = new QRadioButton(obs_module_text("Match Image"), this);
    m_previewModeButtons->addButton(matchImgRadio, int(PmPreviewMode::MatchImage));
    mainLayout->addWidget(matchImgRadio);

#if 0
    // some space inbetween
    QSpacerItem* spacerItem = new QSpacerItem(50, 0);
    mainLayout->addItem(spacerItem);

    // preview scales
    QLabel* scaleLabel = new QLabel(obs_module_text("Scale: "), this);
    mainLayout->addWidget(scaleLabel);

    m_previewScaleStack = new QStackedWidget(this);
    m_previewScaleStack->setSizePolicy(
        QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_videoScaleCombo = new QComboBox(this);
    m_videoScaleCombo->addItem("100%", 1.f);
    m_videoScaleCombo->addItem("75%", 0.75f);
    m_videoScaleCombo->addItem("50%", 0.5f);
    m_videoScaleCombo->addItem("25%", 0.25f);
    m_videoScaleCombo->setCurrentIndex(0);
    connect(m_videoScaleCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::Video), m_videoScaleCombo);

    m_regionScaleCombo = new QComboBox(this);
    m_regionScaleCombo->addItem("50%", 0.5f);
    m_regionScaleCombo->addItem("100%", 1.f);
    m_regionScaleCombo->addItem("150%", 1.5f);
    m_regionScaleCombo->addItem("200%", 2.f);
    m_regionScaleCombo->addItem("500%", 5.f);
    m_regionScaleCombo->addItem("1000%", 10.f);
    m_regionScaleCombo->setCurrentIndex(0);
    connect(m_regionScaleCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::Region), m_regionScaleCombo);

    m_matchImgScaleCombo = new QComboBox(this);
    m_matchImgScaleCombo->addItem("50%", 0.5f);
    m_matchImgScaleCombo->addItem("100%", 1.f);
    m_matchImgScaleCombo->addItem("150%", 1.5f);
    m_matchImgScaleCombo->addItem("200%", 2.f);
    m_matchImgScaleCombo->addItem("500%", 5.f);
    m_matchImgScaleCombo->addItem("1000%", 10.f);
    m_matchImgScaleCombo->setCurrentIndex(0);
    connect(m_matchImgScaleCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);
    m_previewScaleStack->insertWidget(
        int(PmPreviewMode::MatchImage), m_matchImgScaleCombo);

    mainLayout->addWidget(m_previewScaleStack);
#endif

    // core event handlers
    connect(m_core, &PmCore::sigSelectMatchIndex,
        this, &PmPreviewConfigWidget::onSelectMatchIndex, Qt::QueuedConnection);
    //connect(m_core, &PmCore::sigChangedMatchConfig,
    //    this, &PmPreviewConfigWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgSuccess,
        this, &PmPreviewConfigWidget::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
        this, &PmPreviewConfigWidget::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigPreviewConfigChanged,
        this, &PmPreviewConfigWidget::onPreviewConfigChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewActiveFilter,
        this, &PmPreviewConfigWidget::onNewActiveFilter, Qt::QueuedConnection);

    // events sent to core
    connect(this, &PmPreviewConfigWidget::sigPreviewConfigChanged,
        m_core, &PmCore::onPreviewConfigChanged, Qt::QueuedConnection);

    // finish init
    onNewActiveFilter(m_core->activeFilterRef());
    onRunningEnabledChanged(m_core->runningEnabled());
    onPreviewConfigChanged(m_core->previewConfig());
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));

    onConfigUiChanged();
}

void PmPreviewConfigWidget::onPreviewConfigChanged(PmPreviewConfig cfg)
{
    //m_previewCfg = cfg;

    int previewModeIdx = int(cfg.previewMode);

    m_previewModeButtons->blockSignals(true);
    m_previewModeButtons->button(previewModeIdx)->setChecked(true);
    m_previewModeButtons->blockSignals(false);

#if 0
    m_previewScaleStack->blockSignals(true);
    m_previewScaleStack->setCurrentIndex(previewModeIdx);
    m_previewScaleStack->blockSignals(false);

    m_videoScaleCombo->blockSignals(true);
    m_videoScaleCombo->setCurrentIndex(
        m_videoScaleCombo->findData(cfg.previewVideoScale));
    m_videoScaleCombo->blockSignals(false);

    m_regionScaleCombo->blockSignals(true);
    m_regionScaleCombo->setCurrentIndex(
        m_regionScaleCombo->findData(cfg.previewRegionScale));
    m_regionScaleCombo->blockSignals(false);

    m_matchImgScaleCombo->blockSignals(true);
    m_matchImgScaleCombo->setCurrentIndex(
        m_matchImgScaleCombo->findData(cfg.previewMatchImageScale));
    m_matchImgScaleCombo->blockSignals(false);
#endif
}

void PmPreviewConfigWidget::onNewActiveFilter(PmFilterRef ref)
{
    //QMutexLocker locker(&m_filterMutex);
    //m_activeFilter = ref;
    //setEnabled(ref.isValid() && m_core->runningEnabled());
}

void PmPreviewConfigWidget::onRunningEnabledChanged(bool enable)
{
    //setEnabled(enable && m_core->activeFilterRef().isValid());
}

void PmPreviewConfigWidget::onImgSuccess(size_t matchIndex)
{
    if (matchIndex != m_matchIndex) return;

    enableRegionViews(true);
}

void PmPreviewConfigWidget::onImgFailed(size_t matchIndex)
{
    if (matchIndex != m_matchIndex) return;

    enableRegionViews(false);
}

void PmPreviewConfigWidget::onConfigUiChanged()
{
    PmPreviewConfig config = m_core->previewConfig();

    int previewModeIdx = m_previewModeButtons->checkedId();
    config.previewMode = PmPreviewMode(previewModeIdx);

#if 0
    m_previewScaleStack->setCurrentIndex(previewModeIdx);
    auto scaleCombo = m_previewScaleStack->widget(previewModeIdx);
    if (scaleCombo) {
        m_previewScaleStack->setFixedSize(scaleCombo->sizeHint());
    }

    config.previewMode = PmPreviewMode(previewModeIdx);
    config.previewVideoScale
        = m_videoScaleCombo->currentData().toFloat();
    config.previewRegionScale
        = m_regionScaleCombo->currentData().toFloat();
    config.previewMatchImageScale
        = m_matchImgScaleCombo->currentData().toFloat();
#endif

    emit sigPreviewConfigChanged(config);
}

void PmPreviewConfigWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;
    //onChangedMatchConfig(matchIndex, cfg);
    if (m_core->matchImageLoaded(matchIndex)) {
        onImgSuccess(m_matchIndex);
    } else {
        onImgFailed(m_matchIndex);
    }
}

void PmPreviewConfigWidget::enableRegionViews(bool enable)
{
    auto regButton = m_previewModeButtons->button((int)PmPreviewMode::Region);
    regButton->setVisible(enable);
    regButton->setEnabled(enable);
    auto imgButton = m_previewModeButtons->button((int)PmPreviewMode::MatchImage);
    imgButton->setVisible(enable);
    imgButton->setEnabled(enable);

#if 0
    m_regionScaleCombo->setEnabled(enable);
    m_matchImgScaleCombo->setEnabled(enable);
#endif
}