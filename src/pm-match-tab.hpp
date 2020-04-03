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
    void onBrowseButtonReleased();
    void onColorComboIndexChanged();
    void onImgSuccess(std::string filename);
    void onImgFailed(std::string filename);
    void onNewMatchResults(PmMatchResults results);
    void onConfigUiChanged();
    void onDestroy(QObject *obj);

    void onPresetsChanged();
    void onPresetStateChanged();

    void onPresetSelected();
    void onPresetSave();
    void onPresetSaveAs();
    void onConfigReset();
    void onPresetRemove();

protected:
    virtual void closeEvent(QCloseEvent *e) override;

    static void drawPreview(void *data, uint32_t cx, uint32_t cy);
    void drawEffect();
    void drawMatchImage();
    void updateFilterDisplaySize(
        const PmMatchConfig &config, const PmMatchResults &results);

    void maskModeChanged(PmMaskMode mode, QColor color);
    void roiRangesChanged(int baseWidth, int baseHeight, int imgWidth, int imgHeight);
    void configToUi(const PmMatchConfig &config);

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
    QLabel *m_maskModeDisplay;
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
