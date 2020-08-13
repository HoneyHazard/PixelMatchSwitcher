#include "pm-presets-widget.hpp"
#include "pm-core.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QInputDialog>

const char* PmPresetsWidget::k_unsavedPresetStr
    = obs_module_text("<unsaved preset>");

PmPresetsWidget::PmPresetsWidget(PmCore* core, QWidget* parent)
: QGroupBox(obs_module_text("Presets"), parent)
, m_core(core)

{   
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
        this, &PmPresetsWidget::onPresetSave, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetSaveButton);

    m_presetSaveAsButton = prepareButton(obs_module_text("Save Preset As"),
        ":/res/images/icons8-save-as-32.png");
    connect(m_presetSaveAsButton, &QPushButton::released,
        this, &PmPresetsWidget::onPresetSaveAs, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetSaveAsButton);

    m_presetResetButton = prepareButton(obs_module_text("New Configuration"),
        ":/res/images/icons8-file.svg");
    connect(m_presetResetButton, &QPushButton::released,
        this, &PmPresetsWidget::onConfigReset, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetResetButton);

    m_presetRemoveButton = prepareButton(obs_module_text("Remove Preset"),
        ":/res/images/icons8-trash.svg");
    connect(m_presetRemoveButton, &QPushButton::released,
        this, &PmPresetsWidget::onPresetRemove, Qt::QueuedConnection);
    presetLayout->addWidget(m_presetRemoveButton);
    
    // core event handlers
    connect(m_core, &PmCore::sigAvailablePresetsChanged,
        this, &PmPresetsWidget::onAvailablePresetsChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigActivePresetChanged,
        this, &PmPresetsWidget::onActivePresetChanged, Qt::QueuedConnection);
    connect(m_core, &PmCore::sigActivePresetDirtyChanged,
        this, &PmPresetsWidget::onActivePresetDirtyStateChanged, Qt::QueuedConnection);

    // local signals -> core slots
    connect(this, &PmPresetsWidget::sigSaveMatchPreset,
        m_core, &PmCore::onSaveMatchPreset, Qt::QueuedConnection);
    connect(this, &PmPresetsWidget::sigSelectActiveMatchPreset,
        m_core, &PmCore::onSelectActiveMatchPreset, Qt::QueuedConnection);
    connect(this, &PmPresetsWidget::sigRemoveMatchPreset,
        m_core, &PmCore::onRemoveMatchPreset, Qt::QueuedConnection);
    connect(this, &PmPresetsWidget::sigResetMatchConfigs,
        m_core, &PmCore::onResetMatchConfigs, Qt::QueuedConnection);


    // finish init state
    onAvailablePresetsChanged();
    onActivePresetChanged();
    onActivePresetDirtyStateChanged();
}

void PmPresetsWidget::onAvailablePresetsChanged()
{
    auto presetNames = m_core->matchPresetNames();
    m_presetCombo->blockSignals(true);
    m_presetCombo->clear();
    for (const auto &name : presetNames) {
        m_presetCombo->addItem(name.data());
    }
    m_presetCombo->blockSignals(false);
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
}

void PmPresetsWidget::onActivePresetDirtyStateChanged()
{
    bool dirty = m_core->matchConfigDirty();
    m_presetSaveButton->setEnabled(dirty);
    setTitle(dirty ? obs_module_text("Preset (*)") : obs_module_text("Preset"));
}

QPushButton* PmPresetsWidget::prepareButton(const char* tooltip, const char* icoPath)
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

    if (activePreset.size() && selPreset != activePreset
     && m_core->matchConfigDirty()) {
        int ret = QMessageBox::warning(this,
            obs_module_text("Unsaved changes"),
            obs_module_text("Unsaved changes will be lost.\nProceed?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes) {
            m_presetCombo->setCurrentText(activePreset.data());
            return;
        }
    }

    emit sigSelectActiveMatchPreset(selPreset);
}

void PmPresetsWidget::onPresetSave()
{
    std::string presetName = m_core->activeMatchPresetName();
    if (presetName.empty()) {
        onPresetSaveAs();
    } else {
        emit sigSaveMatchPreset(presetName);
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

    emit sigSaveMatchPreset(presetName);
}

void PmPresetsWidget::onConfigReset()
{
    if (m_core->matchConfigDirty()) {
        int ret = QMessageBox::warning(this, 
            obs_module_text("Unsaved changes"),
            obs_module_text("Unsaved changes will be lost.\nProceed?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes)
            return;
    }

    std::string activePreset = m_core->activeMatchPresetName();
    if (activePreset.empty()) {
        emit sigResetMatchConfigs();
    } else {
        emit sigSelectActiveMatchPreset("");
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
    emit sigRemoveMatchPreset(oldPreset);
}
