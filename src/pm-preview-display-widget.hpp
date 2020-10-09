#pragma once

#include <QWidget>
#include <QMutex>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

class OBSQTDisplay;
class QStackedWidget;
class PmCore;
class PmImageView;

/**
 * @brief Provides a preview of filter's visualization output and match image
 */
class PmPreviewDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    PmPreviewDisplayWidget(PmCore* core, QWidget *parent);
    ~PmPreviewDisplayWidget() override;

signals:
    void sigCaptureStateChanged(PmCaptureState state, int x=-1, int y=-1);

protected slots:
    // reaction to core events
    void onPreviewConfigChanged(PmPreviewConfig cfg);

    void onMatchConfigSelect(size_t matchindex, PmMatchConfig cfg);
    void onMatchConfigChanged(size_t matchIndex, PmMatchConfig cfg);

    void onActiveFilterChanged(PmFilterRef ref);
    void onRunningEnabledChanged(bool enable);
    void onMatchImageLoadSuccess(
        size_t matchIndex, std::string filename, QImage img);
    void onMatchImageLoadFailed(size_t matchIndex);

    // shutdown safety
    void onFrontendExiting();
    void onDestroy(QObject* obj); // todo: maybe not needed or useful

protected:
    // callback for OBS display system
    static void drawFilter(void* data, uint32_t cx, uint32_t cy);

    // draw helpers
    void drawFilter();

    // display configuration related
    void updateDisplayState(PmPreviewConfig cfg, size_t matchIndex,
                            bool runningEnabled, PmFilterRef activeFilter);
    void getDisplaySize(int& displayWidth, int& displayHeight);
    void getImageXY(QMouseEvent *mouseEvent, int &imgX, int &imgY);
    void fixGeometry();

    void resizeEvent(QResizeEvent* e) override;    
    void closeEvent(QCloseEvent* e) override; // todo: maybe not needed or useful

    bool eventFilter(QObject* obj, QEvent* event) override; // for display events

    QStackedWidget* m_displayStack;
    OBSQTDisplay* m_filterDisplay;
    PmImageView* m_imageView;

    PmCore* m_core;
    
    size_t m_matchIndex = 0;
    PmPreviewConfig m_previewCfg;
    int m_roiLeft = 0, m_roiBottom = 0;
    
    int m_baseWidth = 0, m_baseHeight = 0;
    int m_matchImgWidth = 0, m_matchImgHeight = 0;

    PmFilterRef m_activeFilter;

    bool m_renderingFilter = false; // safeguard against deletion while rendering in obs render thread
};
