#pragma once

#include <QWidget>
#include <QPointer>

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

signals:
    void sigNewUiConfig(PmMatchConfig);
    void sigSelectActiveMatchPreset(std::string name);
    void sigSaveMatchPreset(std::string name);
    void sigRemoveMatchPreset(std::string name);

private slots:
    // UI element handlers
    void onBrowseButtonReleased();
    void onPickColorButtonReleased();
    void onPresetSelected();
    void onPresetSave();
    void onPresetSaveAs();
    void onConfigReset();
    void onPresetRemove();

    // parse UI state into config
    void onConfigUiChanged();

    // core signal handlers
    void onImgSuccess(std::string filename);
    void onImgFailed(std::string filename);
    void onPresetsChanged();
    void onPresetStateChanged();
    void onNewMatchResults(PmMatchResults results);

    // shutdown safety
    void onDestroy(QObject *obj); // todo: maybe not needed or useful

protected:
    // parse config into UI state (without triggering handlers)
    void configToUi(const PmMatchConfig &config);

    // preview related
    static void drawPreview(void *data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize(
        const PmMatchConfig &config, const PmMatchResults &results);

    // UI assist
    static QColor toQColor(uint32_t val);
    uint32_t toUInt32(QColor val);
    void maskModeChanged(PmMaskMode mode, uint32_t customColor);
    void roiRangesChanged(
        uint32_t baseWidth, uint32_t baseHeight,
        uint32_t imgWidth, uint32_t imgHeight);

    // todo: maybe not needed or useful
    virtual void closeEvent(QCloseEvent *e) override;

protected:
    static const char *k_unsavedPresetStr;

    QComboBox *m_presetCombo;
    QPushButton *m_presetSaveButton;
    QPushButton *m_presetSaveAsButton;
    QPushButton *m_presetResetButton;
    QPushButton *m_presetRemoveButton;

    QLineEdit* m_imgPathEdit;
    OBSQTDisplay *m_filterDisplay;
    QComboBox *m_maskModeCombo;
    QPushButton *m_pickColorButton;
    QLabel *m_maskModeDisplay;
    uint32_t m_customColor;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_perPixelErrorBox;
    QDoubleSpinBox *m_totalMatchThreshBox;
    QLabel *m_matchResultDisplay;

    QButtonGroup *m_previewModeButtons;
    QStackedWidget *m_previewScaleStack;
    QComboBox *m_videoScaleCombo;
    QComboBox *m_regionScaleCombo;
    QComboBox *m_matchImgScaleCombo;
    QLabel *m_notifyLabel; // TODO: use or remove

    QPointer<PmCore> m_core;
    PmMatchResults m_prevResults;
    bool m_rendering = false; // safeguard against deletion while rendering in obs render thread
};
