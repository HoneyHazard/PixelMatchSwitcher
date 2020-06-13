#pragma once

#include <QWidget>
#include <QPointer>
#include <QMutex>

#include "pm-structs.hpp"

class PmCore;

class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;
class QSpinBox;
class OBSQTDisplay;
class QDoubleSpinBox;
class QButtonGroup;
class QStackedWidget;

/*!
 * \brief UI tab that shows match settings, UI preview and preview settings
 */
class PmMatchTab : public QWidget
{
    Q_OBJECT

public:
    PmMatchTab(PmCore *core, QWidget *parent);
    ~PmMatchTab() override;

    void setMatchIndex(size_t idx) { m_matchIndex = idx; }
    size_t matchIndex() const { return m_matchIndex; }

signals:
    void sigNewUiConfig(PmMultiMatchConfig);

#if 0
    void sigSelectActiveMatchPreset(std::string name);
    void sigSaveMatchPreset(std::string name);
    void sigRemoveMatchPreset(std::string name);
#endif

private slots:
    // UI element handlers
    void onBrowseButtonReleased();
    void onPickColorButtonReleased();

#if 0
    void onPresetSelected();
    void onPresetSave();
    void onPresetSaveAs();
    void onConfigReset();
    void onPresetRemove();
#endif

    // parse UI state into config
    void onConfigUiChanged();

    // core signal handlers
    void onImgSuccess(size_t matchIndex, std::string filename, QImage img);
    void onImgFailed(size_t matchIndex, std::string filename);
    //void onPresetsChanged();
    //void onPresetStateChanged();
    void onNewMatchResults(PmMultiMatchResults results);

    // shutdown safety
    void onDestroy(QObject *obj); // todo: maybe not needed or useful

protected:
    // parse config into UI state (without triggering handlers)
    void configToUi(const PmMultiMatchConfig &multiConfig);

    // preview related
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize(
        const PmMultiMatchConfig &multiConfig, 
        const PmMultiMatchResults &multiResults);

    // UI assist
    static QColor toQColor(vec3 val);
    //uint32_t toUInt32(QColor val);
    static vec3 toVec3(QColor val);
    void maskModeChanged(PmMaskMode mode, vec3 customColor);
    void roiRangesChanged(
        uint32_t baseWidth, uint32_t baseHeight,
        uint32_t imgWidth, uint32_t imgHeight);

    // todo: maybe not needed or useful
    virtual void closeEvent(QCloseEvent *e) override;

protected:
    static const char *k_unsavedPresetStr;
    static const char* k_failedImgStr;

    size_t m_matchIndex;

#if 0
    QComboBox *m_presetCombo;
    QPushButton *m_presetSaveButton;
    QPushButton *m_presetSaveAsButton;
    QPushButton *m_presetResetButton;
    QPushButton *m_presetRemoveButton;
#endif

    QLineEdit* m_imgPathEdit;
    OBSQTDisplay *m_filterDisplay;
    QComboBox *m_maskModeCombo;
    QPushButton *m_pickColorButton;
    QLabel *m_maskModeDisplay;
    vec3 m_customColor;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_perPixelErrorBox;
    QDoubleSpinBox *m_totalMatchThreshBox;
    QLabel *m_matchResultDisplay;

    QButtonGroup *m_previewModeButtons;
    QStackedWidget *m_previewScaleStack;
    QComboBox *m_videoScaleCombo;
    QComboBox *m_regionScaleCombo;
    QComboBox *m_matchImgScaleCombo;

    QPointer<PmCore> m_core;
    PmMultiMatchResults m_prevResults;
    bool m_rendering = false; // safeguard against deletion while rendering in obs render thread
    QMutex m_matchImgLock;
    QImage m_matchImg;
    gs_texture_t* m_matchImgTex = nullptr;
};
