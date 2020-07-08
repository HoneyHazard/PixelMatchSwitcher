#pragma once

#include <QWidget>
#include <QMutex>
#include <QPointer>

#include "pm-structs.hpp"
#include "pm-filter-ref.hpp"

class OBSQTDisplay;

class PmCore;
class QComboBox;
class QStackedWidget;
class QButtonGroup;

class PmPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    PmPreviewWidget(PmCore* core, QWidget *parent);
    ~PmPreviewWidget();

signals:
    void sigPreviewConfigChanged(PmPreviewConfig cfg);

protected slots:
    // reaction to core events
    void onPreviewConfigChanged(PmPreviewConfig cfg);

    void onSelectMatchIndex(size_t matchindex, PmMatchConfig cfg);
    void onChangedMatchConfig(size_t matchIndex, PmMatchConfig cfg);

    void onNewActiveFilter(PmFilterRef ref);
    void onImgSuccess(size_t matchIndex, std::string filename, QImage img);
    void onImgFailed(size_t matchIndex, std::string filename);

    // parse UI state into config
    void onConfigUiChanged();

    // shutdown safety
    void onDestroy(QObject* obj); // todo: maybe not needed or useful

protected:
    // preview related
    static void drawPreview(void* data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize();
    void enableRegionViews(bool enable);

    // todo: maybe not needed or useful
    virtual void closeEvent(QCloseEvent* e) override;

    QButtonGroup* m_previewModeButtons;
    QStackedWidget* m_previewScaleStack;
    QComboBox* m_videoScaleCombo;
    QComboBox* m_regionScaleCombo;
    QComboBox* m_matchImgScaleCombo;
    OBSQTDisplay* m_filterDisplay;

    QPointer<PmCore> m_core;
    
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