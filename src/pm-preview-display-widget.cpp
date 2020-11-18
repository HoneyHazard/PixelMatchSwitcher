#include "pm-preview-display-widget.hpp"
#include "pm-image-view.hpp"
#include "pm-core.hpp"
#include "obs-frontend-api.h"

#include <qt-display.hpp>
#include <display-helpers.hpp>

#include <QStackedWidget>
#include <QVBoxLayout>

#include <QThread> // sleep
#include <QMouseEvent>

PmPreviewDisplayWidget::PmPreviewDisplayWidget(PmCore* core, QWidget* parent)
: QWidget(parent)
, m_core(core)
{
    // match display area
    m_filterDisplay = new OBSQTDisplay(this);
    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(m_filterDisplay->GetDisplay(),
                                      PmPreviewDisplayWidget::drawFilter, this);
    };
    m_filterDisplay->installEventFilter(this);
    connect(m_filterDisplay, &OBSQTDisplay::DisplayCreated,
            addDrawCallback);
    connect(m_filterDisplay, &OBSQTDisplay::destroyed,
        this, &PmPreviewDisplayWidget::onDestroy, Qt::DirectConnection);

    // view for displaying match image and messages
    m_imageView = new PmImageView(this);
   
    // display stack
    m_displayStack = new QStackedWidget(this);
    m_displayStack->addWidget(m_filterDisplay);
    m_displayStack->addWidget(m_imageView);

    // main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_displayStack);
    mainLayout->setStretchFactor(m_displayStack, 1000);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    const Qt::ConnectionType qc = Qt::QueuedConnection;

    // core event handlers
    connect(m_core, &PmCore::sigMatchConfigSelect,
            this, &PmPreviewDisplayWidget::onMatchConfigSelect, qc);
    connect(m_core, &PmCore::sigMatchConfigChanged,
        this, &PmPreviewDisplayWidget::onMatchConfigChanged, qc);
    connect(m_core, &PmCore::sigMatchImageLoadSuccess,
            this, &PmPreviewDisplayWidget::onMatchImageLoadSuccess, qc);
    connect(m_core, &PmCore::sigMatchImageLoadFailed,
            this, &PmPreviewDisplayWidget::onMatchImageLoadFailed, qc);
    connect(m_core, &PmCore::sigPreviewConfigChanged,
            this, &PmPreviewDisplayWidget::onPreviewConfigChanged, qc);
    connect(m_core, &PmCore::sigActiveFilterChanged,
            this, &PmPreviewDisplayWidget::onActiveFilterChanged, qc);

    // signals sent to the core
    connect(this, &PmPreviewDisplayWidget::sigCaptureStateChanged,
            m_core, &PmCore::onCaptureStateChanged, qc);

    // finish init
    onActiveFilterChanged(m_core->activeFilterRef());
    onRunningEnabledChanged(m_core->runningEnabled());
    onPreviewConfigChanged(m_core->previewConfig());
    size_t selIdx = m_core->selectedConfigIndex();
    onMatchConfigSelect(selIdx, m_core->matchConfig(selIdx));
}

PmPreviewDisplayWidget::~PmPreviewDisplayWidget()
{
    while (m_renderingFilter) {
        QThread::sleep(1);
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
        m_filterDisplay->GetDisplay(), PmPreviewDisplayWidget::drawFilter, this);
    QWidget::closeEvent(e);
}

bool PmPreviewDisplayWidget::eventFilter(QObject* obj, QEvent* event)
{
    auto captureState = m_core->captureState();

    if ((captureState == PmCaptureState::Activated
      || captureState == PmCaptureState::SelectFinished)
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

    updateDisplayState(
        cfg, m_matchIndex, m_core->runningEnabled(), m_activeFilter);
}

void PmPreviewDisplayWidget::onActiveFilterChanged(PmFilterRef ref)
{
    //QMutexLocker locker(&m_filterMutex);
    m_activeFilter = ref;
    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        m_activeFilter.lockData();
        m_baseWidth = int(filterData->base_width);
        m_baseHeight = int(filterData->base_height);
        m_activeFilter.unlockData();
    } else {
        m_baseWidth = 0;
        m_baseHeight = 0;
    }
    updateDisplayState(
        m_previewCfg, m_matchIndex, m_core->runningEnabled(), ref);
}

void PmPreviewDisplayWidget::onRunningEnabledChanged(bool enable)
{
    updateDisplayState(
        m_previewCfg, m_matchIndex, enable, m_activeFilter);
}

void PmPreviewDisplayWidget::onMatchImageLoadSuccess(
    size_t matchIndex, std::string filename, QImage img)
{
    if (matchIndex != m_matchIndex) return;

    updateDisplayState(
        m_previewCfg, m_matchIndex, m_core->runningEnabled(), m_activeFilter);

    UNUSED_PARAMETER(filename);
    UNUSED_PARAMETER(img);
}

void PmPreviewDisplayWidget::onMatchImageLoadFailed(size_t matchIndex)
{
    if (matchIndex != m_matchIndex) return;

    updateDisplayState(
        m_previewCfg, m_matchIndex, m_core->runningEnabled(), m_activeFilter);
}

void PmPreviewDisplayWidget::onFrontendExiting()
{
    m_activeFilter.reset();

    obs_display_remove_draw_callback(m_filterDisplay->GetDisplay(),
        PmPreviewDisplayWidget::drawFilter, this);
}

void PmPreviewDisplayWidget::onDestroy(QObject* obj)
{
    while (m_renderingFilter) {
        QThread::sleep(1);
    }
    UNUSED_PARAMETER(obj);
}

void PmPreviewDisplayWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;
    onMatchConfigChanged(matchIndex, cfg);
}

void PmPreviewDisplayWidget::onMatchConfigChanged(
    size_t matchIndex, PmMatchConfig cfg)
{
    if (matchIndex != m_matchIndex) return;

    m_roiLeft = cfg.filterCfg.roi_left;
    m_roiBottom = cfg.filterCfg.roi_bottom;

    updateDisplayState(
        m_previewCfg, matchIndex, m_core->runningEnabled(), m_activeFilter);
}

void PmPreviewDisplayWidget::drawFilter(void* data, uint32_t cx, uint32_t cy)
{
    auto widget = static_cast<PmPreviewDisplayWidget*>(data);

    if (widget->m_previewCfg.previewMode != PmPreviewMode::MatchImage) {
        widget->m_renderingFilter = true;
        widget->drawFilter();
        widget->m_renderingFilter = false;
    }

    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

void PmPreviewDisplayWidget::drawFilter()
{
    PmFilterRef &filterRef = m_activeFilter;
    auto renderSrc = filterRef.filter();
    auto filterData = filterRef.filterData();

    float orthoLeft, orthoBottom, orthoRight, orthoTop;
    int vpLeft = 0, vpBottom = 0, vpWidth, vpHeight;
    bool skip = false;
    auto captureState = m_core->captureState();

    if (!renderSrc || !filterData)
        goto done;

    filterRef.lockData();
    m_baseWidth = int(filterData->base_width);
    m_baseHeight = int(filterData->base_height);
    filterRef.unlockData();

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

    filterRef.lockData();
    if (filterData->filter_mode == PM_MASK_BEGIN
     || filterData->filter_mode == PM_MASK_END
     || filterData->filter_mode == PM_SNAPSHOT) {
        // don't mess with these
        skip = true;
    } else {
        switch (captureState) {
        case PmCaptureState::Inactive:
            filterData->filter_mode = PM_MATCH_VISUALIZE; break;
        case PmCaptureState::Automask:
            filterData->filter_mode = PM_MASK_VISUALIZE; break;
        case PmCaptureState::Activated:
        case PmCaptureState::SelectBegin:
        case PmCaptureState::SelectMoved:
        case PmCaptureState::SelectFinished:
        case PmCaptureState::Accepted:
            filterData->filter_mode = PM_SELECT_REGION_VISUALIZE; break;
        }
    }
    filterRef.unlockData();

    if (skip) goto done;

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100.0f, 100.0f);
    gs_set_viewport(vpLeft, vpBottom, vpWidth, vpHeight);

    obs_source_video_render(renderSrc);

    gs_projection_pop();
    gs_viewport_pop();

done:
    obs_source_release(renderSrc);
}

void PmPreviewDisplayWidget::updateDisplayState(
    PmPreviewConfig cfg, size_t matchIndex, 
    bool runningEnabled, PmFilterRef activeFilter)
{
    QImage matchImg = m_core->matchImage(matchIndex);
    if (matchImg.isNull()) {
        m_matchImgWidth = 0;
        m_matchImgHeight = 0;
    } else {
        m_matchImgWidth = matchImg.width();
        m_matchImgHeight = matchImg.height();
    }

    if (cfg.previewMode == PmPreviewMode::MatchImage) {
        QImage matchImg = m_core->matchImage(matchIndex);
        if (matchImg.isNull()) {
            m_imageView->showMessage(obs_module_text(
                "No Match Image Loaded."), Qt::yellow);
        } else {
            m_imageView->showImage(matchImg);
        }
        m_displayStack->setCurrentWidget(m_imageView);
    } else if (!runningEnabled) {
        m_imageView->showMessage(obs_module_text(
            "Check \"Enable Matching\" <br /><br />"
            "to connect to Pixel Match filter(s)."), QColor(255, 159, 0));
        m_displayStack->setCurrentWidget(m_imageView);
    } else if (!activeFilter.isValid()) {
        m_imageView->showMessage(obs_module_text(
            "Filter not found in the current scene.<br />"
            "<br />"
            "Right click an OBS video source. Select Filters.<br />"
            "Add Pixel Match filter to the Effect Filters list.<br />"
            "<br />"
            "Filter already added? Ensure the scene is selected."),
            Qt::yellow);
        m_displayStack->setCurrentWidget(m_imageView);
    } else {
        m_displayStack->setCurrentWidget(m_filterDisplay);
    }

    fixGeometry();
}

void PmPreviewDisplayWidget::getDisplaySize(
    int& displayWidth, int& displayHeight)
{
    int cx = 0, cy = 0;
    if (m_previewCfg.previewMode == PmPreviewMode::Video) {
        cx = m_baseWidth;
        cy = m_baseHeight;
    } else if (m_previewCfg.previewMode == PmPreviewMode::MatchImage) {
        cx = m_matchImgWidth;
        cy = m_matchImgHeight;
    } else {
        // region mode should have match image dimmensions but only has anything
        // actual to show when a filter is active
        if (m_activeFilter.isValid()) {
            cx = m_matchImgWidth;
            cy = m_matchImgHeight;
        }
    }
 
    if (cx == 0 || cy == 0) {
        cx = width();
        cy = height();
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
    m_imageView->setMaximumSize(displayWidth, displayHeight);
    m_imageView->fixGeometry();
    m_filterDisplay->setMaximumSize(displayWidth, displayHeight);
}

void PmPreviewDisplayWidget::getImageXY(QMouseEvent *e, int& imgX, int& imgY)
{
    imgX = int(
        (float)e->x() * (float)m_baseWidth / (float)m_filterDisplay->width());
    imgY = int(
        (float)e->y() * (float)m_baseHeight / (float)m_filterDisplay->height());
}
