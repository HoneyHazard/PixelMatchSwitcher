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
class QDoubleSpinBox;

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
    void sigChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg);

protected slots:
    // core interaction
    void onNewMatchResults(size_t matchIdx, PmMatchResults results);
    void onChangedMatchConfig(size_t matchIdx, PmMatchConfig cfg);
    void onSelectMatchIndex(size_t matchIndex, PmMatchConfig cfg);
    void onNewMultiMatchConfigSize(size_t sz);

    void onImgSuccess(size_t matchIndex, std::string filename, QImage img);
    void onImgFailed(size_t matchIndex, std::string filename);

    // UI element handlers
    void onBrowseButtonReleased();
    void onPickColorButtonReleased();

    // parse UI state into config
    void onConfigUiChanged();

protected:
    // UI assist
    static QColor toQColor(vec3 val);
    //uint32_t toUInt32(QColor val);
    static vec3 toVec3(QColor val);
    void maskModeChanged(PmMaskMode mode, vec3 customColor);
    void roiRangesChanged(
        uint32_t baseWidth, uint32_t baseHeight,
        uint32_t imgWidth, uint32_t imgHeight);

protected:
    static const char *k_unsavedPresetStr;
    static const char* k_failedImgStr;

    size_t m_matchIndex = 0;
    size_t m_multiConfigSz = 0;

    QLabel* m_configCaption;
    QLineEdit* m_labelEdit;
    QLineEdit* m_imgPathEdit;
    QComboBox *m_maskModeCombo;
    QPushButton *m_pickColorButton;
    QLabel *m_maskModeDisplay;
    vec3 m_customColor;
    QSpinBox *m_posXBox, *m_posYBox;
    QDoubleSpinBox *m_perPixelErrorBox;
    QDoubleSpinBox *m_totalMatchThreshBox;
    QLabel *m_matchResultDisplay;

    QPointer<PmCore> m_core;
    PmMatchResults m_prevResults;
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