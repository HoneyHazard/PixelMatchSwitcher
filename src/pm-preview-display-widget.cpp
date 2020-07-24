#include "pm-preview-display-widget.hpp"
#include "pm-core.hpp"

#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QVBoxLayout>

#include <QThread> // sleep
#include <QMouseEvent>

PmPreviewDisplayWidget::PmPreviewDisplayWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    // image/match display area
    m_filterDisplay = new OBSQTDisplay(this);
    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                                      PmPreviewDisplayWidget::drawPreview, this);
    };
    m_filterDisplay->installEventFilter(this);
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);
    connect(m_filterDisplay, &OBSQTDisplay::destroyed,
        this, &PmPreviewDisplayWidget::onDestroy, Qt::DirectConnection);
   
    // main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_filterDisplay);
    mainLayout->setStretchFactor(m_filterDisplay, 1000);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    // core event handlers
    connect(m_core, &PmCore::sigSelectMatchIndex,
            this, &PmPreviewDisplayWidget::onSelectMatchIndex, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigChangedMatchConfig,
        this, &PmPreviewDisplayWidget::onChangedMatchConfig, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgSuccess,
            this, &PmPreviewDisplayWidget::onImgSuccess, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigImgFailed,
            this, &PmPreviewDisplayWidget::onImgFailed, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigPreviewConfigChanged,
            this, &PmPreviewDisplayWidget::onPreviewConfigChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigNewActiveFilter,
            this, &PmPreviewDisplayWidget::onNewActiveFilter, Qt::QueuedConnection);

    // signals sent to the core
    connect(this, &PmPreviewDisplayWidget::sigCaptureStateChanged,
            m_core, &PmCore::onCaptureStateChanged, Qt::QueuedConnection);

    // finish init
    onNewActiveFilter(m_core->activeFilterRef());
    onRunningEnabledChanged(m_core->runningEnabled());
    onPreviewConfigChanged(m_core->previewConfig());
    size_t selIdx = m_core->selectedConfigIndex();
    onSelectMatchIndex(selIdx, m_core->matchConfig(selIdx));
}

PmPreviewDisplayWidget::~PmPreviewDisplayWidget()
{
    while (m_rendering) {
        QThread::sleep(1);
    }
    if (m_matchImgTex) {
        gs_texture_destroy(m_matchImgTex);
    }
}

void PmPreviewDisplayWidget::resizeEvent(QResizeEvent* e)
{
    fixGeometry();
    QWidget::resizeEvent(e);
}

void PmPreviewDisplayWidget::closeEvent(QCloseEvent* e)
{
    obs_display_remove_draw_callback(
        m_filterDisplay->GetDisplay(), PmPreviewDisplayWidget::drawPreview, this);
    QWidget::closeEvent(e);
}

bool PmPreviewDisplayWidget::eventFilter(QObject* obj, QEvent* event)
{
    auto captureState = m_core->captureState();

    if (captureState == PmCaptureState::Activated
      && event->type() == QEvent::MouseButtonPress) {
        int imgX, imgY;
        getImageXY((QMouseEvent*)event, imgX, imgY);
        emit sigCaptureStateChanged(PmCaptureState::SelectBegin, imgX, imgY);
        return true;
    } else if ((captureState == PmCaptureState::SelectBegin
        || captureState == PmCaptureState::SelectMoved)
        && event->type() == QEvent::MouseMove) {
        int imgX, imgY;
        getImageXY((QMouseEvent*)event, imgX, imgY);
        emit sigCaptureStateChanged(PmCaptureState::SelectMoved, imgX, imgY);
        return true;
    } else if (captureState == PmCaptureState::SelectMoved
            && event->type() == QEvent::MouseButtonRelease) {
        int imgX, imgY;
        getImageXY((QMouseEvent*)event, imgX, imgY);
        emit sigCaptureStateChanged(PmCaptureState::SelectFinished, imgX, imgY);
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

void PmPreviewDisplayWidget::onPreviewConfigChanged(PmPreviewConfig cfg)
{
    m_previewCfg = cfg;
    fixGeometry();
}

void PmPreviewDisplayWidget::onNewActiveFilter(PmFilterRef ref)
{
    //QMutexLocker locker(&m_filterMutex);
    m_activeFilter = ref;
    setEnabled(m_activeFilter.isValid() && m_core->runningEnabled());
}

void PmPreviewDisplayWidget::onRunningEnabledChanged(bool enable)
{
    setEnabled(m_activeFilter.isValid() && enable);
}

void PmPreviewDisplayWidget::onImgSuccess(
    size_t matchIndex, std::string filename, QImage img)
{
    if (matchIndex != m_matchIndex) return;

    m_matchImgWidth = img.width();
    m_matchImgHeight = img.height();

    fixGeometry();

    QMutexLocker locker(&m_matchImgLock);
    m_matchImg = img;
}

void PmPreviewDisplayWidget::onImgFailed(size_t matchIndex)
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

void PmPreviewDisplayWidget::onDestroy(QObject* obj)
{
    while (m_rendering) {
        QThread::sleep(1);
    }
    UNUSED_PARAMETER(obj);
}

void PmPreviewDisplayWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig cfg)
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

void PmPreviewDisplayWidget::onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg)
{
    if (matchIndex != m_matchIndex) return;

    m_roiLeft = cfg.filterCfg.roi_left;
    m_roiBottom = cfg.filterCfg.roi_bottom;
}

void PmPreviewDisplayWidget::drawPreview(void* data, uint32_t cx, uint32_t cy)
{
    auto widget = static_cast<PmPreviewDisplayWidget*>(data);
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

void PmPreviewDisplayWidget::drawEffect()
{
    PmFilterRef &filterRef = m_activeFilter;
    auto renderSrc = filterRef.filter();
    auto filterData = filterRef.filterData();

    if (!renderSrc || !filterData) return;

    filterRef.lockData();
    m_baseWidth = filterData->base_width;
    m_baseHeight = filterData->base_height;
    filterRef.unlockData();

    float orthoLeft, orthoBottom, orthoRight, orthoTop;
    int vpLeft = 0, vpBottom = 0, vpWidth, vpHeight;

    getDisplaySize(vpWidth, vpHeight);

    if (m_previewCfg.previewMode == PmPreviewMode::Video) {
        orthoLeft = 0.f;
        orthoBottom = 0.f;
        orthoRight = m_baseWidth;
        orthoTop = m_baseHeight;
    } else { // if (config.previewMode == PmPreviewMode::Region) {
        orthoLeft = m_roiLeft;
        orthoBottom = m_roiBottom;
        orthoRight = m_roiLeft + m_matchImgWidth;
        orthoTop = m_roiBottom + m_matchImgHeight;

        //float scale = m_previewCfg.previewRegionScale;
        vpLeft = 0.f;
        vpBottom = 0.0f;
    }

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100.0f, 100.0f);
    gs_set_viewport(vpLeft, vpBottom, vpWidth, vpHeight);

    filterRef.lockData();
    filterData->filter_mode =
        (m_core->captureState() == PmCaptureState::Inactive) ? PM_VISUALIZE
                                                             : PM_SELECT_REGION;
    filterRef.unlockData();

    obs_source_video_render(renderSrc);

    gs_projection_pop();
    gs_viewport_pop();
}

void PmPreviewDisplayWidget::drawMatchImage()
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

    //float previewScale = m_previewCfg.previewMatchImageScale;

    gs_effect* effect = m_core->drawMatchImageEffect();
    gs_eparam_t* param = gs_effect_get_param_by_name(effect, "image");
    gs_effect_set_texture(param, m_matchImgTex);

    int displayWidth, displayHeight;
    getDisplaySize(displayWidth, displayHeight);

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(0.0f, (float)m_matchImgWidth, 0.0f, (float)m_matchImgHeight,
        -100.0f, 100.0f);
    gs_set_viewport(0, 0, displayWidth, displayHeight);


    while (gs_effect_loop(effect, "Draw")) {
        gs_draw_sprite(m_matchImgTex, 0, 0, 0);
    }

    gs_projection_pop();
    gs_viewport_pop();

}

void PmPreviewDisplayWidget::getDisplaySize(
    int& displayWidth, int& displayHeight)
{
    auto filterRef = m_core->activeFilterRef();
    auto filterData = filterRef.filterData();

    int cx = 0, cy = 0;
    if (m_previewCfg.previewMode == PmPreviewMode::Video) {
        cx = m_baseWidth;
        cy = m_baseHeight;
    } else { // PmPreviewMode::MatchImage or PmPreviewMode::Region
        cx = m_matchImgWidth;
        cy = m_matchImgHeight;
    }

    // https://stackoverflow.com/questions/6565703/math-algorithm-fit-image-to-screen-retain-aspect-ratio
    int windowWidth = width(), windowHeight = height();
    float ri = (float)cx / float(cy);
    float rs = (float)windowWidth / (float)windowHeight;

    if (rs > ri) {
        displayWidth = int((float)cx * (float)windowHeight / (float)cy);
        displayHeight = windowHeight;
    }
    else {
        displayWidth = windowWidth;
        displayHeight = int((float)cy * (float)windowWidth / (float)cx);
    }
}

void PmPreviewDisplayWidget::fixGeometry()
{
    int displayWidth, displayHeight;
    getDisplaySize(displayWidth, displayHeight);
    m_filterDisplay->setMaximumSize(displayWidth, displayHeight);
}

void PmPreviewDisplayWidget::getImageXY(QMouseEvent *e, int& imgX, int& imgY)
{
    imgX = (float)e->x() * (float)m_baseWidth / (float)width();
    imgY = (float)e->y() * (float)m_baseHeight / (float)height();
}
