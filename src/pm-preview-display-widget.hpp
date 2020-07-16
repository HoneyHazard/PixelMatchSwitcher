#pragma once

#include <QWidget>
#include <QMutex>
#include <QPointer>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

class OBSQTDisplay;

class PmCore;

/**
 * @brief Provides a preview of filter's visualization output and match image
 */
class PmPreviewDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    PmPreviewDisplayWidget(PmCore* core, QWidget *parent);
    ~PmPreviewDisplayWidget();

public slots:
    void fixGeometry();

protected slots:
    // reaction to core events
    void onPreviewConfigChanged(PmPreviewConfig cfg);

    void onSelectMatchIndex(size_t matchindex, PmMatchConfig cfg);
    void onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg);

    void onNewActiveFilter(PmFilterRef ref);
    void onRunningEnabledChanged(bool enable);
    void onImgSuccess(size_t matchIndex, std::string filename, QImage img);
    void onImgFailed(size_t matchIndex);

    // shutdown safety
    void onDestroy(QObject* obj); // todo: maybe not needed or useful

protected:
    // callback for OBS display system
    static void drawPreview(void* data, uint32_t cx, uint32_t cy);

    // draw helpers
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize();

    void closeEvent(QCloseEvent* e) override; // todo: maybe not needed or useful

    OBSQTDisplay* m_filterDisplay;

    PmCore* m_core;
    
    size_t m_matchIndex = 0;
    PmPreviewConfig m_previewCfg;
    PmMatchConfig m_matchConfig;
    int m_roiLeft = 0, m_roiBottom = 0;
    int m_matchImgWidth = 0, m_matchImgHeight = 0;

    //QMutex m_filterMutex;
    PmFilterRef m_activeFilter;

    bool m_rendering = false; // safeguard against deletion while rendering in obs render thread
    QMutex m_matchImgLock;
    QImage m_matchImg;
    gs_texture_t* m_matchImgTex = nullptr;
};