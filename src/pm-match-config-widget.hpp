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
class PmMatchConfigWidget : public QWidget
{
    Q_OBJECT

public:
    PmMatchConfigWidget(PmCore *core, QWidget *parent);
    ~PmMatchConfigWidget() override;

    size_t matchIndex() const { return m_matchIndex; }

signals:
    void sigNewMatchConfig(size_t matchIdx, PmMatchConfig cfg);

protected slots:
    // core interaction
    void onNewMatchResults(size_t matchIdx, PmMatchResults results);
    void onNewMatchConfig(size_t matchIdx, PmMatchConfig cfg);
    void onSelectMatchIndex(size_t matchindex, PmMatchConfig cfg);

    void onImgSuccess(size_t matchIndex, std::string filename, QImage img);
    void onImgFailed(size_t matchIndex, std::string filename);

    // UI element handlers
    void onBrowseButtonReleased();
    void onPickColorButtonReleased();

    // parse UI state into config
    void onConfigUiChanged();

    // shutdown safety
    void onDestroy(QObject *obj); // todo: maybe not needed or useful

protected:
    // parse config into UI state (without triggering handlers)

    // preview related
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize(
        const PmMatchConfig &config, const PmMatchResults &results);

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
    PmMatchResults m_prevResults;
    bool m_rendering = false; // safeguard against deletion while rendering in obs render thread
    QMutex m_matchImgLock;
    QImage m_matchImg;
    gs_texture_t* m_matchImgTex = nullptr;
};

// core signal handlers
//void onPresetsChanged();
//void onPresetStateChanged();

#if 0
void sigSelectActiveMatchPreset(std::string name);
void sigSaveMatchPreset(std::string name);
void sigRemoveMatchPreset(std::string name);
#endif

#if 0
void onPresetSelected();
void onPresetSave();
void onPresetSaveAs();
void onConfigReset();
void onPresetRemove();
#endif

#if 0
QComboBox* m_presetCombo;
QPushButton* m_presetSaveButton;
QPushButton* m_presetSaveAsButton;
QPushButton* m_presetResetButton;
QPushButton* m_presetRemoveButton;
#endif