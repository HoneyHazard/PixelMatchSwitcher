#pragma once

#include <QGroupBox>
#include <QMessageBox>
#include <QList>
#include <QPointer>

#include "pm-structs.hpp"

class QComboBox;
class QPushButton;

class PmCore;
class PmPresetsRetriever;

class PmPresetsWidget : public QGroupBox
{
    Q_OBJECT

public:
    PmPresetsWidget(PmCore* core, QWidget* parent);
    bool proceedWithExit();

signals:
    void sigMatchPresetSelect(std::string name);
    void sigMatchPresetSave(std::string name);
    void sigMatchPresetRemove(std::string name);
    void sigMultiMatchConfigReset();
    void sigMatchPresetActiveRevert();
    void sigMatchPresetExport(std::string filename, QList<std::string> presets);
    void sigMatchPresetsImport(std::string filename);
    void sigMatchPresetsAppend(PmMatchPresets newPresets);

protected slots:
    // local UI handlers
    void onPresetSelected();
    void onPresetRevert();
    void onPresetSave();
    void onPresetSaveAs();
    void onNewConfig();
    void onPresetRemove();
    void onPresetImport();
    void onPresetDownload();
    void onPresetExport();

    // core events handlers
    void onAvailablePresetsChanged();
    void onActivePresetChanged();
    void onActivePresetDirtyStateChanged();
    void onPresetsImportAvailable(PmMatchPresets availablePresets);
    void onPresetsDownloadAvailable(QList<std::string> presetNames);

protected:
    QMessageBox::ButtonRole promptUnsavedProceed();

    static const char* k_unsavedPresetStr;
    static const char *k_defaultXmlDownload;

    QPushButton* prepareButton(const char* tooltip, const char* icoPath);

    PmCore* m_core;
    QPointer<PmPresetsRetriever> m_presetsRetriever;

    QComboBox* m_presetCombo;
    QPushButton* m_presetRevertButton;
    QPushButton* m_presetSaveButton;
    QPushButton* m_presetSaveAsButton;
    QPushButton* m_newConfigButton;
    QPushButton* m_presetRemoveButton;
    QPushButton *m_presetImportButton;
    QPushButton *m_presetDownloadButton;
    QPushButton *m_presetExportButton;
};
