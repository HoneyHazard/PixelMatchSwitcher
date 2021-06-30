#include "pm-match-reaction-widget.hpp"
#include "pm-structs.hpp"
#include "pm-core.hpp"
#include "pm-add-action-menu.hpp"
#include "pm-version.hpp"

#include <obs-module.h>
#include <util/dstr.hpp>

#include <QComboBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QStandardItemModel>
#include <QMouseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>

#include <sstream>

#if BROWSER_AVAILABLE
#include <browser-panel.hpp>
extern QCef *cef;
#endif
#include <QDesktopServices>
#include <QUrl>

const QString PmActionEntryWidget::k_defaultTransitionStr
    = obs_module_text("<default transition>");
const int PmActionEntryWidget::k_keyRole = Qt::UserRole + 1;
const int PmActionEntryWidget::k_modifierRole = Qt::UserRole + 2;
const int PmActionEntryWidget::k_keyHintRole = Qt::UserRole + 3;
const char *PmActionEntryWidget::k_timeFormatHelpUrl
    = "https://doc.qt.io/qt-5/qdatetime.html#toString-2";
const char *PmActionEntryWidget::k_fileMarkersHelpHtml = obs_module_text(
    "File markers work in both file names and entries written to files: "
    "<br /><br />"
    "<b>[label]</b>  Insert match config label"
    "<br /><br />"
    "<b>[time]</b>  Insert date/time"
);

PmActionEntryWidget::PmActionEntryWidget(
    PmCore* core, size_t actionIndex, QWidget *parent)
: QWidget(parent)
, m_core(core)
, m_actionIndex(actionIndex)
{
    // for all types of targets
    m_targetCombo = new QComboBox(this);

    // transitions for scenes
    m_transitionsCombo = new QComboBox(this);

    // toggle scene items and filters on and off
    m_toggleSourceCombo = new QComboBox(this);
    m_toggleSourceCombo->addItem(
        obs_module_text("Show"), (unsigned int)(PmToggleCode::On));
    m_toggleSourceCombo->addItem(
        obs_module_text("Hide"), (unsigned int)PmToggleCode::Off);

    m_toggleMuteCombo = new QComboBox(this);
    m_toggleMuteCombo->addItem(
        obs_module_text("Unmute"), (unsigned int)(PmToggleCode::On));
    m_toggleMuteCombo->addItem(
        obs_module_text("Mute"), (unsigned int)PmToggleCode::Off);

    // hotkeys   
    m_hotkeyDetailsLabel = new QLabel(this);
 
    // file operations
    int row = 0;
    QGridLayout *fileLayout = new QGridLayout;
    QLabel *fileActionLabel = new QLabel(
        obs_module_text("File Action: "), this);
    fileLayout->addWidget(fileActionLabel, row, 0);

    m_fileActionCombo = new QComboBox(this);
    m_fileActionCombo->addItem(
        PmAction::fileActionStr(PmFileActionType::WriteAppend),
        (size_t)PmFileActionType::WriteAppend);
    m_fileActionCombo->addItem(
        PmAction::fileActionStr(PmFileActionType::WriteTruncate),
        (size_t)PmFileActionType::WriteTruncate);
    fileLayout->addWidget(m_fileActionCombo, row, 1);

    m_fileMarkersHelpButton
        = new QPushButton(obs_module_text("Markers Help"), this);
    fileLayout->addWidget(m_fileMarkersHelpButton, row, 2);
    row++;

    QLabel *filenameLabel = new QLabel(obs_module_text("Filename: "), this);
    fileLayout->addWidget(filenameLabel, row, 0);

    m_filenameEdit = new QLineEdit(this);
    fileLayout->addWidget(m_filenameEdit, row, 1);

    m_fileBrowseButton = new QPushButton("...", this);
    fileLayout->addWidget(m_fileBrowseButton, row, 2);
    row++;

    QLabel *textLabel = new QLabel(obs_module_text("Text: "), this);
    fileLayout->addWidget(textLabel, row, 0);
    m_fileTextEdit = new QLineEdit(this);
    fileLayout->addWidget(m_fileTextEdit, row, 1);
    m_fileStringsPreviewButton =
	    new QPushButton(obs_module_text("Preview"), this);
    fileLayout->addWidget(m_fileStringsPreviewButton, row, 2);
    row++;

    m_fileTimeFormatLabel = new QLabel(obs_module_text("Time Format: "), this);
    m_fileTimeFormatLabel->setVisible(false);
    fileLayout->addWidget(m_fileTimeFormatLabel, row, 0);
    m_fileTimeFormatEdit = new QLineEdit(this);
    m_fileTimeFormatEdit->setVisible(false);
    fileLayout->addWidget(m_fileTimeFormatEdit, row, 1);

    m_fileTimeFormatHelpButton = new QPushButton(obs_module_text("Help"), this);
    m_fileTimeFormatHelpButton->setVisible(false);
    fileLayout->addWidget(m_fileTimeFormatHelpButton, row, 2);
    row++;

    m_fileActionsWidget = new QWidget(this);
    m_fileActionsWidget->setLayout(fileLayout);

    // selectively shows and selects details for different types of targets
    m_detailsStack = new QStackedWidget(this);
    m_detailsStack->addWidget(m_transitionsCombo);
    m_detailsStack->addWidget(m_toggleSourceCombo);
    m_detailsStack->addWidget(m_hotkeyDetailsLabel);
    m_detailsStack->addWidget(m_toggleMuteCombo);

    // main layout
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(m_targetCombo);
    mainLayout->addWidget(m_detailsStack);
    mainLayout->addWidget(m_fileActionsWidget);
    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 1);
    mainLayout->setStretch(2, 1);
    setLayout(mainLayout);

    // connections: local UI -> action
    connect(m_targetCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_transitionsCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_toggleSourceCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_toggleMuteCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);

    connect(m_fileActionCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_filenameEdit, &QLineEdit::editingFinished,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_fileTextEdit, &QLineEdit::editingFinished,
            this, &PmActionEntryWidget::onUiChanged);
    connect(m_fileTimeFormatEdit, &QLineEdit::editingFinished,
            this, &PmActionEntryWidget::onUiChanged);

    // connections: local UI customization
    connect(m_toggleSourceCombo, &QComboBox::currentTextChanged,
            this, &PmActionEntryWidget::onHotkeySelectionChanged);
    connect(m_fileMarkersHelpButton, &QPushButton::released,
            this, &PmActionEntryWidget::onShowFileMarkersHelp);
    connect(m_filenameEdit, &QLineEdit::editingFinished,
            this, &PmActionEntryWidget::onFileStringsChanged);
    connect(m_fileBrowseButton, &QPushButton::released,
            this, &PmActionEntryWidget::onFileBrowseReleased);
    connect(m_fileTextEdit, &QLineEdit::editingFinished,
            this, &PmActionEntryWidget::onFileStringsChanged);
    connect(m_fileStringsPreviewButton, &QPushButton::released,
            this, &PmActionEntryWidget::onShowFilePreview);
    connect(m_fileTimeFormatHelpButton, &QPushButton::released,
            this, &PmActionEntryWidget::onShowTimeFormatHelp);

    // init state
    onScenesChanged();
    onHotkeySelectionChanged();

    onFileStringsChanged();
    showFileActionsUi(false);
}

void PmActionEntryWidget::updateScenes()
{
	QBrush sceneBrush
        = PmAction::dimmedColor(PmActionType::Scene, m_actionType);
    QBrush siBrush = PmAction::dimmedColor(
        PmActionType::SceneItem, m_actionType);
	QBrush fiBrush = PmAction::dimmedColor(
        PmActionType::Filter, m_actionType);

    QList<std::string> scenes = m_core->sceneNames();
    auto model = dynamic_cast<QStandardItemModel *>(m_targetCombo->model());
    int idx;

    PmActionType aType = m_actionType;
    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    QString selStr = QString("<%1 %2>")
		.arg(obs_module_text("select"))
        .arg(PmAction::actionStr(aType));
    m_targetCombo->addItem(selStr, "");
    model->item(0)->setEnabled(false);
    for (const std::string &scene : scenes) {
	    m_targetCombo->addItem(scene.data(), scene.data());
	    idx = m_targetCombo->count() - 1; 
	    m_targetCombo->setItemData(idx, sceneBrush, Qt::ForegroundRole);
	    if (aType != PmActionType::Scene) {
		    QList<std::string> siNames = m_core->sceneItemNames(scene);
		    if (siNames.empty()) {
			    m_targetCombo->removeItem(idx);
			    continue;
            }
		    model->item(idx)->setEnabled(false);
		    for (const std::string &siName : siNames) {
			    m_targetCombo->addItem(
                    QString("  ") + siName.data(), siName.data());
			    idx = m_targetCombo->count() - 1;
                m_targetCombo->setItemData(idx, siBrush, Qt::ForegroundRole);
			    if (aType != PmActionType::SceneItem) {
                    QList<std::string> fiNames = m_core->filterNames(siName);
				    if (fiNames.empty()) {
                        m_targetCombo->removeItem(idx);
					    continue;
                    }
				    model->item(idx)->setEnabled(false);
                    for (const std::string& fiName : fiNames) {
                        m_targetCombo->addItem(
                            QString("    ") + fiName.data(), fiName.data());
			            idx = m_targetCombo->count() - 1;
                        m_targetCombo->setItemData(
                            idx, fiBrush, Qt::ForegroundRole);
                    }
                }
            }
	    }
    }
    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::updateAudioSources()
{
	QBrush dimmedBrush =
		PmAction::dimmedColor(PmActionType::ToggleMute, m_actionType);
	QBrush colorBrush =
		PmAction::dimmedColor(PmActionType::ToggleMute, m_actionType);

	QList<std::string> audioSources = m_core->audioSourcesNames();
	auto model = dynamic_cast<QStandardItemModel *>(m_targetCombo->model());
	int idx;

	m_targetCombo->blockSignals(true);
	m_targetCombo->clear();
	QString selStr =
		QString("<%1>").arg(obs_module_text("select audio source"));
	m_targetCombo->addItem(selStr, "");
	model->item(0)->setEnabled(false);

	for (const std::string &audioSrc : audioSources) {
		m_targetCombo->addItem(audioSrc.data(), audioSrc.data());
		idx = m_targetCombo->count() - 1;
		m_targetCombo->setItemData(idx, colorBrush, Qt::ForegroundRole);
	}
}

void PmActionEntryWidget::updateTransitons()
{
    QList<std::string> transitions = m_core->availableTransitions();

    m_transitionsCombo->blockSignals(true);
    m_transitionsCombo->clear();
    m_transitionsCombo->addItem(k_defaultTransitionStr, QVariant(QString()));
    for (const std::string &t : transitions) {
        m_transitionsCombo->addItem(t.data(), t.data());
    }
    m_transitionsCombo->blockSignals(false);
}

void PmActionEntryWidget::updateSizeHints(QList<QSize> &columnSizes)
{
    QList<QSize> sizes = {
        //m_actionTypeCombo->sizeHint(),
        m_targetCombo->sizeHint(),
        m_detailsStack->sizeHint()
    };
    while (columnSizes.size() < 2) {
        columnSizes.append({0, 0});
    }
    for (int i = 0; i < columnSizes.size(); ++i) {
        columnSizes[i] = {
            qMax(columnSizes[i].width(), sizes[i].width()),
            qMax(columnSizes[i].height(), sizes[i].height())
        };
    }
}

void PmActionEntryWidget::actionToUi(
    size_t actionIndex, PmAction action, const std::string &cfgLabel)
{
    if (m_actionIndex != actionIndex) return;

    PmActionType prevType = m_actionType;
    if (prevType != action.actionType) {
        m_actionType = action.actionType;
        prepareSelections();
    }

    switch (action.actionType) {
    case PmActionType::None:
        m_targetCombo->setVisible(false);
        m_detailsStack->setVisible(false);
        showFileActionsUi(false);
        break;
    case PmActionType::Scene:
        m_targetCombo->setVisible(true);
        m_targetCombo->blockSignals(true);
        m_targetCombo->setCurrentText(action.targetElement.data());
        m_targetCombo->blockSignals(false);

        m_transitionsCombo->blockSignals(true);
        m_transitionsCombo->setCurrentText(action.targetDetails.data());
        m_transitionsCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
	    selectDetailsWidget(m_transitionsCombo);
    	showFileActionsUi(false);
	    break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
         m_targetCombo->setVisible(true);
         m_targetCombo->blockSignals(true);
         m_targetCombo->setCurrentText(action.targetElement.data());
         m_targetCombo->blockSignals(false);

         m_toggleSourceCombo->blockSignals(true);
         m_toggleSourceCombo->setCurrentIndex(
            m_toggleSourceCombo->findData(action.actionCode));
         m_toggleSourceCombo->blockSignals(false);

         m_detailsStack->setVisible(true);
	     selectDetailsWidget(m_toggleSourceCombo);
         showFileActionsUi(false);
         break;
    case PmActionType::ToggleMute:
	    m_targetCombo->setVisible(true);
	    m_targetCombo->blockSignals(true);
	    m_targetCombo->setCurrentText(action.targetElement.data());
	    m_targetCombo->blockSignals(false);

        m_toggleMuteCombo->blockSignals(true);
        m_toggleMuteCombo->setCurrentIndex(
            m_toggleMuteCombo->findData(action.actionCode));
        m_toggleMuteCombo->blockSignals(false);

        m_detailsStack->setVisible(true);
	    selectDetailsWidget(m_toggleMuteCombo);
	    showFileActionsUi(false);
        break;
    case PmActionType::Hotkey:
	    m_targetCombo->setVisible(true);
	    m_targetCombo->blockSignals(true);
	    if (action.isSet()) {
		    DStr dstr;
		    obs_key_combination_to_str(action.keyCombo, dstr);
		    int targetIdx = m_targetCombo->findData((const char*)dstr);
			m_targetCombo->setCurrentIndex(targetIdx);
	    } else {
		    m_targetCombo->setCurrentIndex(0);
        }
        m_targetCombo->blockSignals(false);
        m_detailsStack->setVisible(true);
	    onHotkeySelectionChanged();
    	selectDetailsWidget(m_hotkeyDetailsLabel);
	    showFileActionsUi(false);
        break;
    case PmActionType::FrontEndAction:
	    m_targetCombo->setVisible(true);
	    m_targetCombo->blockSignals(true);
	    if (action.isSet()) {
		    int targetIdx
                = m_targetCombo->findData((unsigned int)action.actionCode);
		    m_targetCombo->setCurrentIndex(targetIdx);
	    } else {
		    m_targetCombo->setCurrentIndex(0);
	    }
	    m_targetCombo->blockSignals(false);
        m_detailsStack->setVisible(false);
	    showFileActionsUi(false);
	    break;
    case PmActionType::File:
	    m_targetCombo->setVisible(false);
	    m_detailsStack->setVisible(false);

        m_fileActionCombo->blockSignals(true);
	    if (action.isSet()) {
	        int targetIdx = m_fileActionCombo->findData(
    		    (unsigned int)action.actionCode);
            m_fileActionCombo->setCurrentIndex(targetIdx);
        } else {
            m_fileActionCombo->setCurrentIndex(0);
        } 
        m_fileActionCombo->blockSignals(false);

        m_filenameEdit->blockSignals(true);
	    m_filenameEdit->setText(action.targetElement.data());
        m_filenameEdit->blockSignals(false);

        m_fileTextEdit->blockSignals(true);
        m_fileTextEdit->setText(action.targetDetails.data());
	    m_fileTextEdit->blockSignals(false);

        m_fileTimeFormatEdit->blockSignals(true);
	    m_fileTimeFormatEdit->setText(action.timeFormat.data());
	    m_fileTimeFormatEdit->blockSignals(false);

	    onFileStringsChanged();

        {
		    // update file preview
		    QDateTime now = QDateTime::currentDateTime();
		    std::ostringstream oss;
		    oss << "<b>Filename:</b><br />"
		        << action.formattedFileString(
                      action.targetElement, cfgLabel, now).data()
		        << "<br /><br /><b>Entry Preview:</b><br />"
		        << action.formattedFileString(
                      action.targetDetails, cfgLabel, now).data();
		    m_filePreviewStr = oss.str().data();
	    }
	    showFileActionsUi(true);
	    break;
    }

    updateUiStyle(action);
}

void PmActionEntryWidget::insertHotkeysList(
    int &idx, const QString &info, HotkeysList list)
{
	if (list.isEmpty()) return;

    #if 0
	std::stable_sort(begin(list), end(list),
		[&](const HotkeyData &a, const HotkeyData &b) {
		const auto &nameA = std::get<2>(a);
		const auto &nameB = std::get<2>(b);
		return nameA < nameB;
	});
    #endif

    auto model = (QStandardItemModel *)m_targetCombo->model();
	QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::Hotkey);
    QString header = QString("---- %1 ----").arg(info);
    m_targetCombo->addItem(header);
	model->item(idx)->setEnabled(false);    m_targetCombo->setItemData(idx, dimmedBrush, Qt::ForegroundRole);
    idx++;

    QBrush selBrush = PmAction::actionColor(PmActionType::Hotkey);
    for (const auto &hkeyData : list) {
	    const char *descr = obs_hotkey_get_description(hkeyData.hkey);
	    DStr comboStr;
	    obs_key_combination_to_str(hkeyData.keyCombo, comboStr);

	    QString targetStr = QString("%1 (%2)")
            .arg(descr).arg((const char*)comboStr);

        m_targetCombo->addItem(targetStr, (const char*)comboStr);
	    m_targetCombo->setItemData(idx, selBrush, Qt::ForegroundRole);
	    m_targetCombo->setItemData(
            idx, (int)hkeyData.keyCombo.key, k_keyRole);
        m_targetCombo->setItemData(
            idx, (uint32_t)hkeyData.keyCombo.modifiers, k_modifierRole);
	    m_targetCombo->setItemData(idx, info, k_keyHintRole);
	    idx++;
    }
}

void PmActionEntryWidget::insertHotkeysGroup(
    int &idx, const QString &category, const HotkeysGroup &group)
{
	//auto model = (QStandardItemModel *)m_targetCombo->model();
	//QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::Hotkey);

	for (const auto &key : group.uniqueKeys()) {
		QString info = QString("[%1] %2").arg(category).arg(key.data());
		insertHotkeysList(idx, info, group.values(key));
    }
}

void PmActionEntryWidget::updateHotkeys()
{
    HotkeysList allKeys;
    HotkeysList frontendKeys;
    HotkeysGroup outputsKeys;
    HotkeysGroup encodersKeys;
    HotkeysGroup servicesKeys;
    HotkeysGroup scenesKeys;
    HotkeysGroup sourcesKeys;

    #if 0
    obs_enum_hotkeys(
        [](void *data, obs_hotkey_id id, obs_hotkey_t *key) -> bool
        {
		    HotkeysList *hotkeys = (HotkeysList *)data;
		    hotkeys->push_back(
                std::make_tuple(key, id, obs_hotkey_get_description(key)));
		    return true;
        },
        (void*)&allKeys);
    #endif

    obs_enum_hotkey_bindings(
	    [](void *data, size_t idx, obs_hotkey_binding_t *binding) -> bool {
		    HotkeysList *hotkeys = (HotkeysList *)data;
		    //obs_hotkey_id id =
			//    obs_hotkey_binding_get_hotkey_id(binding);
		    obs_hotkey_t* key = obs_hotkey_binding_get_hotkey(binding);
		    obs_key_combination_t combo =
			    obs_hotkey_binding_get_key_combination(binding);
            HotkeyData hkeyData = { key, combo };
		    hotkeys->push_back(hkeyData);
            UNUSED_PARAMETER(idx);
            return true;
	    },
	    (void *)&allKeys);


    for (const auto &hkeyData : allKeys) {
	    auto hkey = hkeyData.hkey;
	    obs_hotkey_registerer_type rtype =
		    obs_hotkey_get_registerer_type(hkey);
	    void *registerer = obs_hotkey_get_registerer(hkey);
	    switch (rtype) {
	    case OBS_HOTKEY_REGISTERER_FRONTEND:
		    frontendKeys.push_back(hkeyData);
		    break;

	    case OBS_HOTKEY_REGISTERER_SOURCE: {
		    auto weakSource =
			    static_cast<obs_weak_source_t *>(registerer);
		    auto source = OBSGetStrongRef(weakSource);
		    if (source) {
			    auto name = obs_source_get_name(source);
			    if (obs_scene_t *scene =
					obs_scene_from_source(source)) {
				    scenesKeys.insert(name, hkeyData);
			    } else {
				    sourcesKeys.insert(name, hkeyData);
			    }
		    }
	    } break;

	    case OBS_HOTKEY_REGISTERER_OUTPUT: {
		    auto weakOutput =
			    static_cast<obs_weak_output_t *>(registerer);
		    auto output = OBSGetStrongRef(weakOutput);
		    if (output) {
			    auto name = obs_output_get_name(output);
			    outputsKeys.insert(name, hkeyData);
		    }
	    } break;

	    case OBS_HOTKEY_REGISTERER_ENCODER: {
		    auto weakEncoder =
			    static_cast<obs_weak_encoder_t *>(registerer);
		    auto encoder = OBSGetStrongRef(weakEncoder);
		    if (encoder) {
			    auto name = obs_encoder_get_name(encoder);
			    encodersKeys.insert(name, hkeyData);
		    }
	    } break;

	    case OBS_HOTKEY_REGISTERER_SERVICE: {
		    auto weakService =
			    static_cast<obs_weak_service_t *>(registerer);
		    auto service = OBSGetStrongRef(weakService);
		    if (service) {
			    auto name = obs_service_get_name(service);
			    servicesKeys.insert(name, hkeyData);
		    }
	    } break;
	    }
    }

    auto model = (QStandardItemModel *)m_targetCombo->model();
    QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::Hotkey);

    m_targetCombo->blockSignals(true);
    m_targetCombo->clear();
    int idx = 0;
    m_targetCombo->addItem(obs_module_text("<select hotkey>"), (unsigned int)-1);
    model->item(idx)->setEnabled(false);
    m_targetCombo->setItemData(idx, dimmedBrush, Qt::ForegroundRole);
    idx++;

    insertHotkeysList(idx, obs_module_text("[frontend]"), frontendKeys);
    insertHotkeysGroup(idx, obs_module_text("output"), outputsKeys);
    insertHotkeysGroup(idx, obs_module_text("encoder"), encodersKeys);
    insertHotkeysGroup(idx, obs_module_text("service"), servicesKeys);
    insertHotkeysGroup(idx, obs_module_text("scene"), scenesKeys);
    insertHotkeysGroup(idx, obs_module_text("source"), sourcesKeys);

    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::insertFrontendEntries(
    int &comboIdx, const QString &category,
    PmFrontEndAction start, PmFrontEndAction end)
{
	auto model = (QStandardItemModel *)m_targetCombo->model();
    QBrush colorBrush = PmAction::actionColor(PmActionType::FrontEndAction);
    QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::FrontEndAction);

    QString title = QString("--- %1 ---").arg(category);
    m_targetCombo->addItem(title);
    m_targetCombo->setItemData(comboIdx, dimmedBrush, Qt::ForegroundRole);
    model->item(comboIdx)->setEnabled(false);
    comboIdx++;

    for (int feaIdx = (int)start; feaIdx <= (int)end; feaIdx++) {
	    PmFrontEndAction fea = (PmFrontEndAction)feaIdx;
	    m_targetCombo->addItem(PmAction::frontEndActionStr(fea), feaIdx);
	    m_targetCombo->setItemData(comboIdx, colorBrush, Qt::ForegroundRole);
	    comboIdx++;
    }
}

void PmActionEntryWidget::updateFrontendActions()
{
	auto model = (QStandardItemModel *)m_targetCombo->model();
	int idx = 0;

	QBrush dimmedBrush = PmAction::dimmedColor(PmActionType::FrontEndAction);

	m_targetCombo->blockSignals(true);
	m_targetCombo->clear();

	m_targetCombo->addItem(obs_module_text("<select front end action>"));
	m_targetCombo->setItemData(idx, dimmedBrush, Qt::ForegroundRole);
	model->item(idx)->setEnabled(false);
	idx++;

	insertFrontendEntries(idx, obs_module_text("streaming"),
	    PmFrontEndAction::StreamingStart, PmFrontEndAction::StreamingStop);
	insertFrontendEntries(idx, obs_module_text("recording"),
		PmFrontEndAction::RecordingStart, PmFrontEndAction::RecordingUnpause);
	insertFrontendEntries(idx, obs_module_text("replay buffer"),
        PmFrontEndAction::ReplayBufferStart,
        PmFrontEndAction::ReplayBufferStop);
	insertFrontendEntries(idx, obs_module_text("other"),
        PmFrontEndAction::TakeScreenshot, PmFrontEndAction::ResetVideo);

    m_targetCombo->blockSignals(false);
}

void PmActionEntryWidget::installEventFilterAll(QObject *obj)
{
	for (QWidget *w : QObject::findChildren<QWidget *>()) {
		w->installEventFilter(obj);
    }
}

void PmActionEntryWidget::prepareSelections()
{
    switch (m_actionType) {
    case PmActionType::Scene:
        updateScenes();
        updateTransitons();
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        updateScenes();
        break;
    case PmActionType::Hotkey:
	    updateHotkeys();
        break;
    case PmActionType::FrontEndAction:
	    updateFrontendActions();
	    break;
    case PmActionType::ToggleMute:
	    updateAudioSources();
	    break;
    default:
	    break;
    }
}

void PmActionEntryWidget::onUiChanged()
{
    PmAction action;
	action.actionType = m_actionType;

    switch (PmActionType(m_actionType)) {
    case PmActionType::None:
        break;
    case PmActionType::Scene:
        action.targetElement
            = m_targetCombo->currentData().toString().toUtf8();
        action.targetDetails
            = m_transitionsCombo->currentData().toString().toUtf8();
        break;
    case PmActionType::SceneItem:
    case PmActionType::Filter:
        action.targetElement
            = m_targetCombo->currentData().toString().toUtf8();
	    action.actionCode = (size_t)m_toggleSourceCombo->currentData().toUInt();
        break;
    case PmActionType::ToggleMute:
	    action.targetElement =
		    m_targetCombo->currentData().toString().toUtf8();
	    action.actionCode = (size_t)m_toggleMuteCombo->currentData().toUInt();
	    break;
    case PmActionType::Hotkey:
	    action.keyCombo.key
            = (obs_key_t) m_targetCombo->currentData(k_keyRole).toInt();
	    action.keyCombo.modifiers
            = (uint32_t) m_targetCombo->currentData(k_modifierRole).toInt();
	    break;
    case PmActionType::FrontEndAction:
	    action.actionCode = (size_t)m_targetCombo->currentData().toUInt();
	    break;
    case PmActionType::File:
	    action.actionCode = (size_t)m_fileActionCombo->currentData().toUInt();
	    action.targetElement = m_filenameEdit->text().toUtf8();
	    action.targetDetails = m_fileTextEdit->text().toUtf8();
	    action.timeFormat = m_fileTimeFormatEdit->text().toUtf8();
	    break;
    }

    emit sigActionChanged(m_actionIndex, action);

    updateUiStyle(action);
}

void PmActionEntryWidget::onHotkeySelectionChanged()
{
	if (m_actionType == PmActionType::Hotkey) {
		QString info = m_targetCombo->currentData(k_keyHintRole).toString();
		m_hotkeyDetailsLabel->setText(info);
    }
}

void PmActionEntryWidget::onFileBrowseReleased()
{
	QString curPath =
		QFileInfo(m_filenameEdit->text()).absoluteDir().path();

	QString filename = QFileDialog::getSaveFileName(
		this, obs_module_text("Choose file save location"), curPath,
		PmConstants::k_writeFilenameFilter);
	if (!filename.isEmpty()) {
		m_filenameEdit->setText(filename);
    }
	onUiChanged();
}

void PmActionEntryWidget::onFileStringsChanged()
{
	if (m_actionType == PmActionType::File) {
		bool timeUsed
            = m_filenameEdit->text().contains(PmAction::k_timeMarker.data())
           || m_fileTextEdit->text().contains(PmAction::k_timeMarker.data());
		bool timeWasUsed = m_fileTimeFormatEdit->isVisible();
		if (timeUsed != timeWasUsed) {
            m_fileTimeFormatLabel->setVisible(timeUsed);
			m_fileTimeFormatEdit->setVisible(timeUsed);
			m_fileTimeFormatHelpButton->setVisible(timeUsed);
			adjustSize();
		}
	}
}

void PmActionEntryWidget::onShowFileMarkersHelp()
{
	QMessageBox::information(
		this, obs_module_text("File markers help"),
		k_fileMarkersHelpHtml, QMessageBox::Ok);
}

void PmActionEntryWidget::onShowFilePreview()
{
	QMessageBox::information(
        this, obs_module_text("Filename and file entry preview"),
        m_filePreviewStr, QMessageBox::Ok);
}

void PmActionEntryWidget::onShowTimeFormatHelp()
{
#if BROWSER_AVAILABLE
	QCef *cef = obs_browser_init_panel();
	if (!cef)
		goto fallback;

    QCefWidget *cefWidget = cef->create_widget(nullptr, k_timeFormatHelpUrl);
	if (!cefWidget)
		goto fallback;

	QDialog *hostDialog = new QDialog(nullptr);
	hostDialog->setWindowTitle(obs_module_text("Time Format Help"));
	cefWidget->setParent(hostDialog);
	hostDialog->exec();
#endif

fallback:
	QDesktopServices::openUrl(QUrl(k_timeFormatHelpUrl));
}

void PmActionEntryWidget::onScenesChanged()
{
	prepareSelections();
}

void PmActionEntryWidget::updateUiStyle(const PmAction &action)
{
	QString colorStyle = QString("color: %1").arg(action.actionColorStr());

    for (QWidget *w : findChildren<QWidget *>()) {
		if (w != m_hotkeyDetailsLabel)
            w->setStyleSheet(colorStyle);
    }
}

void PmActionEntryWidget::selectDetailsWidget(QWidget *widget)
{
	m_detailsStack->setCurrentWidget(widget);
}

void PmActionEntryWidget::showFileActionsUi(bool on)
{
	m_fileActionsWidget->setVisible(on);
	QSizePolicy sp;
	if (on) {
		sp = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	} else {
		sp = QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }
	sp.setRetainSizeWhenHidden(false);
	m_fileActionsWidget->setSizePolicy(sp);
}

//----------------------------------------------------

PmMatchReactionWidget::PmMatchReactionWidget(
    PmCore *core, PmAddActionMenu *addActionMenu,
    PmReactionTarget reactionTarget, PmReactionType reactionType,
    QWidget *parent)
: PmSpoilerWidget(parent)
, m_core(core)
, m_addActionMenu(addActionMenu)
, m_reactionTarget(reactionTarget)
, m_reactionType(reactionType)
{
	QIcon insertIcon;
	insertIcon.addFile(":/res/images/add.png", QSize(), QIcon::Normal,
			   QIcon::Off);
	m_insertActionButton = new QPushButton(this);
	m_insertActionButton->setIcon(insertIcon);
	m_insertActionButton->setIconSize(QSize(16, 16));
	m_insertActionButton->setMaximumSize(22, 22);
	m_insertActionButton->setFlat(true);
	m_insertActionButton->setProperty("themeID", QVariant("addIconSmall"));
	m_insertActionButton->setToolTip(
		obs_module_text("Insert New Action"));
	m_insertActionButton->setFocusPolicy(Qt::NoFocus);
	connect(m_insertActionButton, &QPushButton::released,
            this, &PmMatchReactionWidget::onInsertReleased);

    QIcon removeIcon;
	removeIcon.addFile(
        ":/res/images/list_remove.png", QSize(), QIcon::Normal, QIcon::Off);
    m_removeActionButton = new QPushButton(this);
	m_removeActionButton->setIcon(removeIcon);
    m_removeActionButton->setIconSize(QSize(16, 16));
	m_removeActionButton->setMaximumSize(22, 22);
    m_removeActionButton->setFlat(true);
    m_removeActionButton->setProperty("themeID", QVariant("removeIconSmall"));
    m_removeActionButton->setToolTip(obs_module_text("Remove Action"));
    m_removeActionButton->setFocusPolicy(Qt::NoFocus);
    connect(m_removeActionButton, &QPushButton::released, this,
	        &PmMatchReactionWidget::onRemoveReleased);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_insertActionButton);
    buttonLayout->addWidget(m_removeActionButton);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    setTopRightLayout(buttonLayout);

    m_actionListWidget = new QListWidget(this);
    m_actionListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_actionListWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_actionListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_actionListWidget->setSizeAdjustPolicy(QListWidget::AdjustToContents);
    connect(m_actionListWidget, &QListWidget::currentRowChanged,
            this, &PmMatchReactionWidget::updateButtonsState);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_actionListWidget);
    setContentLayout(mainLayout);
    m_contentArea->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Expanding);

    // entry vs no-match specific
    const auto qc = Qt::QueuedConnection;

    if (reactionTarget == PmReactionTarget::Entry) {
        connect(m_core, &PmCore::sigMatchConfigSelect,
                this, &PmMatchReactionWidget::onMatchConfigSelect, qc);
        connect(m_core, &PmCore::sigMatchConfigChanged,
                this, &PmMatchReactionWidget::onMatchConfigChanged, qc);
        connect(m_core, &PmCore::sigMultiMatchConfigSizeChanged,
                this, &PmMatchReactionWidget::onMultiMatchConfigSizeChanged, qc);

        connect(this, &PmMatchReactionWidget::sigMatchConfigChanged,
                m_core, &PmCore::onMatchConfigChanged, qc);

        onMultiMatchConfigSizeChanged(m_core->multiMatchConfigSize());
        size_t idx = m_core->selectedConfigIndex();
        onMatchConfigSelect(idx, m_core->matchConfig(idx));
    } else { // Anything
        connect(m_core, &PmCore::sigNoMatchReactionChanged,
                this, &PmMatchReactionWidget::onNoMatchReactionChanged, qc);
        connect(this, &PmMatchReactionWidget::sigNoMatchReactionChanged,
                m_core, &PmCore::onNoMatchReactionChanged, qc);

        onNoMatchReactionChanged(m_core->noMatchReaction());
    }
}

void PmMatchReactionWidget::onMatchConfigChanged(
    size_t matchIdx, PmMatchConfig cfg)
{
    if (matchIdx != m_matchIndex) return;

    const PmReaction &reaction = cfg.reaction;
    reactionToUi(reaction, cfg.label);
}

void PmMatchReactionWidget::onNoMatchReactionChanged(PmReaction reaction)
{
	reactionToUi(reaction, "global");
}

PmReaction PmMatchReactionWidget::pullReaction() const
{
    return m_reactionTarget == PmReactionTarget::Entry
        ? m_core->reaction(m_matchIndex) : m_core->noMatchReaction();
}

void PmMatchReactionWidget::pushReaction(const PmReaction &reaction)
{
    if (m_reactionTarget == PmReactionTarget::Entry) {
        PmMatchConfig cfg = m_core->matchConfig(m_matchIndex);
        cfg.reaction = reaction;
        emit sigMatchConfigChanged(m_matchIndex, cfg);
    } else {
        emit sigNoMatchReactionChanged(reaction);
    }
}

void PmMatchReactionWidget::reactionToUi(
    const PmReaction &reaction, const std::string &cfgLabel)
{
    const auto &actionList = (m_reactionType == PmReactionType::Match) ?
        reaction.matchActions : reaction.unmatchActions;

    size_t listSz = actionList.size();
    for (size_t i = 0; i < listSz; ++i) {
	    QListWidgetItem *item = nullptr;
        PmActionEntryWidget *entryWidget = nullptr;
        if (i >= m_actionListWidget->count()) {
            const auto qc = Qt::QueuedConnection;
            entryWidget = new PmActionEntryWidget(m_core, i, this);
            entryWidget->installEventFilterAll(this);
            connect(m_core, &PmCore::sigScenesChanged,
                    entryWidget, &PmActionEntryWidget::onScenesChanged, qc);
            connect(entryWidget, &PmActionEntryWidget::sigActionChanged,
                    this, &PmMatchReactionWidget::onActionChanged, qc);

            item = new QListWidgetItem(m_actionListWidget);
            m_actionListWidget->addItem(item);
            m_actionListWidget->setItemWidget(item, entryWidget);
        } else {
            item = m_actionListWidget->item(int(i));
            entryWidget
                = (PmActionEntryWidget*) m_actionListWidget->itemWidget(item);
        }
        entryWidget->actionToUi(i, actionList[i], cfgLabel);
	    item->setSizeHint(entryWidget->sizeHint());
    }
    while (m_actionListWidget->count() > listSz) {
        QListWidgetItem *item = m_actionListWidget->takeItem(
            m_actionListWidget->count() - 1);
        delete item;
    }

    int listMinHeight = 10;
    if (listSz > 0) {
	    int idx = m_actionListWidget->currentRow();
	    if (idx < 0) idx = 0;
	    listMinHeight = m_actionListWidget->sizeHintForRow(idx);
    }
    m_actionListWidget->setMinimumHeight(listMinHeight);

    bool expand = (listSz > 0);
    toggleExpand(expand); // calls updateButtonsState() and updateContentHeight()
    updateTitle();

    // will ensure the last item is visible and selected after being added
    if (m_lastActionCount != listSz
     && listSz > 0
     && !actionList[listSz-1].isSet()) {
	    auto lastItem = m_actionListWidget->item((int)listSz - 1);
	    m_actionListWidget->scrollToItem(lastItem);
	    lastItem->setSelected(true);
    }
    m_lastActionCount = listSz;
}

int PmMatchReactionWidget::maxContentHeight() const
{
	int ret = 0;
	int count = m_actionListWidget->count();
	const int extraPadding = 10; // TODO: figure out where it's coming from

	for (int i = 0; i < count; i++) {
		ret += m_actionListWidget->sizeHintForRow(i);
    }
	if (m_actionListWidget->horizontalScrollBar()->isVisible()) {
	    ret += m_actionListWidget->horizontalScrollBar()->sizeHint().height();
    }
	return ret + extraPadding;
}

bool PmMatchReactionWidget::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = (QMouseEvent *)event;
		auto listPos = m_actionListWidget->mapFromGlobal(
            mouseEvent->globalPos());
		auto item = m_actionListWidget->itemAt(listPos);
		if (item)
			item->setSelected(true);
	}
	return QObject::eventFilter(obj, event);
}

void PmMatchReactionWidget::updateButtonsState()
{
	bool available = true;
	if (m_reactionTarget == PmReactionTarget::Entry) {
		available = m_matchIndex < m_multiConfigSz;
    }
    m_insertActionButton->setVisible(available);
    m_removeActionButton->setVisible(available && isExpanded());

    int actionIdx = m_actionListWidget->currentRow();
    m_removeActionButton->setEnabled(actionIdx >= 0);
}

void PmMatchReactionWidget::updateTitle()
{
	QString str;
	if (m_reactionTarget == PmReactionTarget::Global) {
		str = (m_reactionType == PmReactionType::Match)
		    ? obs_module_text("Anything Matched Actions")
		    : obs_module_text("Nothing Matched Actions");
	} else {
		str = QString(obs_module_text("%1 Actions #%2: %3"))
			.arg((m_reactionType == PmReactionType::Match)
				     ? obs_module_text("Match")
				     : obs_module_text("Unmatch"))
			.arg(m_matchIndex + 1)
			.arg(m_core->matchConfigLabel(m_matchIndex).data());
	}

    if (m_actionListWidget->count() > 0) {
		str += QString(" [%1]").arg(m_actionListWidget->count());
    }
    setTitle(str);
}

void PmMatchReactionWidget::toggleExpand(bool on)
{
	if (m_actionListWidget->count() == 0)
		on = false;

	PmSpoilerWidget::toggleExpand(on);

    updateButtonsState();
}

void PmMatchReactionWidget::onMatchConfigSelect(
    size_t matchIndex, PmMatchConfig cfg)
{
    m_matchIndex = matchIndex;

    setEnabled(matchIndex < m_multiConfigSz);

    onMatchConfigChanged(matchIndex, cfg);
}

void PmMatchReactionWidget::onMultiMatchConfigSizeChanged(size_t sz)
{
    m_multiConfigSz = sz;
    setEnabled(m_matchIndex < m_multiConfigSz);
}

void PmMatchReactionWidget::onActionChanged(size_t actionIndex, PmAction action)
{
    PmReaction reaction = pullReaction();

    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions[actionIndex] = action;
    } else {
        reaction.unmatchActions[actionIndex] = action;
    }

    pushReaction(reaction);
}

void PmMatchReactionWidget::onInsertReleased()
{
	m_addActionMenu->setTypeAndTarget(m_reactionTarget, m_reactionType);
	m_addActionMenu->setMatchIndex(m_matchIndex);
	m_addActionMenu->popup(QCursor::pos());
}

void PmMatchReactionWidget::onRemoveReleased()
{
    int idx = 0;
    if (m_actionListWidget->count() > 0) {
        idx = (size_t)m_actionListWidget->currentRow();
        if (idx < 0)
            return;
    } else {                       
        return;
    }

    PmReaction reaction = pullReaction();
    if (m_reactionType == PmReactionType::Match) {
        reaction.matchActions.erase(reaction.matchActions.begin() + idx);
    } else { // Unmatch
        reaction.unmatchActions.erase(reaction.unmatchActions.begin() + idx);
    }
    pushReaction(reaction);
}
