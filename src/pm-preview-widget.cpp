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

    // core event handlers
    connect(m_core, &PmCore::sigSelectMatchIndex,
            this, &PmPreviewWidget::onSelectMatchIndex, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmPreviewWidget::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmPreviewWidget::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewMatchResults,
            this, &PmPreviewWidget::onNewMatchResults, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigChangedMatchConfig,
            this, &PmPreviewWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigSelectMatchIndex,
            this, &PmPreviewWidget::onSelectMatchIndex, Qt::QueuedConnection);

    // finish init
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));
    onNewMatchResults(selIdx, m_core->matchResults(selIdx));
    onConfigUiChanged();

    std::string matchImgFilename = m_core->matchImgFilename(m_matchIndex);
    const QImage& matchImg = m_core->matchImage(m_matchIndex);
    if (matchImgFilename.size()) {
        if (matchImg.isNull()) {
            onImgFailed(m_matchIndex, matchImgFilename);
        } else {
            onImgSuccess(m_matchIndex, matchImgFilename, matchImg);
        }
    }
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

void PmPreviewWidget::onChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    blockSignals(true);
    m_previewModeButtons->button(int(cfg.previewMode))->setChecked(true);
    m_videoScaleCombo->setCurrentIndex(
        m_videoScaleCombo->findData(cfg.previewVideoScale));
    m_regionScaleCombo->setCurrentIndex(
        m_regionScaleCombo->findData(cfg.previewRegionScale));
    m_matchImgScaleCombo->setCurrentIndex(
        m_matchImgScaleCombo->findData(cfg.previewMatchImageScale));
    blockSignals(false);
}

void PmPreviewWidget::onNewMatchResults(size_t matchIdx, PmMatchResults results)
{
    if (matchIdx != m_matchIndex) return;

    updateFilterDisplaySize(m_core->matchConfig(matchIdx), results);
}

void PmPreviewWidget::onImgSuccess(size_t matchIndex, std::string filename, QImage img)
{
    if (matchIndex != m_matchIndex) return;

    QMutexLocker locker(&m_matchImgLock);
    m_matchImg = img;
}

void PmPreviewWidget::onImgFailed(size_t matchIndex, std::string filename)
{
    if (matchIndex != m_matchIndex) return;

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
    PmMatchConfig config = m_core->matchConfig(m_matchIndex);

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

    updateFilterDisplaySize(config, m_core->matchResults(m_matchIndex));
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

}


void PmPreviewWidget::drawPreview(void* data, uint32_t cx, uint32_t cy)
{
    auto widget = static_cast<PmPreviewWidget*>(data);
    auto core = widget->m_core;

    if (!core) return;

    widget->m_rendering = true;
    auto config = core->matchConfig(widget->m_matchIndex);
    if (config.previewMode == PmPreviewMode::MatchImage) {
        widget->drawMatchImage();
    } else if (core->activeFilterRef().isValid()) {
        widget->drawEffect();
    }
    widget->m_rendering = false;

    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

void PmPreviewWidget::drawEffect()
{
    auto filterRef = m_core->activeFilterRef();
    auto renderSrc = filterRef.filter();

    if (!renderSrc) return;

    float orthoLeft, orthoBottom, orthoRight, orthoTop;
    int vpLeft, vpBottom, vpWidth, vpHeight;

    auto results = m_core->matchResults(m_matchIndex);
    auto config = m_core->matchConfig(m_matchIndex);

    if (config.previewMode == PmPreviewMode::Video) {
        int cx = int(results.baseWidth);
        int cy = int(results.baseHeight);
        int scaledCx = int(cx * config.previewVideoScale);
        int scaledCy = int(cy * config.previewVideoScale);

        orthoLeft = 0.f;
        orthoBottom = 0.f;
        orthoRight = cx;
        orthoTop = cy;

        float scale;
        GetScaleAndCenterPos(cx, cy, scaledCx, scaledCy, vpLeft, vpBottom, scale);
        vpWidth = scaledCx;
        vpHeight = scaledCy;
    } else { // if (config.previewMode == PmPreviewMode::Region) {
        orthoLeft = config.filterCfg.roi_left;
        orthoBottom = config.filterCfg.roi_bottom;
        orthoRight = config.filterCfg.roi_left + int(results.matchImgWidth);
        orthoTop = config.filterCfg.roi_bottom + int(results.matchImgHeight);

        float scale = config.previewRegionScale;
        vpLeft = 0.f;
        vpBottom = 0.0f;
        vpWidth = int(results.matchImgWidth * scale);
        vpHeight = int(results.matchImgHeight * scale);
    }

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100.0f, 100.0f);
    gs_set_viewport(vpLeft, vpBottom, vpWidth, vpHeight);

    auto filterData = filterRef.filterData();
    filterRef.lockData();
    filterData->preview_mode = true;
    filterData->show_border = (config.previewMode == PmPreviewMode::Video);
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
                m_matchImg.width(), m_matchImg.height(),
                GS_BGRA, (uint8_t)-1,
                (const uint8_t**)(&bits), 0);
            m_matchImg = QImage();
        }
    }

    auto config = m_core->matchConfig(m_matchIndex);
    auto filterRef = m_core->activeFilterRef();
    auto filterData = filterRef.filterData();

    float previewScale = config.previewMatchImageScale;

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


void PmPreviewWidget::updateFilterDisplaySize(
    const PmMatchConfig& config, const PmMatchResults& results)
{
    int cx, cy;
    if (config.previewMode == PmPreviewMode::Video) {
        if (!m_core->activeFilterRef().isValid()) {
            cx = 0;
            cy = 0;
        } else {
            float scale = config.previewVideoScale;
            cx = int(results.baseWidth * scale);
            cy = int(results.baseHeight * scale);
        }
    } else if (config.previewMode == PmPreviewMode::Region) {
        if (!m_core->activeFilterRef().isValid()) {
            cx = 0;
            cy = 0;
        } else {
            float scale = config.previewRegionScale;
            cx = int(results.matchImgWidth * scale);
            cy = int(results.matchImgHeight * scale);
        }
    } else { // PmPreviewMode::MatchImage
        float scale = config.previewMatchImageScale;
        cx = int(results.matchImgWidth * scale);
        cy = int(results.matchImgHeight * scale);
    }

    if (m_filterDisplay->width() != cx) {
        m_filterDisplay->setFixedWidth(cx);
    }
    if (m_filterDisplay->height() != cy) {
        m_filterDisplay->setFixedHeight(cy);
    }
}