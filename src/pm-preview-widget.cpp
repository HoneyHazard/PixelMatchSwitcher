#include "pm-preview-widget.hpp"
#include "pm-core.hpp"

#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QVBoxLayout>

#include <QThread> // sleep

PmPreviewWidget::PmPreviewWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
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
    
    // main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_filterDisplay);
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
    connect(m_core, &PmCore::sigNewActiveFilter,
            this, &PmPreviewWidget::onNewActiveFilter, Qt::QueuedConnection);

    // finish init
    onNewActiveFilter(m_core->activeFilterRef());
    onRunningEnabledChanged(m_core->runningEnabled());
    onPreviewConfigChanged(m_core->previewConfig());
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));
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
}

void PmPreviewWidget::onNewActiveFilter(PmFilterRef ref)
{
    //QMutexLocker locker(&m_filterMutex);
    m_activeFilter = ref;
    setEnabled(m_activeFilter.isValid() && m_core->runningEnabled());
}

void PmPreviewWidget::onRunningEnabledChanged(bool enable)
{
    setEnabled(m_activeFilter.isValid() && enable);
}

void PmPreviewWidget::onImgSuccess(
    size_t matchIndex, std::string filename, QImage img)
{
    if (matchIndex != m_matchIndex) return;

    m_matchImgWidth = img.width();
    m_matchImgHeight = img.height();

    updateFilterDisplaySize();

    QMutexLocker locker(&m_matchImgLock);
    m_matchImg = img;
}

void PmPreviewWidget::onImgFailed(size_t matchIndex)
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

    if (m_core->matchImageLoaded(matchIndex)) {
        std::string matchImgFilename = m_core->matchImgFilename(m_matchIndex);
        QImage matchImg = m_core->matchImage(m_matchIndex);
        onImgSuccess(m_matchIndex, matchImgFilename, matchImg);
    } else {
        onImgFailed(m_matchIndex);
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
