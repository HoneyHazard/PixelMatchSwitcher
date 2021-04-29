#include "pm-presets-widget.hpp"
#include "pm-preset-exists-dialog.hpp"
#include "pm-list-selector-dialog.hpp"
#include "pm-retriever-progress-dialog.hpp"
#include "pm-presets-retriever.hpp"
#include "pm-core.hpp"

#include <sstream>

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QFileDialog>

const char* PmPresetsWidget::k_unsavedPresetStr
    = obs_module_text("<unsaved preset>");
const char* PmPresetsWidget::k_defaultXmlUrl
    = "https://raw.githubusercontent.com/HoneyHazard/PixelMatchPresets/main/meta.xml";

PmPresetsWidget::PmPresetsWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Presets"), parent)
, m_core(core)
{
    const Qt::ConnectionType qc = Qt::QueuedConnection;

    // top level layout
    QHBoxLayout* presetLayout = new QHBoxLayout;
    setLayout(presetLayout);

    // preset controls
    m_presetCombo = new QComboBox(this);
    m_presetCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
    m_presetCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    connect(m_presetCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(onPresetSelected()));
    presetLayout->addWidget(m_presetCombo);

    m_presetSaveButton = prepareButton(obs_module_text("Save Preset"),
        ":/res/images/icons8-save-32.png");
    connect(m_presetSaveButton, &QPushButton::released,
            this, &PmPresetsWidget::onPresetSave, qc);
    presetLayout->addWidget(m_presetSaveButton);

    m_presetSaveAsButton = prepareButton(obs_module_text("Save Preset As"),
        ":/res/images/icons8-save-as-32.png");
    connect(m_presetSaveAsButton, &QPushButton::released,
        this, &PmPresetsWidget::onPresetSaveAs, qc);
    presetLayout->addWidget(m_presetSaveAsButton);

    m_presetRevertButton = prepareButton(obs_module_text("Revert Changes"),
        ":/res/images/revert.svg");
    m_presetRevertButton->setProperty("themeID", QVariant("revertIcon"));
    connect(m_presetRevertButton, &QPushButton::released,
            this, &PmPresetsWidget::onPresetRevert, qc);
    presetLayout->addWidget(m_presetRevertButton);

    m_newConfigButton = prepareButton(obs_module_text("New Configuration"),
        ":/res/images/icons8-file.svg");
    connect(m_newConfigButton, &QPushButton::released,
            this, &PmPresetsWidget::onNewConfig, qc);
    presetLayout->addWidget(m_newConfigButton);

    m_presetRemoveButton = prepareButton(obs_module_text("Remove Preset"),
        ":/res/images/icons8-trash.svg");
    connect(m_presetRemoveButton, &QPushButton::released,
        this, &PmPresetsWidget::onPresetRemove, qc);
    presetLayout->addWidget(m_presetRemoveButton);

    // divider #1
    QFrame *divider1 = new QFrame(this);
    divider1->setFrameShape(QFrame::VLine);
    divider1->setFrameShadow(QFrame::Plain);
    divider1->setFixedWidth(1);
    presetLayout->addWidget(divider1);

    m_presetImportButton = prepareButton(obs_module_text("Import Preset(s)"),
        ":/res/images/icons8-import-32.png");
    connect(m_presetImportButton, &QPushButton::released,
            this, &PmPresetsWidget::onPresetImportXml, qc);
    presetLayout->addWidget(m_presetImportButton);

    m_presetDownloadButton = prepareButton(
        obs_module_text("Download Preset(s)"),
        ":/res/images/icons8-download-32.png");
    connect(m_presetDownloadButton, &QPushButton::released,
            this, &PmPresetsWidget::onPresetDownload, qc);
    presetLayout->addWidget(m_presetDownloadButton);

    // divider #2
    QFrame *divider2 = new QFrame(this);
    divider2->setFrameShape(QFrame::VLine);
    divider2->setFrameShadow(QFrame::Plain);
    divider2->setFixedWidth(1);
    presetLayout->addWidget(divider2);

    m_presetExportButton = prepareButton(obs_module_text("Export Preset(s)"),
        ":/res/images/icons8-export-32.png");
    connect(m_presetExportButton, &QPushButton::released,
            this, &PmPresetsWidget::onPresetExportXml, qc);
    presetLayout->addWidget(m_presetExportButton);

    // core event handlers
    connect(m_core, &PmCore::sigAvailablePresetsChanged,
            this, &PmPresetsWidget::onAvailablePresetsChanged, qc);
    connect(m_core, &PmCore::sigActivePresetChanged,
            this, &PmPresetsWidget::onActivePresetChanged, qc);
    connect(m_core, &PmCore::sigActivePresetDirtyChanged,
            this, &PmPresetsWidget::onActivePresetDirtyStateChanged, qc);
    connect(m_core, &PmCore::sigPresetsImportAvailable,
            this, &PmPresetsWidget::onPresetsImportAvailable, qc);
    connect(m_core, &PmCore::sigMatchImagesOrphaned,
            this, &PmPresetsWidget::onMatchImagesOrphaned, qc);

    // local signals -> core slots
    connect(this, &PmPresetsWidget::sigMatchPresetSave,
            m_core, &PmCore::onMatchPresetSave, qc);
    connect(this, &PmPresetsWidget::sigMatchPresetSelect,
            m_core, &PmCore::onMatchPresetSelect, qc);
    connect(this, &PmPresetsWidget::sigMatchPresetRemove,
            m_core, &PmCore::onMatchPresetRemove, qc);
    connect(this, &PmPresetsWidget::sigMultiMatchConfigReset,
            m_core, &PmCore::onMultiMatchConfigReset, qc);
    connect(this, &PmPresetsWidget::sigMatchPresetActiveRevert,
            m_core, &PmCore::onMatchPresetActiveRevert, qc);
    connect(this, &PmPresetsWidget::sigMatchPresetExport,
            m_core, &PmCore::onMatchPresetExport, qc);
    connect(this, &PmPresetsWidget::sigMatchPresetsImport,
            m_core, &PmCore::onMatchPresetsImport, qc);
    connect(this, &PmPresetsWidget::sigMatchPresetsAppend,
            m_core, &PmCore::onMatchPresetsAppend, qc);
    connect(this, &PmPresetsWidget::sigMatchImagesRemove,
            m_core, &PmCore::onMatchImagesRemove, qc);

    // finish init state
    onAvailablePresetsChanged();
    onActivePresetChanged();
    onActivePresetDirtyStateChanged();
}

void PmPresetsWidget::onAvailablePresetsChanged()
{
    auto presetNames = m_core->matchPresetNames();
    std::string activePreset = m_core->activeMatchPresetName();
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    for (const auto &name : presetNames) {
        m_presetCombo->addItem(name.data());
    }
    if (activePreset.size()) {
        m_presetCombo->setCurrentText(activePreset.data());
    } else {
        m_presetCombo->addItem(k_unsavedPresetStr);
        m_presetCombo->setCurrentText(k_unsavedPresetStr);
    }
    m_presetCombo->blockSignals(false);

    if (activePreset.empty() && presetNames.size()
     && !m_core->matchConfigDirty()) {
        emit sigMatchPresetSelect(presetNames.first());
    }
}

void PmPresetsWidget::onActivePresetChanged()
{
    std::string activePreset = m_core->activeMatchPresetName();

    m_presetCombo->blockSignals(true);
    int findPlaceholder = m_presetCombo->findText(k_unsavedPresetStr);
    if (activePreset.empty()) {
        if (findPlaceholder == -1) {
            m_presetCombo->addItem(k_unsavedPresetStr);
        }
        m_presetCombo->setCurrentText(k_unsavedPresetStr);
    } else {
        if (findPlaceholder != -1) {
            m_presetCombo->removeItem(findPlaceholder);
        }
        m_presetCombo->setCurrentText(activePreset.data());
    }
    m_presetCombo->blockSignals(false);

    m_presetRemoveButton->setEnabled(!activePreset.empty());
    m_presetExportButton->setEnabled(!activePreset.empty());
}

void PmPresetsWidget::onActivePresetDirtyStateChanged()
{
    bool dirty = m_core->matchConfigDirty();
    std::string activePreset = m_core->activeMatchPresetName();
    m_presetRevertButton->setEnabled(dirty && !activePreset.empty());
    m_presetSaveButton->setEnabled(dirty);
    setTitle(dirty ? obs_module_text("Preset (*)") : obs_module_text("Preset"));
}

void PmPresetsWidget::onPresetsImportAvailable(PmMatchPresets availablePresets)
{
    QList<std::string> selectedPresets;
    if (availablePresets.size() <= 1) {
        PmListSelectorDialog selector(
            obs_module_text("Presets to Import"),
            availablePresets.keys(), availablePresets.keys(), this);
        selectedPresets = selector.selectedChoices();
        if (selector.result() == QFileDialog::Rejected
         || selectedPresets.empty())
            return;
    } else {
        selectedPresets = availablePresets.keys();
    }

    importPresets(availablePresets, selectedPresets);
}

void PmPresetsWidget::onPresetsDownloadAvailable(
    std::string xmlUrl, QList<std::string> presetNames)
{
    PmListSelectorDialog selector(
        obs_module_text("Presets to Download"), presetNames, {}, this);
    QList<std::string> selectedPresets = selector.selectedChoices();

    if (m_presetsRetriever) {
        if (selector.result() == QDialog::Rejected || selectedPresets.empty()) {
            m_presetsRetriever->onAbort();
        } else {
            m_presetsRetriever->retrievePresets(selectedPresets);
        }
    }

    UNUSED_PARAMETER(xmlUrl);
}

QPushButton* PmPresetsWidget::prepareButton(
    const char* tooltip, const char* icoPath)
{
    QIcon icon;
    icon.addFile(icoPath, QSize(), QIcon::Normal, QIcon::Off);

    QPushButton* ret = new QPushButton(icon, "", this);
    ret->setToolTip(tooltip);
    ret->setIcon(icon);
    ret->setIconSize(QSize(16, 16));
    ret->setMaximumSize(22, 22);
    ret->setFlat(true);
    //ret->setProperty("themeID", QVariant(themeId));
    ret->setFocusPolicy(Qt::NoFocus);

    return ret;
}

void PmPresetsWidget::onPresetSelected()
{
    std::string selPreset = m_presetCombo->currentText().toUtf8().data();
    std::string activePreset = m_core->activeMatchPresetName();

    if (selPreset != activePreset && m_core->matchConfigDirty()) {
        auto role = promptUnsavedProceed();
        if (role == QMessageBox::RejectRole) {
            m_presetCombo->blockSignals(true);
            m_presetCombo->setCurrentText(
                activePreset.size() ? activePreset.data() : k_unsavedPresetStr);
            m_presetCombo->blockSignals(false);
            return;
        } else {
            if (role == QMessageBox::YesRole) {
                if (activePreset.empty()) {
                    onPresetSaveAs();
                } else {
                    emit sigMatchPresetSave(activePreset);
                }
            }
        }
    }
    emit sigMatchPresetSelect(selPreset);
}

void PmPresetsWidget::onPresetRevert()
{
    std::string presetName = m_core->activeMatchPresetName();
    if (presetName.size() && m_core->matchConfigDirty()) {
        emit sigMatchPresetActiveRevert();
    }
}

void PmPresetsWidget::onPresetSave()
{
    std::string presetName = m_core->activeMatchPresetName();
    if (presetName.empty()) {
        onPresetSaveAs();
    } else {
        emit sigMatchPresetSave(presetName);
    }
}

void PmPresetsWidget::onPresetSaveAs()
{
    bool ok;
    QString presetNameQstr = QInputDialog::getText(
        this, obs_module_text("Save Preset"), obs_module_text("Enter Name: "),
        QLineEdit::Normal, QString(), &ok);

    std::string presetName = presetNameQstr.toUtf8().data();

    if (!ok || presetName.empty()) return;

    if (m_core->activeMatchPresetName() != presetName
        && m_core->matchPresetExists(presetName)) {
        int ret = QMessageBox::warning(this, 
            obs_module_text("Preset Exists"),
            QString(obs_module_text("Overwrite preset \"%1\"?"))
               .arg(presetNameQstr),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if (ret != QMessageBox::Yes) return;
    }

    emit sigMatchPresetSave(presetName);
}

void PmPresetsWidget::onNewConfig()
{
    std::string activePreset = m_core->activeMatchPresetName();

    if (m_core->matchConfigDirty()) {
        auto role = promptUnsavedProceed();
        if (role == QMessageBox::RejectRole) {
            return;
        } else {
            if (role == QMessageBox::YesRole) {
                if (activePreset.empty()) {
                    onPresetSaveAs();
                } else {
                    emit sigMatchPresetSave(activePreset);
                }
            }
        }
    }

    if (activePreset.empty()) {
        emit sigMultiMatchConfigReset();
    } else {
        emit sigMatchPresetSelect("");
    }
}

void PmPresetsWidget::onPresetRemove()
{
    std::string oldPreset = m_core->activeMatchPresetName();
    if (oldPreset.size() && m_core->matchPresetSize(oldPreset)) {
        int ret = QMessageBox::warning(this,
            obs_module_text("Remove preset?"),
            QString(obs_module_text("Really remove preset \"%1\"?"))
                .arg(oldPreset.data()),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes)
            return;
    }
    emit sigMatchPresetRemove(oldPreset);
}

void PmPresetsWidget::onPresetExportXml()
{
    if (!proceedWithExit()) return;

    QList<std::string> availablePresets = m_core->matchPresetNames();
    QList<std::string> selectedPresets;

    std::string activePresetName = m_core->activeMatchPresetName();
    if (activePresetName.size()) {
        selectedPresets = { activePresetName };
    }

    if (availablePresets.size() > 1) {
        PmListSelectorDialog selector(
            obs_module_text("Presets to Export"),
            availablePresets, selectedPresets, this);
        selectedPresets = selector.selectedChoices();
        if (selector.result() == QDialog::Rejected || selectedPresets.empty())
            return;
    }

    QFileDialog saveDialog(
        this, obs_module_text("Export Preset(s) XML"), QString(),
        PmConstants::k_xmlFilenameFilter);
    QString saveFilename = activePresetName.data();
    saveFilename.replace(PmConstants::k_problemCharacterRegex, "");
    saveDialog.selectFile(saveFilename);
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.exec();

    QStringList selectedFiles = saveDialog.selectedFiles();
    if (saveDialog.result() != QDialog::Accepted
     || selectedFiles.empty()) return;
    QString qstrFilename = selectedFiles.first();
    std::string filename(qstrFilename.toUtf8().data());

    emit sigMatchPresetExport(filename, selectedPresets);
}

void PmPresetsWidget::onPresetImportXml()
{
    if (!proceedWithExit()) return;

    QString qstrFilename = QFileDialog::getOpenFileName(
        this, obs_module_text("Import Presets(s) XML"), QString(),
        PmConstants::k_xmlFilenameFilter);
    std::string filename = qstrFilename.toUtf8().data();
    if (filename.size()) {
        emit sigMatchPresetsImport(filename);
    }
}

void PmPresetsWidget::onPresetDownload()
{
    if (m_presetsRetriever) return; // retrieval already in progress; wait!

    bool ok;
    std::string url = QInputDialog::getText(this,
        obs_module_text("Download Preset"),
        obs_module_text("Enter URL: "),
        QLineEdit::Normal, k_defaultXmlUrl, &ok).toUtf8().data();
    if (ok && url.size()) {
        const Qt::ConnectionType qc = Qt::QueuedConnection;
        m_presetsRetriever = new PmPresetsRetriever(url);
        connect(
            m_presetsRetriever, &PmPresetsRetriever::sigXmlPresetsAvailable,
            this, &PmPresetsWidget::onPresetsDownloadAvailable, qc);
        connect(
            m_presetsRetriever, &PmPresetsRetriever::sigPresetsReady,
            this, &PmPresetsWidget::onPresetsDownloadFinished, qc);
        new PmPresetsRetrievalDialog(m_presetsRetriever, this);
        m_presetsRetriever->downloadXml();
    }
}

void PmPresetsWidget::onPresetsDownloadFinished(PmMatchPresets presets)
{
    importPresets(presets, presets.keys());
}

void PmPresetsWidget::onMatchImagesOrphaned(QList<std::string> filenames)
{
    PmListSelectorDialog selector(obs_module_text(
        "Remove downloaded images no longer used by any preset?"),
        filenames, filenames);
    QList<std::string> selectedImages = selector.selectedChoices();
    if (selectedImages.size()) {
        emit sigMatchImagesRemove(selectedImages);
    }
}

void PmPresetsWidget::importPresets(
    const PmMatchPresets &presets, const QList<std::string> &selectedPresets)
{
    PmMatchPresets newPresets;
    std::string firstImported;
    PmDuplicateNameReaction defaultReaction
        = PmDuplicateNameReaction::Undefined;
    size_t numRemaining = presets.count();
    size_t numReplaced = 0;
    size_t numUnchanged = 0;
    size_t numSkipped = 0;
    size_t numImported = 0;

    for (std::string presetName : selectedPresets) {
        PmMultiMatchConfig mcfg = presets[presetName];

        --numRemaining;

        bool skip = false;
        while (m_core->matchPresetExists(presetName)) {
            // preset with this name exists
            auto existingPreset = m_core->matchPresetByName(presetName);
            const auto& newPreset = presets[presetName];
            if (newPreset == existingPreset) {
                // it's the same as in existing configuration. don't bother
                ++numUnchanged;
                skip = true;
                break;
            }
            // new preset, same name, different configuration.
            PmDuplicateNameReaction reaction;
            if (defaultReaction != PmDuplicateNameReaction::Undefined) {
                // user previously requested to handle all duplicates the same way
                reaction = defaultReaction;
            } else {
                // user needs to make a choice to react
                PmPresetExistsDialog choiceDialog(
                    presetName, numRemaining > 0, this);
                reaction = choiceDialog.choice();
                if (choiceDialog.applyToAll()) {
                    // user requests a default reaction for subsequent duplicates
                    defaultReaction = reaction;
                }
            }

            if (reaction == PmDuplicateNameReaction::Abort) {
                // abort everything
                return;
            } else if (reaction == PmDuplicateNameReaction::Skip) {
                // break out of the name check loop and skip preset
                skip = true;
                ++numSkipped;
                break;
            } else if (reaction == PmDuplicateNameReaction::Replace) {
                // break out of the name check loop and allow overwrite
                ++numReplaced;
                break; 
            } else if (reaction == PmDuplicateNameReaction::Rename) {
                // ask for a new name
                bool ok;
                QString presetNameQstr = QInputDialog::getText(
                    this, obs_module_text("Rename Imported Preset"),
                    obs_module_text("Enter Name: "), QLineEdit::Normal,
                    QString(presetName.data()) + obs_module_text(" (new)"),
                    &ok);
               if (ok && presetNameQstr.size()) {
                    // proceed with attempting to rename
                    presetName = presetNameQstr.toUtf8().data();
                } else {
                    // user cancelled rename. undo default choice to allow aborting, etc.
                    defaultReaction = PmDuplicateNameReaction::Undefined;
                }
            }
        }

        if (!skip) {
            // stage an approved imported preset
            newPresets[presetName] = mcfg;
            ++numImported;
        }
    }

    if (newPresets.size()) {
        emit sigMatchPresetsAppend(newPresets);
    }

    std::ostringstream oss;
    oss << numImported << obs_module_text(" preset(s) imported");
    if (numSkipped > 0) {
        oss << "\n\n" << numSkipped
            << obs_module_text(" existing preset(s) skipped");
    }
    if (numReplaced > 0) {
        oss << "\n\n" << numReplaced
            << obs_module_text(" existing preset(s) replaced");
    }
    if (numUnchanged > 0) {
        oss << "\n\n" << numUnchanged
            << obs_module_text(" existing preset(s) unchanged");
    }
    if (numReplaced > 0) {
        oss << "\n\n" << numReplaced
            << obs_module_text(" existing preset(s) skipped");
    }
    QMessageBox::information(
        this, obs_module_text("Presets Import Finished"),
        oss.str().data());
}

QMessageBox::ButtonRole PmPresetsWidget::promptUnsavedProceed()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(obs_module_text("Unsaved changes"));
    msgBox.setText(obs_module_text(
        "What would you like to do with unsaved changes?"));
    msgBox.addButton(obs_module_text("Save"), QMessageBox::YesRole);
    msgBox.addButton(obs_module_text("Discard"), QMessageBox::NoRole);
    msgBox.addButton(obs_module_text("Cancel"), QMessageBox::RejectRole);

    msgBox.exec();

    return msgBox.buttonRole(msgBox.clickedButton());
}

bool PmPresetsWidget::proceedWithExit()
{
    auto activePreset = m_core->activeMatchPresetName();

    if (m_core->matchConfigDirty()) {
        auto role = promptUnsavedProceed();
        if (role == QMessageBox::RejectRole) {
            return false;
        } else {
            if (role == QMessageBox::YesRole) {
                if (activePreset.empty()) {
                    onPresetSaveAs();
                } else {
                    emit sigMatchPresetSave(activePreset);
                }
            } else {
                onPresetRevert();
            }
        }
    }
    return true;
}
