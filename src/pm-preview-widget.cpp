#include "pm-preview-widget.hpp"
#include "pm-core.hpp"

#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QFormLayout>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QRadioButton>
#include <QComboBox>

#include <QThread> // sleep

PmPreviewWidget::PmPreviewWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    // main layout
    QFormLayout* mainLayout = new QFormLayout;
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // preview mode
    m_previewModeButtons = new QButtonGroup(this);
    m_previewModeButtons->setExclusive(true);
    connect(m_previewModeButtons, SIGNAL(buttonReleased(int)),
        this, SLOT(onConfigUiChanged()), Qt::QueuedConnection);

    QHBoxLayout* previewModeLayout = new QHBoxLayout;
    previewModeLayout->setContentsMargins(0, 0, 0, 0);

    QRadioButton* videoModeRadio
        = new QRadioButton(obs_module_text("Video"), this);
    m_previewModeButtons->addButton(videoModeRadio, int(PmPreviewMode::Video));
    previewModeLayout->addWidget(videoModeRadio);

    QRadioButton* regionModeRadio
        = new QRadioButton(obs_module_text("Region"), this);
    m_previewModeButtons->addButton(regionModeRadio, int(PmPreviewMode::Region));
    previewModeLayout->addWidget(regionModeRadio);

    QRadioButton* matchImgRadio
        = new QRadioButton(obs_module_text("Match Image"), this);
    m_previewModeButtons->addButton(matchImgRadio, int(PmPreviewMode::MatchImage));
    previewModeLayout->addWidget(matchImgRadio);

    mainLayout->addRow(
        obs_module_text("Preview Mode: "), previewModeLayout);

    // preview scales
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

    mainLayout->addRow(
        obs_module_text("Preview Scale: "), m_previewScaleStack);

    // image/match display area

    m_filterDisplay = new OBSQTDisplay(this);
    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                                      PmPreviewWidget::drawPreview, this);
    };
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);
    connect(m_filterDisplay, &OBSQTDisplay::destroyed,
        this, &PmPreviewWidget::onDestroy, Qt::DirectConnection);
    mainLayout->addRow(m_filterDisplay);

    setLayout(mainLayout);

    // core event handlers
    connect(m_core, &PmCore::sigSelectMatchIndex,
            this, &PmPreviewWidget::onSelectMatchIndex, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigChangedMatchConfig,
        this, &PmPreviewWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmPreviewWidget::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmPreviewWidget::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigPreviewConfigChanged,
            this, &PmPreviewWidget::onPreviewConfigChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSelectMatchIndex,
            this, &PmPreviewWidget::onSelectMatchIndex, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewActiveFilter,
            this, &PmPreviewWidget::onNewActiveFilter, Qt::QueuedConnection);

    // events sent to core
    connect(this, &PmPreviewWidget::sigPreviewConfigChanged,
            m_core, &PmCore::onPreviewConfigChanged, Qt::QueuedConnection);

    // finish init
    onNewActiveFilter(m_core->activeFilterRef());
    onPreviewConfigChanged(m_core->previewConfig());
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));

    onConfigUiChanged();
}

PmPreviewWidget::~PmPreviewWidget()
{
    while (m_rendering) {
        QThread::sleep(1);
    }
    if (m_matchImgTex) {
        gs_texture_destroy(m_matchImgTex);
    }
}

void PmPreviewWidget::closeEvent(QCloseEvent* e)
{
    obs_display_remove_draw_callback(
        m_filterDisplay->GetDisplay(), PmPreviewWidget::drawPreview, this);
    QWidget::closeEvent(e);
}

void PmPreviewWidget::onPreviewConfigChanged(PmPreviewConfig cfg)
{
    m_previewCfg = cfg;
    updateFilterDisplaySize();

    int previewModeIdx = int(cfg.previewMode);

    m_previewScaleStack->blockSignals(true);
    m_previewScaleStack->setCurrentIndex(previewModeIdx);
    m_previewScaleStack->blockSignals(false);

    m_previewModeButtons->blockSignals(true);
    m_previewModeButtons->button(previewModeIdx)->setChecked(true);
    m_previewModeButtons->blockSignals(false);

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
}

void PmPreviewWidget::onNewActiveFilter(PmFilterRef ref)
{
    //QMutexLocker locker(&m_filterMutex);
    m_activeFilter = ref;
}

void PmPreviewWidget::onImgSuccess(
    size_t matchIndex, std::string filename, QImage img)
{
    if (matchIndex != m_matchIndex) return;

    m_matchImgWidth = img.width();
    m_matchImgHeight = img.height();

    updateFilterDisplaySize();
    enableRegionViews(true);

    QMutexLocker locker(&m_matchImgLock);
    m_matchImg = img;
}

void PmPreviewWidget::onImgFailed(size_t matchIndex, std::string filename)
{
    if (matchIndex != m_matchIndex) return;

    enableRegionViews(false);

    QMutexLocker locker(&m_matchImgLock);
    if (m_matchImgTex) {
        obs_enter_graphics();
        gs_texture_destroy(m_matchImgTex);
        obs_leave_graphics();
        m_matchImgTex = nullptr;
    }
}

void PmPreviewWidget::onConfigUiChanged()
{
    PmPreviewConfig config = m_core->previewConfig();

    int previewModeIdx = m_previewModeButtons->checkedId();
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

    emit sigPreviewConfigChanged(config);

    //updateFilterDisplaySize(
    //    config, m_core->matchResults(m_matchIndex));

}

void PmPreviewWidget::onDestroy(QObject* obj)
{
    while (m_rendering) {
        QThread::sleep(1);
    }
    UNUSED_PARAMETER(obj);
}

void PmPreviewWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;
    onChangedMatchConfig(matchIndex, cfg);

    if (m_matchImgTex) {
        obs_enter_graphics();
        gs_texture_destroy(m_matchImgTex);
        obs_leave_graphics();
        m_matchImgTex = nullptr;
    }

    std::string matchImgFilename = m_core->matchImgFilename(m_matchIndex);
    QImage matchImg = m_core->matchImage(m_matchIndex);
    if (matchImg.isNull()) {
        onImgFailed(m_matchIndex, matchImgFilename);
    } else {
        onImgSuccess(m_matchIndex, matchImgFilename, matchImg);
    }
}

void PmPreviewWidget::onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg)
{
    if (matchIndex != m_matchIndex) return;

    m_roiLeft = cfg.filterCfg.roi_left;
    m_roiBottom = cfg.filterCfg.roi_bottom;

    updateFilterDisplaySize();
}

void PmPreviewWidget::drawPreview(void* data, uint32_t cx, uint32_t cy)
{
    auto widget = static_cast<PmPreviewWidget*>(data);
    auto core = widget->m_core;

    if (!core) return;

    widget->m_rendering = true;
    if (widget->m_previewCfg.previewMode == PmPreviewMode::MatchImage) {
        widget->drawMatchImage();
    } else {
        widget->drawEffect();
    }
    widget->m_rendering = false;

    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

void PmPreviewWidget::drawEffect()
{
    PmFilterRef &filterRef = m_activeFilter;
    auto renderSrc = filterRef.filter();
    auto filterData = filterRef.filterData();

    if (!renderSrc || !filterRef.isValid()) return;

    float orthoLeft, orthoBottom, orthoRight, orthoTop;
    int vpLeft, vpBottom, vpWidth, vpHeight;

    //auto results = m_core->matchResults(m_matchIndex);
    //auto config = m_core->matchConfig(m_matchIndex);

    if (m_previewCfg.previewMode == PmPreviewMode::Video) {
        filterRef.lockData();
        int cx = int(filterData->base_width);
        int cy = int(filterData->base_height);
        filterRef.unlockData();
        int scaledCx = int(cx * m_previewCfg.previewVideoScale);
        int scaledCy = int(cy * m_previewCfg.previewVideoScale);

        orthoLeft = 0.f;
        orthoBottom = 0.f;
        orthoRight = cx;
        orthoTop = cy;

        float scale;
        GetScaleAndCenterPos(cx, cy, scaledCx, scaledCy, vpLeft, vpBottom, scale);
        vpWidth = scaledCx;
        vpHeight = scaledCy;
    } else { // if (config.previewMode == PmPreviewMode::Region) {
        orthoLeft = m_roiLeft;
        orthoBottom = m_roiBottom;
        orthoRight = m_roiLeft + m_matchImgWidth;
        orthoTop = m_roiBottom + m_matchImgHeight;

        float scale = m_previewCfg.previewRegionScale;
        vpLeft = 0.f;
        vpBottom = 0.0f;
        vpWidth = int(m_matchImgWidth * scale);
        vpHeight = int(m_matchImgHeight * scale);
    }

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100.0f, 100.0f);
    gs_set_viewport(vpLeft, vpBottom, vpWidth, vpHeight);

    filterRef.lockData();
    filterData->preview_mode = true;
    filterData->show_border = (m_previewCfg.previewMode == PmPreviewMode::Video);
    filterRef.unlockData();

    obs_source_video_render(renderSrc);

    gs_projection_pop();
    gs_viewport_pop();
}

void PmPreviewWidget::drawMatchImage()
{
    {
        QMutexLocker locker(&m_matchImgLock);
        if (!m_matchImg.isNull()) {
            if (m_matchImgTex) {
                gs_texture_destroy(m_matchImgTex);
            }
            unsigned char* bits = m_matchImg.bits();
            m_matchImgTex = gs_texture_create(
                m_matchImgWidth, m_matchImgHeight,
                GS_BGRA, (uint8_t)-1,
                (const uint8_t**)(&bits), 0);
            m_matchImg = QImage();
        }
    }

    float previewScale = m_previewCfg.previewMatchImageScale;

    gs_effect* effect = m_core->drawMatchImageEffect();
    gs_eparam_t* param = gs_effect_get_param_by_name(effect, "image");
    gs_effect_set_texture(param, m_matchImgTex);

    while (gs_effect_loop(effect, "Draw")) {
        gs_matrix_push();
        gs_matrix_scale3f(previewScale, previewScale, previewScale);
        gs_draw_sprite(m_matchImgTex, 0, 0, 0);
        gs_matrix_pop();
    }
}


void PmPreviewWidget::updateFilterDisplaySize()
    //const PmMatchConfig& config, const PmMatchResults& results)
{
    auto filterRef = m_core->activeFilterRef();
    auto filterData = filterRef.filterData();

    int cx, cy;
    if (m_previewCfg.previewMode == PmPreviewMode::Video) {
        if (filterData) {
            float scale = m_previewCfg.previewVideoScale;
            cx = int(filterData->base_width * scale);
            cy = int(filterData->base_height * scale);
        } else {
            cx = 0;
            cy = 0;
        }
    } else if (m_previewCfg.previewMode == PmPreviewMode::Region) {
        if (filterData) {
            float scale = m_previewCfg.previewRegionScale;
            cx = int(m_matchImgWidth * scale);
            cy = int(m_matchImgHeight * scale);
        } else {
            cx = 0;
            cy = 0;
        }
    } else { // PmPreviewMode::MatchImage
        float scale = m_previewCfg.previewMatchImageScale;
        cx = int(m_matchImgWidth * scale);
        cy = int(m_matchImgHeight * scale);
    }

    if (m_filterDisplay->width() != cx) {
        m_filterDisplay->setFixedWidth(cx);
    }
    if (m_filterDisplay->height() != cy) {
        m_filterDisplay->setFixedHeight(cy);
    }
}

void PmPreviewWidget::enableRegionViews(bool enable)
{
    auto regButton = m_previewModeButtons->button((int)PmPreviewMode::Region);
    regButton->setVisible(enable);
    regButton->setEnabled(enable);
    auto imgButton = m_previewModeButtons->button((int)PmPreviewMode::MatchImage);
    imgButton->setVisible(enable);
    imgButton->setEnabled(enable);

    m_regionScaleCombo->setEnabled(enable);
    m_matchImgScaleCombo->setEnabled(enable);

    if (!enable && m_previewCfg.previewMode != PmPreviewMode::Video) {
        auto cfg = m_core->previewConfig();
        cfg.previewMode = PmPreviewMode::Video;
        emit sigPreviewConfigChanged(cfg);
    }
}
