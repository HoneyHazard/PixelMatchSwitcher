#pragma once

#include <QWidget>

class QComboBox;
class QPushButton;
class QCheckBox;

class PmCore;

class PmPresetsWidget : public QWidget
{
    Q_OBJECT

public:
    PmPresetsWidget(PmCore* core, QWidget* parent);

signals:
    void sigSelectActiveMatchPreset(std::string name);
    void sigSaveMatchPreset(std::string name);
    void sigRemoveMatchPreset(std::string name);
    void sigRunningEnabledChanged(bool enable);
    void sigSwitchingEnabledChanged(bool enable);

protected slots:
    // local UI handlers
    void onPresetSelected();
    void onPresetSave();
    void onPresetSaveAs();
    void onConfigReset();
    void onPresetRemove();

    // core events handlers
    void onAvailablePresetsChanged();
    void onActivePresetChanged();
    void onActivePresetDirtyStateChanged();
    void onRunningEnabledChanged(bool enable);
    void onSwitchingEnabledChanged(bool enable);

protected:
    static const char* k_unsavedPresetStr;

    PmCore* m_core;

    QCheckBox* m_runningCheckbox;
    QCheckBox* m_switchingCheckbox;

    QComboBox* m_presetCombo;
    QPushButton* m_presetSaveButton;
    QPushButton* m_presetSaveAsButton;
    QPushButton* m_presetResetButton;
    QPushButton* m_presetRemoveButton;
};