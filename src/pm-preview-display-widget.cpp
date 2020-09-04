#include "pm-preview-display-widget.hpp"
#include "pm-image-view.hpp"
#include "pm-core.hpp"

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

void PmPreviewDisplayWidget::onNewActiveFilter(PmFilterRef ref)
{
    //QMutexLocker locker(&m_filterMutex);
    m_activeFilter = ref;
    //setEnabled(m_activeFilter.isValid() && m_core->runningEnabled());
    auto filterData = m_activeFilter.filterData();
    if (filterData) {
        m_activeFilter.lockData();
        m_baseWidth = filterData->base_width;
        m_baseHeight = filterData->base_height;
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
    //setEnabled(m_activeFilter.isValid() && enable);
    updateDisplayState(
        m_previewCfg, m_matchIndex, enable, m_activeFilter);
}

void PmPreviewDisplayWidget::onImgSuccess(
    size_t matchIndex, std::string filename, QImage img)
{
    if (matchIndex != m_matchIndex) return;

    updateDisplayState(
        m_previewCfg, m_matchIndex, m_core->runningEnabled(), m_activeFilter);
}

void PmPreviewDisplayWidget::onImgFailed(size_t matchIndex)
{
    if (matchIndex != m_matchIndex) return;
    
    updateDisplayState(
        m_previewCfg, m_matchIndex, m_core->runningEnabled(), m_activeFilter);
}

void PmPreviewDisplayWidget::onDestroy(QObject* obj)
{
    while (m_renderingFilter) {
        QThread::sleep(1);
    }
    UNUSED_PARAMETER(obj);
}

void PmPreviewDisplayWidget::onSelectMatchIndex(size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;
    onChangedMatchConfig(matchIndex, cfg);
}

void PmPreviewDisplayWidget::onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg)
{
    if (matchIndex != m_matchIndex) return;

    m_roiLeft = cfg.filterCfg.roi_left;
    m_roiBottom = cfg.filterCfg.roi_bottom;
    
#if 0
    if (m_core->matchImageLoaded(matchIndex)) {
        onImgSuccess(matchIndex,
            m_core->matchImgFilename(matchIndex),
            m_core->matchImage(matchIndex));
    }
    else {
        onImgFailed(matchIndex);
    }
#endif

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

    bool skip = false;
    auto captureState = m_core->captureState();
    
    filterRef.lockData();
    if (filterData->filter_mode == PM_MASK_BEGIN
     || filterData->filter_mode == PM_MASK_END
     || filterData->filter_mode == PM_SNAPSHOT) {
        // don't mess with these
        skip = true;
    } else {

        enum pm_filter_mode filterMode;
        switch (captureState) {
        case PmCaptureState::Inactive:
            filterMode = PM_MATCH_VISUALIZE; break;
        case PmCaptureState::Automask:
            filterMode = PM_MASK_VISUALIZE; break;
        case PmCaptureState::Activated:
        case PmCaptureState::SelectBegin:
        case PmCaptureState::SelectMoved:
        case PmCaptureState::SelectFinished:
        case PmCaptureState::Accepted:
            filterMode = PM_SELECT_REGION; break;
        default: 
            filterMode = filterData->filter_mode; break;
        }
        filterData->filter_mode = filterMode;
    }
    filterRef.unlockData();

    if (skip) return;

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, -100.0f, 100.0f);
    gs_set_viewport(vpLeft, vpBottom, vpWidth, vpHeight);

    obs_source_video_render(renderSrc);

    gs_projection_pop();
    gs_viewport_pop();
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
            m_imageView->showDisabled(obs_module_text(
                "No Match Image Loaded."));
        } else {
            m_imageView->showImage(matchImg);
        }
        m_displayStack->setCurrentWidget(m_imageView);
    } else if (!runningEnabled) {
        m_imageView->showDisabled(obs_module_text(
            "Check \"Enable Matching\" to connect to Pixel Match filters"));
        m_displayStack->setCurrentWidget(m_imageView);
    } else if (!activeFilter.isValid() || !activeFilter.isActive()) {
        m_imageView->showDisabled(obs_module_text(
            "Add Pixel Match filters to your video sources"));
        m_displayStack->setCurrentWidget(m_imageView);
    } else {
        m_displayStack->setCurrentWidget(m_filterDisplay);
    }

    fixGeometry();
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
    } else if (m_previewCfg.previewMode == PmPreviewMode::MatchImage) {
        cx = m_matchImgWidth;
        cy = m_matchImgHeight;
    } else {
        // region mode has match image dimmensions but only has anything
        // to show when a filter is active
        if (m_activeFilter.isActive()) {
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
