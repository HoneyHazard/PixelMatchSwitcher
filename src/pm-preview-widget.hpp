#pragma once

#include <QWidget>
#include <QMutex>
#include <QPointer>

#include "pm-structs.hpp"

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
    void sigChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg);

protected slots:
    // reaction to core events
    void onSelectMatchIndex(size_t matchindex, PmMatchConfig cfg);
    void onChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg);
    void onNewMatchResults(size_t idx, PmMatchResults results);
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
    void updateFilterDisplaySize(
        const PmMatchConfig& config, const PmMatchResults& results);
    void unsetTexture();

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
    bool m_rendering = false; // safeguard against deletion while rendering in obs render thread
    QMutex m_matchImgLock;
    QImage m_matchImg;
    gs_texture_t* m_matchImgTex = nullptr;
};